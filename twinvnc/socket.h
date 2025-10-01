#ifndef SOCKET_H
#define SOCKET_H
/*
**    $VER: Socket.h 2.0 (17/10/2005) New dev.
**                   1.1 (16/11/2001) (Add SocketStr())
**                   1.0 (06/12/2000)
**    Includes Release 2.0
**
**    Standards structure and value definitions
**
**    (C) Copyright 2000-2005 Régis Gréau
**        All Right Reserved
*/

#ifdef __cplusplus
extern "C" {
#endif

/* __SASC_60 or __SASC or __GNUC__ or __GNUC__ or VC++ */
#if defined(__SASC) || defined(__amigaos4__) || defined(__MORPHOS__)
	#define SOCKET_AMIGA
	//#warning "SOCKET_AMIGA"
	#ifdef __amigaos4__
		#define SOCKET_AMIGAOS4
		//#warning "SOCKET_AMIGAOS4"
	#endif
	#ifdef __MORPHOS__
		#define SOCKET_MORPHOS
		//#warning "SOCKET_MORPHOS"
	#endif
#else
	#ifdef __GNUC__
		#define SOCKET_UNIX
		//#warning "SOCKET_UNIX"
	#else
		#define SOCKET_WINDOWS
		//#warning "SOCKET_WINDOWS"
	#endif
#endif

/* Some includes */
#ifdef SOCKET_AMIGA
	#include <proto/exec.h>
	#include <proto/dos.h>
	#include <exec/memory.h>
	#include <utility/tagitem.h>
	#include <proto/socket.h>
	#include <netdb.h>
	#include <sys/types.h>
	#include <sys/ioctl.h>
	//#include <sys/errno.h>
#endif

#ifdef SOCKET_WINDOWS
	#include <winsock.h>
	#include <stdlib.h>
	#include <string.h>
#endif

#ifdef SOCKET_UNIX
	#include <sys/time.h>
	#include <sys/select.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/wait.h>
	#include <sys/param.h>
	#include <sys/ioctl.h>
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <stdlib.h>
	#include <string.h>
#endif

#if defined(SOCKET_WINDOWS) || defined(SOCKET_UNIX)
	#ifndef TAG_DONE
	#define TAG_DONE 0
#endif

/* Some booleans and types defines */
#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#ifndef BOOL
	#define BOOL long
#endif
#ifndef VOID
	#define VOID void
#endif
#ifndef LONG
	#define LONG long
#endif
#ifndef ULONG
	#define ULONG unsigned long
#endif
#ifndef UBYTE
	#define UBYTE unsigned char
#endif
#ifndef UWORD
	#define UWORD unsigned short
#endif

#endif

/* Some socket types defines */
#if defined(SOCKET_AMIGA) || defined(SOCKET_WINDOWS)
	#define SOCKET int
	typedef int socklen_t;
#endif
#ifdef SOCKET_UNIX
	#define SOCKET int
#endif

/* Redefine type */
struct SocketFDSet
{
	int fds_Max;
	fd_set fds_Fds;
};
//#define SOCKETFDSET fd_set
typedef struct SocketFDSet SOCKETFDSET;
#ifdef SOCKET_AMIGA
	#define SOCKET_FDZERO(a) SocketBZero((UBYTE *)(a),(LONG)(sizeof(*(a))))
	#define MYFDZERO SOCKET_FDZERO
#else
	#define SOCKET_FDZERO(a) {FD_ZERO(&(a)->fds_Fds); (a)->fds_Max=0;}
	#define MYFDZERO FD_ZERO
#endif
#define SOCKET_FDSET(a,b) {FD_SET((a)->s_Socket,&((b)->fds_Fds)); if((b)->fds_Max<(a)->s_Socket) (b)->fds_Max=(a)->s_Socket;}
#define SOCKET_FDISSET(a,b) FD_ISSET((a)->s_Socket,&((b)->fds_Fds))
#define SOCKET_FDCLR(a,b) FD_CLR((a)->s_Socket,&((b)->fds_Fds))

/* SocketCreateConnectTagList() Tags */
#define SOCKETCREATECONNECT_Port   (((ULONG)(1L<<31))+0x00000001)
#define SOCKETCREATECONNECT_Listen (((ULONG)(1L<<31))+0x00000002)
#define SOCKETCREATECONNECT_Bind (((ULONG)(1L<<31))+0x00000003)

/* SocketOpenConnectTagList() Tags */
#define SOCKETOPENCONNECT_HostName (((ULONG)(1L<<31))+0x00000001)
#define SOCKETOPENCONNECT_Port     (((ULONG)(1L<<31))+0x00000002)
#define SOCKETOPENCONNECT_Wait     (((ULONG)(1L<<31))+0x00000003)
#define SOCKETOPENCONNECT_Timeout  (((ULONG)(1L<<31))+0x00000004)
#define SOCKETOPENCONNECT_Block    (((ULONG)(1L<<31))+0x00000005)

/* SocketSelectTagList() Tags */
#define SOCKETTAG_DONE   0
#define SOCKETWAIT_Read       (((ULONG)(1L<<31))+0x00000001)
#define SOCKETWAIT_Write      (((ULONG)(1L<<31))+0x00000002)
#define SOCKETWAIT_Except     (((ULONG)(1L<<31))+0x00000003)
#define SOCKETWAIT_TimeOutSec (((ULONG)(1L<<31))+0x00000004)
#define SOCKETWAIT_TimeOutMic (((ULONG)(1L<<31))+0x00000005)
#define SOCKETWAIT_Wait       (((ULONG)(1L<<31))+0x00000006)

#ifdef SOCKET_AMIGA
	#define SOCKETCMD_OPEN   1
	#define SOCKETCMD_CLOSE  2
	#define SOCKETCMD_DNS    3
	#define SOCKETERR_OK     1
	#define SOCKETERR_FAILED 2
	struct MessageSocket
	{
		struct Message ms_Message;
		ULONG ms_Command;
		struct Socket *ms_Socket;
		ULONG ms_Error;
	};
#endif

/* Some structures */
struct Socket
{
	SOCKET s_Socket;
	UBYTE *s_IP;
	UWORD s_Port;
	struct sockaddr_in s_SockAddrIn;
	socklen_t s_SockAddrInLen;
#ifdef SOCKET_AMIGA
	/* For async connection */
	struct MsgPort *s_MsgPort;
	struct MessageSocket s_MessageSocket;
#endif
};

/* Some functions compatibilities */
#define SocketCreateConnect(a,b) SocketCreateConnectTags(SOCKETCREATECONNECT_Port,(LONG)(a),SOCKETCREATECONNECT_Listen,(LONG)b,SOCKETTAG_DONE)
#define SocketOpenConnect(a,b) SocketOpenConnectTags(SOCKETOPENCONNECT_HostName,(LONG)(a),SOCKETOPENCONNECT_Port,(LONG)(b),SOCKETTAG_DONE)
#define SocketOpenConnectTimeout(a,b,c) SocketOpenConnectTags(SOCKETOPENCONNECT_HostName,(LONG)(a),SOCKETOPENCONNECT_Port,(LONG)(b),SOCKETOPENCONNECT_Timeout,(LONG)(c),SOCKETTAG_DONE)

/* Some prototypes */
extern VOID SocketBZero(UBYTE *,LONG);
extern BOOL SocketOpenLibrary(VOID);
extern VOID SocketCloseLibrary(VOID);

/* Some specials compilation functions */
#if !defined(SOCKET_MORPHOS) && !defined(SOCKET_AMIGAOS4) && !defined(SOCKET_UNIX)
	extern struct Socket *SocketCreateConnectTags(LONG,...);
	extern struct Socket *SocketOpenConnectTags(LONG,...);
	extern LONG SocketWaitTags(LONG,...);
#else
	#define SocketCreateConnectTags(args...) (struct Socket *)({unsigned long myargs[]={args}; SocketCreateConnectTagList((long *)myargs);})
	#define SocketOpenConnectTags(args...) (struct Socket *)({unsigned long myargs[]={args}; SocketOpenConnectTagList((long *)myargs);})
	#define SocketWaitTags(args...) (LONG)({unsigned long myargs[]={args}; SocketWaitTagList((long *)myargs);})
#endif

/* Experimental Code **************************************************************************/
#ifdef SOCKET_AMIGA
	extern struct Socket *SocketOpenConnect2TagList(LONG *);
	extern struct Socket *SocketOpenConnect2Tags(LONG,...);
	extern VOID SocketCloseConnect2(struct Socket *);
#endif
/* End of Experimental Code *******************************************************************/

extern int SocketSetBlock(struct Socket *,long);
extern struct Socket *SocketCreateConnectTagList(LONG *);
extern VOID SocketDeleteConnect(struct Socket *);
extern struct Socket *SocketOpenConnectTagList(LONG *);
extern VOID SocketCloseConnect(struct Socket *);

extern LONG SocketListen(struct Socket *,LONG);
extern struct Socket *SocketAcceptConnect(struct Socket *);
extern LONG SocketWaitTagList(LONG *);
extern LONG SocketRead(struct Socket *,VOID *,LONG);
extern LONG SocketWrite(struct Socket *,VOID *,LONG);

#ifdef __cplusplus
}
#endif
#endif /* SOCKET_H */
