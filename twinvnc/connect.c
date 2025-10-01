#include "system.h"
#include "connect.h"
#include "auth.h"
#include "general.h"
#include "twin.h"
#include "twinvnc_const.h"
#include <stdio.h>


/*
    29-03-2016 (Seg)    Début ré-intégration transfert de fichier tight 1.x
    20-02-2016 (Seg)    Gestion du protocole Tight + Mise à la norme du code
    04-04-2004 (Seg)    Procédures de connection au serveur VNC
*/


/***** Prototypes */
LONG Conn_ContactServer(struct Twin *, void (*)(struct WindowStatus *, const char *, const char *));
LONG Conn_NegociateProtocol(struct Twin *);
LONG Conn_SetClientInitialisation(struct Twin *);
LONG Conn_WaitServerInitialisation(struct Twin *);
LONG Conn_ReadTightCapabilities(struct Twin *, LONG, void *, void (*)(void *, LONG, const struct TightCapability *));

void Conn_ReadTightCapServer(void *, LONG, const struct TightCapability *);
void Conn_ReadTightCapClient(void *, LONG, const struct TightCapability *);
void Conn_ReadTightCapEncoding(void *, LONG, const struct TightCapability *);


/*****
    Ouverture d'une connexion sur un serveur VNC
*****/

LONG Conn_ContactServer(struct Twin *Twin, void (*StatusFuncPtr)(struct WindowStatus *, const char *, const char *))
{
    /* Négociation de protocole entre le client et le serveur */
    LONG Result;

    StatusFuncPtr(&Twin->WindowStatus,Sys_GetString(LOC_STATUS_TEXT_NEGOTIATION,"Protocol negotiation"),NULL);
    Result=Conn_NegociateProtocol(Twin);

    if(Result>0)
    {
        /* Authentification du client */
        StatusFuncPtr(&Twin->WindowStatus,Sys_GetString(LOC_STATUS_TEXT_AUTHENTIFICATION,"Authentification/Password"),NULL);
        Result=Auth_PerformAuthentification(Twin);

        if(Result>0)
        {
            /* Initialisation du client */
            StatusFuncPtr(&Twin->WindowStatus,Sys_GetString(LOC_STATUS_TEXT_CLIENT_INIT,"Client initialisation"),NULL);
            Result=Conn_SetClientInitialisation(Twin);
            if(Result>0)
            {
                /* Réception des informations sur le FrameBuffer */
                StatusFuncPtr(&Twin->WindowStatus,Sys_GetString(LOC_STATUS_TEXT_WAIT_SERVER_INIT,"Waiting server initialisation"),NULL);
                Result=Conn_WaitServerInitialisation(Twin);
            }
        }
    }

    return Result;
}


/*****
    Négociation de protocole entre le client et le serveur
*****/

LONG Conn_NegociateProtocol(struct Twin *Twin)
{
    /* Lecture de la signature */
    char Header[RFB_PROTOCOLVERSION_SIZEOF+1];
    LONG Result=Gal_WaitRead(Twin,(void *)Header,RFB_PROTOCOLVERSION_SIZEOF);

    if(Result>0)
    {
        ULONG Version=0, Revision=0;

        Header[Result]=0;

        if(sscanf(Header,RFB_PROTOCOLVERSION_FORMAT,&Version,&Revision)==2)
        {
            ULONG VerRev=(Version<<16)+Revision;
            struct Connect *co=&Twin->Connect;

            /* Négociation de protocole entre le client et le serveur */
            co->ServerVersion=Version;
            co->ServerRevision=Revision;

            if(VerRev>((RFB_PROTOCOL_VERSION<<16)+RFB_PROTOCOL_REVISION))
            {
                co->ServerVersion=RFB_PROTOCOL_VERSION;
                co->ServerRevision=RFB_PROTOCOL_REVISION;
            }

            if(VerRev<((RFB_PROTOCOL_MIN_VERSION<<16)+RFB_PROTOCOL_MIN_REVISION))
            {
                co->ServerVersion=RFB_PROTOCOL_MIN_VERSION;
                co->ServerRevision=RFB_PROTOCOL_MIN_REVISION;
            }

            /* Envoi du protocole supporté par le client */
            Sys_SPrintf(Header,RFB_PROTOCOLVERSION_FORMAT,co->ServerVersion,co->ServerRevision);
            Result=Gal_WaitWrite(Twin,(void *)Header,RFB_PROTOCOLVERSION_SIZEOF);
            if(Result==0) Result=RESULT_PROTOCOL_NEGOTIATION_FAILED;
        }
    }

    return Result;
}


/*****
    Initialisation du client
*****/

LONG Conn_SetClientInitialisation(struct Twin *Twin)
{
    struct ClientInitialisation Msg;

    Msg.Shared=Twin->CurrentProtocolPrefs.IsShared;

    return Gal_WaitWrite(Twin,(void *)&Msg,sizeof(Msg));
}


/*****
    Permet d'obtenir les informations sur le FrameBuffer
*****/

LONG Conn_WaitServerInitialisation(struct Twin *Twin)
{
    struct ServerInitialisation Msg;
    LONG Result=Gal_WaitRead(Twin,(void *)&Msg,sizeof(Msg));

    if(Result>0)
    {
        struct Connect *co=&Twin->Connect;

        co->ServerWidth=(ULONG)Msg.FrameBufferWidth;
        co->ServerHeight=(ULONG)Msg.FrameBufferHeight;
        co->ServerPixelFormat=Msg.PixelFormat;

        Result=Gal_WaitFixedRead(Twin,co->ServerName,sizeof(co->ServerName)-1,Msg.NameLength);
        if(Result>0)
        {
            co->ServerName[Result]=0;
            /*
            Sys_Printf("FrameBuffer: %ldx%ld (%s)\n",(ULONG)Msg.FrameBufferWidth,(ULONG)Msg.FrameBufferHeight,co->ServerName);
            Sys_Printf("FrameBuffer: BitPerPixel = %ld\n",(ULONG)Msg.PixelFormat.BitPerPixel);
            Sys_Printf("FrameBuffer: Depth       = %ld\n",(ULONG)Msg.PixelFormat.Depth);
            Sys_Printf("FrameBuffer: BigEndian   = %ld\n",(ULONG)Msg.PixelFormat.BigEndian);
            Sys_Printf("FrameBuffer: TrueColor   = %ld\n",(ULONG)Msg.PixelFormat.TrueColor);
            Sys_Printf("FrameBuffer: RedMax      = %ld\n",(ULONG)Msg.PixelFormat.RedMax);
            Sys_Printf("FrameBuffer: GreenMax    = %ld\n",(ULONG)Msg.PixelFormat.GreenMax);
            Sys_Printf("FrameBuffer: BlueMax     = %ld\n",(ULONG)Msg.PixelFormat.BlueMax);
            Sys_Printf("FrameBuffer: RedShift    = %ld\n",(ULONG)Msg.PixelFormat.RedShift);
            Sys_Printf("FrameBuffer: GreenShift  = %ld\n",(ULONG)Msg.PixelFormat.GreenShift);
            Sys_Printf("FrameBuffer: BlueShift   = %ld\n",(ULONG)Msg.PixelFormat.BlueShift);
            */

            /* Si la sécurité choisie est Tight, alors il faut lire les données ci-après */
            if(co->Security==RFB_SECURITYTYPE_TIGHT)
            {
                struct ServerInteractionCaps Msg;

                Result=Gal_WaitRead(Twin,(void *)&Msg,sizeof(Msg));
                if(Result>0) Result=Conn_ReadTightCapabilities(Twin,Msg.ServerMessageTypes,(void *)&co->TightCapServer,Conn_ReadTightCapServer);
                if(Result>0) Result=Conn_ReadTightCapabilities(Twin,Msg.ClientMessageTypes,(void *)&co->TightCapClient,Conn_ReadTightCapClient);
                if(Result>0) Result=Conn_ReadTightCapabilities(Twin,Msg.EncodingTypes,NULL,Conn_ReadTightCapEncoding);
            }
        }
    }

    return Result;
}


/*****
    Lecture des options possibles du serveur
*****/

LONG Conn_ReadTightCapabilities(struct Twin *Twin, LONG Count, void *UserPtr, void (*HookPtr)(void *UserPtr, LONG Idx, const struct TightCapability *Cap))
{
    LONG Result=RESULT_SUCCESS,i;
    struct TightCapabilityMsg CapMsg;

    for(i=0; i<Count && Result>0; i++)
    {
        Result=Gal_WaitRead(Twin,(void *)&CapMsg,sizeof(CapMsg));
        if(Result>0)
        {
            struct TightCapability Cap;
            Cap.Code=CapMsg.Code;
            Sys_StrCopy(Cap.Vendor,CapMsg.Vendor,sizeof(CapMsg.Vendor));
            Sys_StrCopy(Cap.Name,CapMsg.Name,sizeof(CapMsg.Name));
            if(HookPtr!=NULL) HookPtr(UserPtr,i,&Cap);
        }
    }

    return Result;
}


/*****
    Hook pour interpréter les messages pouvant être reçus du serveur
*****/

void Conn_ReadTightCapServer(void *UserPtr, LONG Idx, const struct TightCapability *Cap)
{
    ULONG Flags=*((ULONG *)UserPtr);

    if(Cap->Code==TIGHT1_FILE_LIST_DATA) Flags|=FLG_TIGHT1_FILE_LIST_DATA;
    if(Cap->Code==TIGHT1_FILE_DOWNLOAD_DATA) Flags|=FLG_TIGHT1_DOWNLOAD_DATA;

    if(Cap->Code==TIGHT2_COMPRESSION_SUPPORT_REPLY) Flags|=FLG_TIGHT2_COMPRESSION_SUPPORT_REPLY;
    if(Cap->Code==TIGHT2_FILE_LIST_REPLY) Flags|=FLG_TIGHT2_FILE_LIST_REPLY;
    if(Cap->Code==TIGHT2_MD5_REPLY) Flags|=FLG_TIGHT2_MD5_REPLY;
    if(Cap->Code==TIGHT2_UPLOAD_START_REPLY) Flags|=FLG_TIGHT2_UPLOAD_START_REPLY;
    if(Cap->Code==TIGHT2_UPLOAD_DATA_REPLY) Flags|=FLG_TIGHT2_UPLOAD_DATA_REPLY;
    if(Cap->Code==TIGHT2_UPLOAD_END_REPLY) Flags|=FLG_TIGHT2_UPLOAD_END_REPLY;
    if(Cap->Code==TIGHT2_DOWNLOAD_START_REPLY) Flags|=FLG_TIGHT2_DOWNLOAD_START_REPLY;
    if(Cap->Code==TIGHT2_DOWNLOAD_DATA_REPLY) Flags|=FLG_TIGHT2_DOWNLOAD_DATA_REPLY;
    if(Cap->Code==TIGHT2_DOWNLOAD_END_REPLY) Flags|=FLG_TIGHT2_DOWNLOAD_END_REPLY;
    if(Cap->Code==TIGHT2_MKDIR_REPLY) Flags|=FLG_TIGHT2_MKDIR_REPLY;
    if(Cap->Code==TIGHT2_REMOVE_REPLY) Flags|=FLG_TIGHT2_REMOVE_REPLY;
    if(Cap->Code==TIGHT2_RENAME_REPLY) Flags|=FLG_TIGHT2_RENAME_REPLY;
    if(Cap->Code==TIGHT2_DIRSIZE_REPLY) Flags|=FLG_TIGHT2_DIRSIZE_REPLY;
    if(Cap->Code==TIGHT2_LAST_REQUEST_FAILED_REPLY) Flags|=FLG_TIGHT2_LAST_REQUEST_FAILED_REPLY;
    *((ULONG *)UserPtr)=Flags;

    //if(Cap->Code>-512) Sys_Printf("Code=%ld, Vendor=%s, Name=%s\n",Cap->Code,Cap->Vendor,Cap->Name);
    //else Sys_Printf("Code=%04lx, Vendor=%s, Name=%s\n",Cap->Code,Cap->Vendor,Cap->Name);
}


/*****
    Hook pour interpréter les messages pouvant être envoyés par le client
*****/

void Conn_ReadTightCapClient(void *UserPtr, LONG Idx, const struct TightCapability *Cap)
{
    ULONG Flags=*((ULONG *)UserPtr);

    if(Cap->Code==TIGHT1_FILE_LIST_REQUEST) Flags|=FLG_TIGHT1_FILE_LIST_REQUEST;
    if(Cap->Code==TIGHT1_FILE_DOWNLOAD_REQUEST) Flags|=FLG_TIGHT1_FILE_DOWNLOAD_REQUEST;
    if(Cap->Code==TIGHT1_FILE_UPLOAD_REQUEST) Flags|=FLG_TIGHT1_FILE_UPLOAD_REQUEST;

    if(Cap->Code==TIGHT2_COMPRESSION_SUPPORT_REQUEST) Flags|=FLG_TIGHT2_COMPRESSION_SUPPORT_REQUEST;
    if(Cap->Code==TIGHT2_FILE_LIST_REQUEST) Flags|=FLG_TIGHT2_FILE_LIST_REQUEST;
    if(Cap->Code==TIGHT2_MD5_REQUEST) Flags|=FLG_TIGHT2_MD5_REQUEST;
    if(Cap->Code==TIGHT2_UPLOAD_START_REQUEST) Flags|=FLG_TIGHT2_UPLOAD_START_REQUEST;
    if(Cap->Code==TIGHT2_UPLOAD_DATA_REQUEST) Flags|=FLG_TIGHT2_UPLOAD_DATA_REQUEST;
    if(Cap->Code==TIGHT2_UPLOAD_END_REQUEST) Flags|=FLG_TIGHT2_UPLOAD_END_REQUEST;
    if(Cap->Code==TIGHT2_DOWNLOAD_START_REQUEST) Flags|=FLG_TIGHT2_DOWNLOAD_START_REQUEST;
    if(Cap->Code==TIGHT2_DOWNLOAD_DATA_REQUEST) Flags|=FLG_TIGHT2_DOWNLOAD_DATA_REQUEST;
    if(Cap->Code==TIGHT2_MKDIR_REQUEST) Flags|=FLG_TIGHT2_MKDIR_REQUEST;
    if(Cap->Code==TIGHT2_REMOVE_REQUEST) Flags|=FLG_TIGHT2_REMOVE_REQUEST;
    if(Cap->Code==TIGHT2_RENAME_REQUEST) Flags|=FLG_TIGHT2_RENAME_REQUEST;
    if(Cap->Code==TIGHT2_DIRSIZE_REQUEST) Flags|=FLG_TIGHT2_DIRSIZE_REQUEST;
    *((ULONG *)UserPtr)=Flags;

    //if(Cap->Code>-512) Sys_Printf("Code=%ld, Vendor=%s, Name=%s\n",Cap->Code,Cap->Vendor,Cap->Name);
    //else Sys_Printf("Code=%04lx, Vendor=%s, Name=%s\n",Cap->Code,Cap->Vendor,Cap->Name);
}


/*****
    Hook pour interpréter les types d'encodages supportés par le serveur
*****/

void Conn_ReadTightCapEncoding(void *UserPtr, LONG Idx, const struct TightCapability *Cap)
{
    //if(Cap->Code>-512) Sys_Printf("Code=%ld, Vendor=%s, Name=%s\n",Cap->Code,Cap->Vendor,Cap->Name);
    //else Sys_Printf("Code=%04lx, Vendor=%s, Name=%s\n",Cap->Code,Cap->Vendor,Cap->Name);
}
