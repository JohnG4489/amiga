/*
**    $VER: Socket.c 2.0
**
**    Source Release 2.0
**
**    Standard source code
**
**    (C) Copyright 2000-2005 Régis Gréau
**        All Right Reserved
*/

#include <stdarg.h>
#include <assert.h>
#include <stdio.h>

#include "socket.h"

#ifdef SOCKET_AMIGA
#include <proto/dos.h>
#include <dos/dos.h>
#include <dos/dostags.h>
#endif


/* Compatibility redefined functions */
#ifdef SOCKET_AMIGA
/* Memory functions */
#define SOCKET_ALLOC(a) AllocVec(a,MEMF_ANY)
#define SOCKET_ALLOCCLEAR(a) AllocVec(a,MEMF_ANY|MEMF_CLEAR)
#define SOCKET_FREE(a) FreeVec(a)
#define SOCKET_MEMCPY(a,b,c) CopyMem((APTR)(b),(APTR)(a),(ULONG)(c))
/* Socket functions */
#define SOCKET_CLOSESOCKET(a) CloseSocket(a)
#define SOCKET_INETNTOA(a) Inet_NtoA((a).s_addr)
#define SOCKET_ACCEPT(a,b,c) accept((LONG)a,(struct sockaddr *)b,(LONG *)c)
#define SOCKET_SELECT(a,b,c,d,e,f) WaitSelect((int)(a),b,c,d,e,f)
#define SOCKET_SOCKET(a,b,c) socket((LONG)(a),(LONG)(b),(LONG)(c))
#define SOCKET_LISTEN(a,b) listen(a,b)
#define SOCKET_CONNECT(a,b,c) connect((int)(a),(struct sockaddr *)(b),(int)(c))
#define SOCKET_IOCTLSOCKET(a,b,c) IoctlSocket(a,b,(char *)(c))
#define SOCKET_ERROR -1
/* Semaphores functions */
#define SOCKET_INITSEMAPHORE(a) InitSemaphore(a)
#define SOCKET_OBTAINSEMAPHORE(a) ObtainSemaphore(a)
#define SOCKET_RELEASESEMAPHORE(a) ReleaseSemaphore(a)
#else /* Not AMIGA */
/* Memory functions */
#define SOCKET_ALLOC(a) malloc(a)
#define SOCKET_ALLOCCLEAR(a) calloc(1,a)
#define SOCKET_FREE(a) free(a)
#define SOCKET_MEMCPY(a,b,c) memcpy(a,b,c)
/* Socket functions */
#ifdef SOCKET_UNIX
#define SOCKET_CLOSESOCKET(a) close((int)(a))
#define SOCKET_IOCTLSOCKET(a,b,c) ioctl(a,b,c)
#else
#define SOCKET_CLOSESOCKET(a) closesocket(a)
#define SOCKET_IOCTLSOCKET(a,b,c) ioctlsocket(a,b,c)
#endif
#define SOCKET_INETNTOA(a) inet_ntoa(a)
#define SOCKET_ACCEPT(a,b,c) accept((int)(a),(struct sockaddr *)(b),(socklen_t *)(c))
#define SOCKET_SELECT(a,b,c,d,e,f) select((int)(a),(fd_set *)(b),(fd_set *)(c),(fd_set *)(d),(e))
#define SOCKET_SOCKET(a,b,c) socket((int)(a),(int)(b),(int)(c))
#define SOCKET_LISTEN(a,b) listen((int)(a),(int)(b))
#define SOCKET_CONNECT(a,b,c) connect((int)(a),(struct sockaddr *)(b),(socklen_t)(c))
#ifdef SOCKET_UNIX 
#define SOCKET_ERROR -1
#endif
/* Semaphores functions */
#ifdef SOCKET_UNIX
#define SOCKET_INITSEMAPHORE(a)
#define SOCKET_OBTAINSEMAPHORE(a)
#define SOCKET_RELEASESEMAPHORE(a)
#else
#define SOCKET_INITSEMAPHORE(a) InitializeCriticalSection(a)
#define SOCKET_OBTAINSEMAPHORE(a) EnterCriticalSection(a)
#define SOCKET_RELEASESEMAPHORE(a) LeaveCriticalSection(a)
#endif
#endif

/* Amiga classic or full compatible only */
#if defined(SOCKET_AMIGA)
struct Library *SocketBase=NULL;
struct SignalSemaphore SignalSemaphore;
struct MsgPort *MainMsgPort=NULL;

#if defined(SOCKET_AMIGAOS4)
struct SocketIFace *ISocket=NULL;
#endif
#endif

/* Windows compatibility */
#if defined(SOCKET_WINDOWS)
CRITICAL_SECTION SignalSemaphore;
#endif

#define SOCKET_INETADDR(a) inet_addr(a)

/* Some prototypes */
LONG __inline SocketListen(struct Socket *,LONG);


/* Some static functions */
static struct Socket __inline *SocketAlloc(LONG HostNameSize)
{
	struct Socket *Socket;

	if((Socket=(struct Socket *)SOCKET_ALLOCCLEAR(sizeof(struct Socket)))!=NULL)
		if((Socket->s_IP=(UBYTE *)SOCKET_ALLOC(HostNameSize))!=NULL)
		{
			Socket->s_SockAddrInLen=sizeof(struct sockaddr_in);
			Socket->s_Socket=(SOCKET)SOCKET_ERROR;
			return Socket;
		}
	SOCKET_FREE(Socket);
	return NULL;
}

static VOID __inline SocketFree(struct Socket *Socket)
{
	if(Socket)
	{
		SOCKET_FREE(Socket->s_IP);
		SOCKET_FREE(Socket);
	}
}

static LONG SocketGetTagData(LONG Tag,LONG Default,LONG *TagList)
{
	LONG a;

	for(a=0;TagList[a];a+=2)
		if(TagList[a]==Tag)
			return TagList[a+1];
	return Default;
}

static LONG __inline SocketStrLen(UBYTE *String)
{
	LONG a;

	for(a=0;String[a];a++);
	return a;
}

static VOID __inline SocketStrCpy(UBYTE *StringDst,UBYTE *StringSrc)
{
	LONG c=0;

	while((StringDst[c]=StringSrc[c])!=0) c++;
}

VOID __inline SocketBZero(UBYTE *Buffer,LONG BufferLen)
{
	LONG a;

	for(a=0;a<BufferLen;a++) Buffer[a]=0;
}

/* Sockets functions */
BOOL SocketOpenLibrary(VOID)
{
	SOCKET_INITSEMAPHORE(&SignalSemaphore);
#if defined(SOCKET_AMIGA)
	if((SocketBase=OpenLibrary("bsdsocket.library",4L))!=NULL) 
#if defined(SOCKET_AMIGAOS4)
	ISocket=(struct SocketIFace *)GetInterface(SocketBase,"main",1L,NULL);
#endif
	if((MainMsgPort=CreateMsgPort())!=NULL)
		return TRUE;
	return FALSE;
#endif
#ifdef SOCKET_WINDOWS
	{
		WSADATA wsaData;

		return !WSAStartup(MAKEWORD(1,1)/*VersionRequested*/,&wsaData);
	}
#endif
}

VOID SocketCloseLibrary(VOID)
{
#if defined(SOCKET_AMIGA)
	if(MainMsgPort!=NULL) DeleteMsgPort(MainMsgPort);
#if defined(SOCKET_AMIGAOS4)
	DropInterface((struct Interface *)ISocket);
#endif
	CloseLibrary(SocketBase);
	SocketBase=NULL;
#endif
#ifdef SOCKET_WINDOWS
	WSACleanup();
#endif
}

struct Socket *SocketCreateConnectTagList(LONG *Tags)
{
	struct Socket *Socket;
	UWORD Port;
	UBYTE *BindAddr;
	LONG ErrorCode=TRUE,Listen;

	assert(Tags!=NULL);
	if((Socket=SocketAlloc(1024))!=NULL)
	{
		gethostname((char *)Socket->s_IP,1024);
		Listen=(LONG)SocketGetTagData(SOCKETCREATECONNECT_Listen,(LONG)64,Tags);
		Port=(UWORD)SocketGetTagData(SOCKETCREATECONNECT_Port,(LONG)80,Tags);
		BindAddr=(UBYTE *)SocketGetTagData(SOCKETCREATECONNECT_Bind,(LONG)NULL,Tags);
		Socket->s_Port=Port;
		Socket->s_SockAddrIn.sin_family=AF_INET;
		Socket->s_SockAddrIn.sin_port=htons(Port);
		Socket->s_SockAddrIn.sin_addr.s_addr=BindAddr?SOCKET_INETADDR(BindAddr):INADDR_ANY;
		if((Socket->s_Socket=SOCKET_SOCKET(AF_INET,SOCK_STREAM,0))!=SOCKET_ERROR)
			if(!bind(Socket->s_Socket,(struct sockaddr *)&Socket->s_SockAddrIn,(socklen_t)sizeof(struct sockaddr_in)))
				if(!SocketListen(Socket,Listen))
					ErrorCode=FALSE;
		if(ErrorCode)
		{
			SocketDeleteConnect(Socket);
			Socket=NULL;
		}
	}
	return Socket;
}

#if !defined(SOCKET_MORPHOS) && !defined(SOCKET_AMIGAOS4) && !defined(SOCKET_UNIX)
struct Socket *SocketCreateConnectTags(LONG Tag,...)
{
	return SocketCreateConnectTagList(&Tag);
}
#endif

VOID SocketDeleteConnect(struct Socket *Socket)
{
	if(Socket)
	{
		if(Socket->s_Socket>=0) SOCKET_CLOSESOCKET(Socket->s_Socket);
		SocketFree(Socket);
	}
}

int SocketSetBlock(struct Socket *Socket,long Block)
{
	long NoBlock;

	NoBlock=!Block;
	return SOCKET_IOCTLSOCKET(Socket->s_Socket,FIONBIO,&NoBlock);
}

BOOL SocketInitConnect(struct Socket *Socket,LONG Timeout)
{
	struct timeval timeout={0,0};
	struct hostent *RemoteHost;
	fd_set Write;
	//long NoBlock=1;
	BOOL RetCode=FALSE;
	ULONG Signal=0;
	ULONG *Wait=&Signal;
	int con;

	Socket->s_Socket=-1;
	timeout.tv_sec=Timeout;
	SOCKET_OBTAINSEMAPHORE(&SignalSemaphore);
	if((RemoteHost=gethostbyname((char *)Socket->s_IP))!=NULL)
	{
		SOCKET_MEMCPY(&Socket->s_SockAddrIn.sin_addr,RemoteHost->h_addr,RemoteHost->h_length);
		Socket->s_SockAddrIn.sin_family=AF_INET; //emoteHost->h_addrtype;
		Socket->s_SockAddrIn.sin_port=htons(Socket->s_Port);
		Socket->s_SockAddrInLen=sizeof(struct sockaddr_in);
	}
	SOCKET_RELEASESEMAPHORE(&SignalSemaphore);
	if(RemoteHost!=NULL)
		if((Socket->s_Socket=SOCKET_SOCKET(AF_INET,SOCK_STREAM,0))>=0)
		{
			if(Timeout>=0) SocketSetBlock(Socket,0);
			con=SOCKET_CONNECT(Socket->s_Socket,&Socket->s_SockAddrIn,sizeof(struct sockaddr_in));
			if(Timeout>0 && con)
			{
				MYFDZERO(&Write);
				FD_SET(Socket->s_Socket,&Write);
				if(SOCKET_SELECT(Socket->s_Socket+1,NULL,&Write,NULL,&timeout,Wait))
				{
					//SOCKET_IOCTLSOCKET(Socket->s_Socket,FIONBIO,&NoBlock);
					SocketSetBlock(Socket,1);
					con=0;
				}
			} else
				if(!Timeout) con=0;

			if(!con) RetCode=TRUE;
		}
	return RetCode;
}

struct Socket *SocketOpenConnectTagList(LONG *Tags)
{
	struct Socket *Socket;
	UBYTE *HostName;

	assert(Tags!=NULL);
	HostName=(UBYTE *)SocketGetTagData(SOCKETOPENCONNECT_HostName,(LONG)"",Tags);
	if((Socket=SocketAlloc(SocketStrLen(HostName)+1))!=NULL)
	{
		SocketStrCpy(Socket->s_IP,HostName);
		Socket->s_Port=(UWORD)SocketGetTagData(SOCKETOPENCONNECT_Port,(LONG)80,Tags);
		if(!SocketInitConnect(Socket,SocketGetTagData(SOCKETOPENCONNECT_Timeout,(LONG)-1,Tags)))
		{
			SocketCloseConnect(Socket);
			Socket=NULL;
		}
	}
	return Socket;
}

#if !defined(SOCKET_MORPHOS) && !defined(SOCKET_AMIGAOS4) && !defined(SOCKET_UNIX)
struct Socket *SocketOpenConnectTags(LONG Tag,...)
{
	return SocketOpenConnectTagList(&Tag);
}
#endif

VOID SocketCloseConnect(struct Socket *Socket)
{
	if(Socket)
	{
		if(Socket->s_Socket>=0) SOCKET_CLOSESOCKET(Socket->s_Socket);
		SocketFree(Socket);
	}
}


LONG __inline SocketListen(struct Socket *Socket,LONG Listen)
{
	assert(Socket!=NULL);
	return (LONG)SOCKET_LISTEN(Socket->s_Socket,Listen);
}

struct Socket *SocketAcceptConnect(struct Socket *Socket)
{
	struct Socket *NewSocket;

	assert(Socket!=NULL);
	if((NewSocket=SocketAlloc(64))!=NULL)
		if((NewSocket->s_Socket=SOCKET_ACCEPT(Socket->s_Socket,&NewSocket->s_SockAddrIn,&NewSocket->s_SockAddrInLen))>=0)
		{
			//printf("Socket: %d\n",(int)NewSocket->s_Socket); sleep(5);
			SOCKET_OBTAINSEMAPHORE(&SignalSemaphore);
			SocketStrCpy(NewSocket->s_IP,(UBYTE *)SOCKET_INETNTOA(NewSocket->s_SockAddrIn.sin_addr));
			//printf("Ip: %s\n",NewSocket->s_IP);
			SOCKET_RELEASESEMAPHORE(&SignalSemaphore);
			NewSocket->s_Port=Socket->s_Port;
			return NewSocket;
		}
	SocketFree(NewSocket);
	return NULL;
}

LONG SocketWaitTagList(LONG *Tags)
{
	struct timeval TimeOut1={0,0},*TimeOut=NULL;
	SOCKETFDSET *PReadFDS=NULL,*PWriteFDS=NULL,*PExceptFDS=NULL;
	ULONG Signal=0;
	ULONG *Wait=&Signal;
	LONG Tag1,a;
	int FDMax=0;

	assert(Tags!=NULL);
	for(a=0;(Tag1=Tags[a*2])!=SOCKETTAG_DONE;a++)
		switch(Tag1)
		{
			case SOCKETWAIT_Read:
				PReadFDS=(SOCKETFDSET *)Tags[a*2+1];
				if(FDMax<PReadFDS->fds_Max) FDMax=PReadFDS->fds_Max;
				break;
			case SOCKETWAIT_Write:
				PWriteFDS=(SOCKETFDSET *)Tags[a*2+1];
				if(FDMax<PWriteFDS->fds_Max) FDMax=PWriteFDS->fds_Max;
				break;
			case SOCKETWAIT_Except:
				PExceptFDS=(SOCKETFDSET *)Tags[a*2+1];
				if(FDMax<PExceptFDS->fds_Max) FDMax=PExceptFDS->fds_Max;
				break;
			case SOCKETWAIT_Wait:
				Wait=(ULONG *)Tags[a*2+1];
				break;
			case SOCKETWAIT_TimeOutSec:
				TimeOut=(&TimeOut1);
				TimeOut->tv_sec=Tags[a*2+1];
				break;
			case SOCKETWAIT_TimeOutMic:
				TimeOut=(&TimeOut1);
				TimeOut->tv_usec=Tags[a*2+1];
				break;
		}
	//printf("Socket max: %d\n",(int)FDMax+1);
	return (LONG)SOCKET_SELECT(FDMax+1,PReadFDS?&PReadFDS->fds_Fds:NULL,PWriteFDS?&PWriteFDS->fds_Fds:NULL,PExceptFDS?&PExceptFDS->fds_Fds:NULL,TimeOut,Wait);
}

#if !defined(SOCKET_MORPHOS) && !defined(SOCKET_AMIGAOS4) && !defined(SOCKET_UNIX)
LONG __inline SocketWaitTags(LONG Tags,...)
{
	return SocketWaitTagList(&Tags);
}
#endif

LONG __inline SocketRead(struct Socket *Socket,VOID *Buffer,LONG Len)
{
	assert(Socket!=NULL);
	assert(Buffer!=NULL);
	return recv((int)(Socket->s_Socket),(void *)(Buffer),(int)(Len),0);
}

LONG __inline SocketWrite(struct Socket *Socket,VOID *Buffer,LONG Len)
{
	assert(Socket!=NULL);
	assert(Buffer!=NULL);
	return send((int)(Socket->s_Socket),(void *)(Buffer),(int)(Len),0);
}


/* Experimental Code **************************************************************************/
#ifdef SOCKET_AMIGA
#include <dos/dostags.h>


BOOL SocketInitConnect2(struct Socket *Socket,struct Library *SocketBase)
{
	struct hostent *RemoteHost;
	BOOL RetCode=FALSE;

	SOCKET_OBTAINSEMAPHORE(&SignalSemaphore);
	if((RemoteHost=gethostbyname((char *)Socket->s_IP))!=NULL)
	{
		SOCKET_MEMCPY(&Socket->s_SockAddrIn.sin_addr,RemoteHost->h_addr,RemoteHost->h_length);
		Socket->s_SockAddrIn.sin_family=RemoteHost->h_addrtype;
		Socket->s_SockAddrIn.sin_port=htons(Socket->s_Port);
		Socket->s_SockAddrInLen=sizeof(struct sockaddr_in);
	}
	SOCKET_RELEASESEMAPHORE(&SignalSemaphore);
	if(RemoteHost!=NULL && (Socket->s_Socket=SOCKET_SOCKET(AF_INET,SOCK_STREAM,0))!=SOCKET_ERROR)
		if(!SOCKET_CONNECT(Socket->s_Socket,&Socket->s_SockAddrIn,sizeof(struct sockaddr_in)))
			RetCode=TRUE;
	return RetCode;
}

#if 1


VOID ASyncManager(VOID)
{
	struct Library *SocketBase;
	struct Task *Task=FindTask(NULL);
		struct MsgPort *MsgPort=(struct MsgPort *)Task->tc_UserData;
		struct MessageSocket *MessageSocket;
		struct Socket *Socket;
		struct hostent *RemoteHost;
		BOOL TaskEnd=FALSE; 
	ULONG WaitSignal;

	Printf("ASyncManager created\n");
	while(!TaskEnd)
	{
		WaitSignal=Wait(SIGBREAKF_CTRL_D);
		//if(WaitSignal&SIGBREAKF_CTRL_D)
		while(MessageSocket=(struct MessageSocket *)GetMsg(MsgPort))
		{
			MessageSocket->ms_Error=SOCKETERR_FAILED;
			switch(MessageSocket->ms_Command)
			{
				case SOCKETCMD_OPEN:
					Printf("Received SOCKETCMD_OPEN!\n");
					if((SocketBase=OpenLibrary("bsdsocket.library",4))!=NULL)
							MessageSocket->ms_Error=SOCKETERR_OK;
					break;
				case SOCKETCMD_CLOSE:
					Printf("Received SOCKETCMD_CLOSE!\n");
					MessageSocket->ms_Error=SOCKETERR_OK;
					CloseLibrary(SocketBase);
					TaskEnd=TRUE;
					break;
				case SOCKETCMD_DNS:
					Printf("Received SOCKETCMD_OPEN!\n");
					Socket=MessageSocket->ms_Socket;
					if((RemoteHost=gethostbyname((char *)Socket->s_IP))!=NULL)
					{
						SOCKET_MEMCPY(&Socket->s_SockAddrIn.sin_addr,RemoteHost->h_addr,RemoteHost->h_length);
						Socket->s_SockAddrIn.sin_family=RemoteHost->h_addrtype;
						Socket->s_SockAddrIn.sin_port=htons(Socket->s_Port);
						Socket->s_SockAddrInLen=sizeof(struct sockaddr_in);
						MessageSocket->ms_Error=SOCKETERR_OK;
					}
					break;
				default:
					Printf("Received default!\n");
					break;
			}
			ReplyMsg((struct Message *)MessageSocket);
		}
	}
	Printf("ASyncManager deleted\n");
	//Forbid();
}
#endif

struct MsgPort *SocketOpenSubTask(VOID);
VOID SocketCloseSubTask(struct MsgPort *);

struct MsgPort *SocketOpenSubTask(VOID)
{
	struct Task *Task;
	struct MsgPort *MsgPort;
	struct MessageSocket MessageSocket;
	BOOL ErrorCode=TRUE;

	if(MsgPort=(struct MsgPort *)AllocVec(sizeof(struct MsgPort),MEMF_ANY))
	{
		Forbid();
		if(Task=(struct Task *)CreateNewProcTags(NP_Entry,(ULONG)ASyncManager,
		                                         NP_StackSize,1024,
                                                 NP_Priority,0,
                                                 NP_Name,(ULONG)"SubConnect_Task",
                                                 NP_Output,Output(),
                                                 NP_CloseOutput,FALSE,
                                                 TAG_DONE))
		{
			Task->tc_UserData=MsgPort;
			/* Creation du MsgPort de la tache fille */
			MsgPort->mp_SigBit=SIGBREAKB_CTRL_D;
			MsgPort->mp_Flags=PA_SIGNAL;
			MsgPort->mp_SigTask=Task;
			NewList(&MsgPort->mp_MsgList);

			/* On initialise la tache et attend la reponse */
			MessageSocket.ms_Message.mn_ReplyPort=MainMsgPort;
			MessageSocket.ms_Command=SOCKETCMD_OPEN;
			PutMsg(MsgPort,(struct Message *)&MessageSocket);
			WaitPort(MessageSocket.ms_Message.mn_ReplyPort);

			/* Recuperation de la reponse */
			GetMsg(MessageSocket.ms_Message.mn_ReplyPort);
			if(MessageSocket.ms_Error!=SOCKETERR_OK)
			ErrorCode=FALSE;
		}
		if(ErrorCode)
		{
			SocketCloseSubTask(MsgPort);
			MsgPort=NULL;
		}
		Permit();
	}
	return MsgPort;
}

VOID SocketCloseSubTask(struct MsgPort *MsgPort)
{
	struct MessageSocket MessageSocket;

	if(MsgPort)
	{
		if(MsgPort->mp_SigTask)
		{
			/* Fermeture de la tache fille et attente de la reponse */
			MessageSocket.ms_Message.mn_ReplyPort=MainMsgPort;
			MessageSocket.ms_Command=SOCKETCMD_CLOSE;
			PutMsg(MsgPort,(struct Message *)&MessageSocket);
			WaitPort(MessageSocket.ms_Message.mn_ReplyPort);

			/* Recuperation de la reponse et desallocation */
			GetMsg(MessageSocket.ms_Message.mn_ReplyPort);
		}
		FreeVec(MsgPort);
	}
}


struct Socket *SocketOpenConnect2TagList(LONG *Tags)
{
	struct Socket *Socket;
	UBYTE *HostName;
	//BOOL RetCode=FALSE;

	assert(Tags!=NULL);
	HostName=(UBYTE *)SocketGetTagData(SOCKETOPENCONNECT_HostName,(LONG)"",Tags);
	if((Socket=SocketAlloc(SocketStrLen(HostName)+1))!=NULL)
	{
		SocketStrCpy(Socket->s_IP,HostName);
		Socket->s_Port=(UWORD)SocketGetTagData(SOCKETOPENCONNECT_Port,(LONG)80,Tags);
		//Socket->s_Signal=(UWORD)SocketGetTagData(SOCKETOPENCONNECT_Wait,(LONG)80,Tags);
		if(Socket->s_MsgPort=SocketOpenSubTask())
		{
			Socket->s_MessageSocket.ms_Command=SOCKETCMD_DNS;
			Socket->s_MessageSocket.ms_Socket=Socket;
			PutMsg(Socket->s_MsgPort,(struct Message *)&Socket->s_MessageSocket);
		} else
		{
			SocketFree(Socket);
			Socket=NULL;
		}
	}
	return Socket;
}

BOOL SocketWaitConnect(struct Socket *Socket,LONG Signal)
{
	/*BOOL RetCode=FALSE;*/

	if((Socket->s_Socket=SOCKET_SOCKET(AF_INET,SOCK_STREAM,0))!=SOCKET_ERROR)
	{
		SOCKETFDSET SFDWrite;
		long NoBlock=0;

		Wait(1<<MainMsgPort->mp_SigBit|Signal);
		SOCKET_IOCTLSOCKET(Socket->s_Socket,FIONBIO,&NoBlock);
		SOCKET_CONNECT(Socket->s_Socket,&Socket->s_SockAddrIn,sizeof(struct sockaddr_in));
		SOCKET_FDZERO(&SFDWrite);
		SOCKET_FDSET(Socket,&SFDWrite);
		SocketWaitTags(SOCKETWAIT_Write,&SFDWrite,SOCKETWAIT_Wait,Signal,SOCKETTAG_DONE);
	}
	return TRUE;
}

struct Socket *SocketOpenConnect2Tags(LONG Tag,...)
{
	return SocketOpenConnect2TagList(&Tag);
}

VOID SocketCloseConnect2(struct Socket *Socket)
{
	if(Socket)
	{
		SocketCloseSubTask(Socket->s_MsgPort);
		if(Socket->s_Socket!=SOCKET_ERROR) SOCKET_CLOSESOCKET(Socket->s_Socket);
		SocketFree(Socket);
	}
}
#endif
/* End of Experimental Code *******************************************************************/
