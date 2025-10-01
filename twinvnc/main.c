#include "global.h"
#include "system.h"
#include "main.h"
#include "general.h"
#include "client.h"
#include "server.h"
#include "input.h"
#include "config.h"
#include "display.h"
#include "toolbar.h"
#include "twin.h"
#include "util.h"
#include "decodercursor.h"
#include "decoderhextile.h"
#include "decoderzlibraw.h"
#include "decoderzrle.h"
#include "decodertight.h"
#include "twinvnc_const.h"
#include "windowconnection.h"
#include "windowstatus.h"
#include "windowoptions.h"
#include "AppIcon.h"


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    17-01-2016 (Seg)    Modifications suite au changement de l'API decoding
    16-01-2016 (Seg)    Modification suite à la refonte de display.c
    25-02-2007 (Seg)    Remaniement de code pour l'écriture de la config et l'affichage des boîtes
    25-12-2006 (Seg)    Correction d'un problème de Depth en plein écran au démarrage de TwinVNC
    19-11-2006 (Seg)    Ajout de l'option NOREPORTMOUSE
    29-08-2005 (Seg)    Suppression des routines de gestion de config
    23-05-2005 (Seg)    Ajout d'un bout de code pour l'AppIcon d'iconification
    21-05-2005 (Seg)    Gestion du centrage de la fenêtre d'affichage
    14-05-2005 (Seg)    Remaniement du code et ajout des boîtes de connexion et de statut
    28-02-2005 (Seg)    Ajout du mode ICONIFY pour démarrer TwinVNC en mode iconifié
    23-02-2005 (Seg)    Gestion du Clipboard
    29-03-2004 (Seg)    Début du projet TwinVNC
*/


char Version[] = "\0$VER:"__APPNAME__" v"__CURRENTVERSION__" ["__OSNAME__"] by Seg (c) Dimension & Dpt22 ("__CURRENTDATE__")";


struct TwinClipHookData
{
    struct Task *TaskPtr;
    BYTE SigBit;
};


static long err_number=0;

/***** Prototypes */
M_HOOK_PROTO(ClipboardHook, struct Hook *, void *, struct ClipHookMsg *);
BOOL OpenDecoder(struct Twin *);
void CloseDecoder(struct Twin *);
LONG CheckNewProtocolSettings(struct Twin *, struct ProtocolPrefs *);
LONG DisplayTwinMessage(struct Twin *, LONG);
LONG ManageWindowConnection(struct Twin *);
LONG RefreshFrameBuffer(struct Twin *);
void TwinVNCLoop(struct Twin *, BOOL);
LONG TwinVNCPhase1Connection(struct Twin *);
LONG TwinVNCPhase2Display(struct Twin *);


BOOL (*FuncOpenDecoderPtr[])(struct Twin *)=
{
    &OpenDecoderCursor,
    &OpenDecoderHextile,
    &OpenDecoderZLibRaw,
    &OpenDecoderZRle,
    &OpenDecoderTight,
    NULL
};

void (*FuncCloseDecoderPtr[])(struct Twin *)=
{
    &CloseDecoderCursor,
    &CloseDecoderHextile,
    &CloseDecoderZLibRaw,
    &CloseDecoderZRle,
    &CloseDecoderTight,
    NULL
};



/*****
    Hook pour la gestion du clipboard
*****/

M_HOOK_FUNC(ClipboardHook, struct Hook *h, void *o, struct ClipHookMsg *msg)
{
    if(msg->chm_ChangeCmd==CMD_UPDATE)
    {
        struct TwinClipHookData *TCHDataPtr=(struct TwinClipHookData *)h->h_Data;
        Signal(TCHDataPtr->TaskPtr,1UL<<TCHDataPtr->SigBit);
    }

    return 0;
}


/*****
    Ouverture de tous les décodeurs (Tight, ZRle, etc...)
*****/

BOOL OpenDecoder(struct Twin *Twin)
{
    BOOL IsSuccess=TRUE;
    BOOL (**Ptr)(struct Twin *)=FuncOpenDecoderPtr;

    while((*Ptr)!=NULL)
    {
        if(!(*Ptr)(Twin)) IsSuccess=FALSE;
        Ptr++;
    }

    if(!IsSuccess) CloseDecoder(Twin);

    return IsSuccess;
}


/*****
    Fermeture de tous les décodeurs
*****/

void CloseDecoder(struct Twin *Twin)
{
    void (**Ptr)(struct Twin *)=FuncCloseDecoderPtr;

    while((*Ptr)!=NULL)
    {
        (*Ptr)(Twin);
        Ptr++;
    }
}


/*****************************************************************************/
/*****************************************************************************/
/*****                                                                   *****/
/*****  SOUS ROUTINES                                                    *****/
/*****                                                                   *****/
/*****************************************************************************/
/*****************************************************************************/

/*****
    Affichage d'un message en fonction de l'ID de résultat des opérations socket
*****/

LONG DisplayTwinMessage(struct Twin *Twin, LONG ResultID)
{
    LONG Result=ResultID;
    const char *Msg=NULL;

    switch(err_number)
    {
        case 4:
            ResultID=RESULT_EXIT;
            break;

        case 0:
        case 22:
            //Deconnexion normale
            break;
    }

    switch(ResultID)
    {
        case RESULT_DISCONNECTED:
            Msg=Sys_GetString(LOC_MSG_DISCONNECTED,"Disconnected");
            break;

        case RESULT_SOCKET_ERROR:
            Msg=Sys_GetString(LOC_MSG_SOCKET_ERROR,"Network error");
            break;

        case RESULT_PROTOCOL_NEGOTIATION_FAILED:
            Msg=Sys_GetString(LOC_MSG_PROTOCOL_NEGOTIATION_FAILED,"Protocol negotiation failed");
            break;

        case RESULT_AUTHENTIFICATION_INVALID:
            Msg=Sys_GetString(LOC_MSG_INVALID_AUTHENTIFICATION,"Invalid authentification");
            break;

        case RESULT_AUTHENTIFICATION_FAILED:
            Msg=Sys_GetString(LOC_MSG_AUTHENTIFICATION_FAILED,"Authentification failed");
            break;

        case RESULT_AUTHENTIFICATION_TOOMANY:
            Msg=Sys_GetString(LOC_MSG_AUTHENTIFICATION_TOOMANY,"Too many users connected");
            break;

        case RESULT_CORRUPTED_DATA:
            Msg=Sys_GetString(LOC_MSG_CORRUPTED_DATA,"Corrupted data");
            break;

        case RESULT_NOT_ENOUGH_MEMORY:
            Msg=Sys_GetString(LOC_MSG_NOT_ENOUGH_MEMORY,"Not enough memory");
            break;

        case RESULT_CONNECTION_ERROR:
            Msg=Sys_GetString(LOC_MSG_CONNECTION_ERROR,"Unknown host server");
            break;

        case RESULT_NO_BSDSOCKET_LIBRARY:
            Msg=Sys_GetString(LOC_MSG_NO_BSDSOCKET_LIBRARY,"TCP/IP stack not found");
            break;

        case RESULT_BAD_SCREEN_MODEID:
            Msg=Sys_GetString(LOC_MSG_BAD_SCREEN_MODEID,"Bad screen mode");
            break;

        case RESULT_BAD_SCREEN_DEPTH:
            Msg=Sys_GetString(LOC_MSG_BAD_SCREEN_DEPTH,"Bad screen depth");
            break;

        case RESULT_CANT_OPEN_SCREEN:
            Msg=Sys_GetString(LOC_MSG_CANT_OPEN_SCREEN,"Cannot open screen");
            break;

        case RESULT_NOT_ENOUGH_VIDEO_MEMORY:
            Msg=Sys_GetString(LOC_MSG_NOT_ENOUGH_VIDEO_MEMORY,"Not enough video memory");
            break;

        case RESULT_TIMEOUT:
            Msg=Sys_GetString(LOC_MSG_TIMEOUT,"Connection time out");
            break;

        case RESULT_EXIT:
            Result=RESULT_EXIT;
            break;

        case RESULT_RECONNECT:
            Result=RESULT_SUCCESS;
            break;

        default:
            Sys_Printf("default:%ld\n",ResultID);
            Sys_Printf("Errno=%ld\n",err_number);
            Result=RESULT_EXIT;
            break;
    }

    if(Msg!=NULL)
    {
        static char Tmp[1024];
        struct EasyStruct ReqMsg=
        {
            sizeof (struct EasyStruct),
            0,
            NULL,
            NULL,
            "Ok",
        };

        ReqMsg.es_Title=(char *)Sys_GetString(LOC_MSG_TITLE,"Error");
        ReqMsg.es_TextFormat=(char *)Msg;
        if(Sys_StrLen(Twin->Connect.Reason)>0)
        {
            Sys_SPrintf(Tmp,"%s\nReason=%s",Msg,Twin->Connect.Reason);
            ReqMsg.es_TextFormat=Tmp;
        }
        EasyRequest(NULL,&ReqMsg,NULL,TAG_DONE);
    }

    return Result;
}


/*****
    Gestion de la fenêtre de connexion
*****/

LONG ManageWindowConnection(struct Twin *Twin)
{
    LONG Result=RESULT_NOT_ENOUGH_MEMORY;

    /* Ouverture de la boîte de connexion */
    if(Gal_EasyOpenWindowConnection(Twin))
    {
        struct WindowConnection *wc=&Twin->WindowConnection;
        BOOL IsExit=FALSE;

        Result=RESULT_SUCCESS;
        while(!IsExit && Result>0)
        {
            WaitPort(Twin->MainMPort);

            Twin->IsCheckProtocolPrefs=FALSE;
            Twin->WindowConnectionResult=WIN_CONNECTION_RESULT_NONE;
            Result=Gal_ManageMessagePort(Twin,1UL<<Twin->MainMPort->mp_SigBit);

            /* Traitement du résultat de la fenêtre de connexion */
            switch(Twin->WindowConnectionResult)
            {
                case WIN_CONNECTION_RESULT_CONNECT:
                    Win_SaveHostsHistory(wc);
                    IsExit=TRUE;
                    break;

                case WIN_CONNECTION_RESULT_QUIT:
                    Result=RESULT_EXIT;
                    IsExit=TRUE;
                    break;
            }

            if(Twin->IsCheckProtocolPrefs)
            {
                Twin->RemoteProtocolPrefs=Twin->NewProtocolPrefs;
            }
        }

        Sys_StrCopy(Twin->Host,Win_GetWindowConnectionHost(wc,&Twin->Port),SIZEOF_HOST_BUFFER);
        Sys_StrCopy(Twin->Password,Win_GetWindowConnectionPassword(wc),SIZEOF_PASSWORD_BUFFER);

        Win_CloseWindowConnection(wc);
    }

    return Result;
}


/*****************************************************************************/
/*****************************************************************************/
/*****                                                                   *****/
/*****  GESTION DES PREFS                                                *****/
/*****                                                                   *****/
/*****************************************************************************/
/*****************************************************************************/

/*****
    Vérification des changements dans la config du protocole
*****/

LONG CheckNewProtocolSettings(struct Twin *Twin, struct ProtocolPrefs *NewPrefs)
{
    LONG Result=RESULT_SUCCESS;
    struct ProtocolPrefs *pp=&Twin->RemoteProtocolPrefs;
    struct Display *disp=&Twin->Display;
    BOOL IsForceSetEncodings=FALSE;

    /* Choix du facteur de scale serveur */
    if(NewPrefs->ServerScale!=pp->ServerScale)
    {
        Result=SetScale(Twin,NewPrefs->ServerScale);
    }

    /* Choix du pixel format */
    if(Result>0)
    {
        const struct PixelFormat *pixf=Gal_GetPixelFormatFromDepth(Twin,NewPrefs->Depth);
        struct Connect *co=&Twin->Connect;
        struct FrameBuffer *fb=&disp->FrameBuffer;
        BOOL IsRethinkViewType=FALSE;

        if(NewPrefs->Depth!=pp->Depth)
        {
            IsForceSetEncodings=TRUE;
            Disp_ClearCursor(disp);
            CloseDecoderCursor(Twin);

            if((Result=SetClientPixelFormat(Twin,pixf))>0)
            {
                Frb_CalcColorMapTable(fb,pixf);
                Twin->IsRefreshIncremental=FALSE;
                IsRethinkViewType=TRUE;
            }
        }

        if(Result>0 &&
          (fb->Width!=co->ServerWidth ||
           fb->Height!=co->ServerHeight ||
           NewPrefs->Depth!=pp->Depth))
        {
            Twin->IsRefreshIncremental=FALSE;
            if(!Frb_Prepare(fb,pixf,co->ServerWidth,co->ServerHeight))
            {
                Result=RESULT_NOT_ENOUGH_MEMORY;
            }
        }

        /* On rafraîchit après le PrepareFrameBuffer() */
        if(IsRethinkViewType)
        {
            if(!Disp_RethinkViewType(disp,TRUE,FALSE,TRUE)) Result=RESULT_NOT_ENOUGH_MEMORY;
        }
    }

    /* Choix de la méthode d'encodage */
    if(Result>0 &&
       (IsForceSetEncodings ||
        NewPrefs->EncoderId!=pp->EncoderId ||
        NewPrefs->IsChangeZLib!=pp->IsChangeZLib || NewPrefs->ZLibLevel!=pp->ZLibLevel ||
        NewPrefs->IsUseJpeg!=pp->IsUseJpeg || NewPrefs->JpegQuality!=pp->JpegQuality ||
        NewPrefs->IsNoLocalCursor!=pp->IsNoLocalCursor ||
        NewPrefs->IsNoCopyRect!=pp->IsNoCopyRect ||
        NewPrefs->IsReportMouse!=pp->IsReportMouse))
    {
        /* Tests pour savoir si on doit forcer le refresh global */
        if(NewPrefs->IsUseJpeg!=pp->IsUseJpeg ||
           (NewPrefs->IsUseJpeg && NewPrefs->JpegQuality!=pp->JpegQuality) ||
           NewPrefs->IsNoLocalCursor!=pp->IsNoLocalCursor)
        {
            Twin->IsRefreshIncremental=FALSE;
        }

        if(NewPrefs->IsNoLocalCursor)
        {
            Disp_ClearCursor(disp);
            CloseDecoderCursor(Twin);
        }

        Result=SetEncodings(Twin,NewPrefs);
    }

    *pp=*NewPrefs;
    Disp_SetViewTitle(disp,Gal_MakeDisplayTitle(Twin));

    return Result;
}


/*****************************************************************************/
/*****************************************************************************/
/*****                                                                   *****/
/*****  ROUTINES PRINCIPALES DE GESTION DE TWINVNC                       *****/
/*****                                                                   *****/
/*****************************************************************************/
/*****************************************************************************/

/*****
    Gestion de l'application
*****/

void TwinVNCLoop(struct Twin *Twin, BOOL IsAutomaticConnection)
{
    LONG Result=RESULT_SUCCESS;

    Gal_InitDisplay(Twin);
    while(Result>0)
    {
        Twin->NewGlobalPrefs=Twin->CurrentGlobalPrefs;
        Twin->NewProtocolPrefs=Twin->RemoteProtocolPrefs;

        if(!IsAutomaticConnection)
        {
            /* Gestion de la boîte de connexion */
            Result=ManageWindowConnection(Twin);
        }

        if(Result>0)
        {
            /* Connexion au serveur */
            Result=TwinVNCPhase1Connection(Twin);

            if(Result<=0)
            {
                Result=DisplayTwinMessage(Twin,Result);
            }
        }

        IsAutomaticConnection=FALSE;
    }
}


/*****
    Première phase de démarrage de TwinVNC: La connexion au serveur
*****/

LONG TwinVNCPhase1Connection(struct Twin *Twin)
{
    LONG Result=RESULT_SUCCESS;
    struct GlobalPrefs *gp=&Twin->CurrentGlobalPrefs;

    /* On affiche la boîte de statut si l'iconification au démarrage n'est pas activée */
    if(!gp->IsStartIconify)
    {
        if(!Gal_EasyOpenWindowStatus(Twin)) Result=RESULT_NOT_ENOUGH_MEMORY;
    }

    if(Result>0)
    {
        struct WindowStatus *ws=&Twin->WindowStatus;

        Win_SetWindowStatusMessage(ws,"Open TCP/IP stack...",NULL);

        Result=RESULT_NO_BSDSOCKET_LIBRARY;
        if(SocketOpenLibrary())
        {
            Sys_SPrintf(Twin->Tmp,"%s:%ld",Twin->Host,Twin->Port);
            Win_SetWindowStatusMessage(ws,Sys_GetString(LOC_STATUS_TEXT_CONNECTION,"Connect to Host:"),Twin->Tmp);

            Result=RESULT_CONNECTION_ERROR;
            if((Twin->Connect.Socket=SocketOpenConnectTimeout(Twin->Host,(unsigned short)Twin->Port,0))!=NULL)
            {
                ULONG Clock=Sys_GetClock()/CLOCKS_PER_SEC;

                /* On attend que la connexion soit effective ou annulée */
                Result=RESULT_SUCCESS;
                while(Result>0)
                {
                    ULONG SigMask=1UL<<Twin->MainMPort->mp_SigBit;
                    SOCKET_FDZERO(&Twin->Connect.FDSSignal);
                    SOCKET_FDSET(Twin->Connect.Socket,&Twin->Connect.FDSSignal);
                    if((Result=SocketWaitTags(
                        SOCKETWAIT_Read,(ULONG)&Twin->Connect.FDSSignal,
                        SOCKETWAIT_Wait,(ULONG)&SigMask,
                        SOCKETWAIT_TimeOutSec,1,
                        TAG_DONE))>=0)
                    {
                        if(SOCKET_FDISSET(Twin->Connect.Socket,&Twin->Connect.FDSSignal)) break;
                        if(Sys_GetClock()/CLOCKS_PER_SEC-Clock>=30) Result=RESULT_TIMEOUT;
                        else Result=Gal_ManageMessagePort(Twin,SigMask);
                    }
                }

                /* On continue si la connexion est ok */
                if(Result>0)
                {
                    long NoDelay=-1;

                    /* Ajout des options socket */
                    SocketSetBlock(Twin->Connect.Socket,1);
                    setsockopt(Twin->Connect.Socket->s_Socket,IPPROTO_TCP,TCP_NODELAY,&NoDelay,sizeof(NoDelay));

                    /* Début de gestion du protocole RFB */
                    Result=RESULT_NOT_ENOUGH_MEMORY;
                    if(OpenDecoder(Twin))
                    {
                        if((Result=Conn_ContactServer(Twin,Win_SetWindowStatusMessage))>0)
                        {
                            /* On ferme la fenêtre de statut */
                            Win_CloseWindowStatus(ws);

                            /* Début de la phase 2 */
                            Result=TwinVNCPhase2Display(Twin);
                        }

                        CloseDecoder(Twin);
                    }
                }

                err_number=Errno();
                SocketCloseConnect(Twin->Connect.Socket);
            }

            SocketCloseLibrary();
        }

        Win_CloseWindowStatus(ws);
    }

    return Result;
}


/*****
    Deuxième phase de démarrage de TwinVNC: L'affichage
*****/

LONG TwinVNCPhase2Display(struct Twin *Twin)
{
    LONG Result;
    struct GlobalPrefs *gp=&Twin->CurrentGlobalPrefs;
    struct ProtocolPrefs *pp=&Twin->CurrentProtocolPrefs;
    const struct PixelFormat *pixf=Gal_GetPixelFormatFromDepth(Twin,pp->Depth);
    struct Display *disp=&Twin->Display;

    /* Mise à jour de la table des palettes avec le PixelFormat courant */
    Frb_CalcColorMapTable(&disp->FrameBuffer,pixf);

    /* Initialisation des préférences */
    Twin->NewGlobalPrefs=*gp;
    Cfg_InitDefaultProtocolPrefs(&Twin->RemoteProtocolPrefs);
    if(!gp->ScrDepth) gp->ScrDepth=(ULONG)pixf->Depth;
    Inp_Init(&Twin->Inputs);

    /* Initialisation des paramètres VNC pour l'affichage */
    Twin->IsCheckProtocolPrefs=FALSE;
    Twin->IsDisplayChanged=TRUE;
    Twin->WaitRefreshPriority=0;

    if((Result=CheckNewProtocolSettings(Twin,pp))>0)
    {
        if(gp->IsStartIconify) Result=Gal_IconifyApplication(Twin);
        else Result=Gal_OpenDisplay(Twin);
    }

    while(Result>0)
    {
        if(Twin->IsCheckProtocolPrefs && Twin->WaitRefreshPriority<=0)
        {
            Twin->IsCheckProtocolPrefs=FALSE;
            Result=CheckNewProtocolSettings(Twin,pp);
        }

        if(Result>0)
        {
            /* Rafraîchissement de l'écran */
            if((Result=RefreshFrameBuffer(Twin))>0)
            {
                ULONG SigMask;

                if((Result=Gal_WaitEvent(Twin,&SigMask))>0)
                {
                    /* Gestion des messages entrants/sortants */
                    if(Result==RESULT_EVENT_SERVER) Result=Srv_ManageServerMessage(Twin);
                    else Result=Gal_ManageMessagePort(Twin,SigMask);
                }
            }
        }
    }

    Disp_CloseDisplay(disp);
    Frb_Flush(&disp->FrameBuffer);
    Util_FlushDecodeBuffers(Twin,TRUE);

    return Result;
}


/*****
    Rafraîchissement de l'affichage
*****/

LONG RefreshFrameBuffer(struct Twin *Twin)
{
    LONG Result=RESULT_SUCCESS;

    if(Twin->IsRefreshIncremental)
    {
        if(Twin->WaitRefreshPriority<1 && Twin->CurrentGlobalPrefs.DisplayOption!=DISPLAY_OPTION_NODISPLAY)
        {
            Result=SetFrameBufferUpdateRequest(Twin,TRUE);
            Twin->WaitRefreshPriority=1;
        }
    }
    else
    {
        if(Twin->WaitRefreshPriority<2)
        {
            Result=SetFrameBufferUpdateRequest(Twin,FALSE);
            Twin->IsRefreshIncremental=TRUE;
            Twin->WaitRefreshPriority=2;
        }
    }

    return Result;
}


/*****************************************************************************/
/*****************************************************************************/
/*****                                                                   *****/
/*****  MAIN                                                             *****/
/*****                                                                   *****/
/*****************************************************************************/
/*****************************************************************************/

int main(int argc, char **argv)
{
    /* Ouverture des librairies amiga */
    if(Sys_OpenAllLibs())
    {
        struct Twin *Twin=(struct Twin *)Sys_AllocMem(sizeof(struct Twin));

        if(Twin!=NULL)
        {
            CopyMem((APTR)&AppIconDObj,(APTR)&Twin->DefaultDiskObject,sizeof(Twin->DefaultDiskObject));

            /* Création des classes boopsi nécessaires pour TwinVNC */
            if(Gui_CreateClasses(ToolBarClassList))
            {
                /* Création du port message des fenêtres */
                if((Twin->MainMPort=CreateMsgPort())!=NULL)
                {
                    /* Création du port message de l'APP Icon */
                    if((Twin->IconMPort=CreateMsgPort())!=NULL)
                    {
                        /* Allocation d'un signal pour le Copier/Coller */
                        if((Twin->ClipboardSignal=AllocSignal(-1))!=-1)
                        {
                            struct TwinClipHookData TCHData;

                            TCHData.TaskPtr=FindTask(NULL);
                            TCHData.SigBit=Twin->ClipboardSignal;

                            /* Ouverture du clipboard.device pour la gestion du Copier/Coller */
                            if(Sys_OpenClipboard((HOOKFUNC)&ClipboardHook,(void *)&TCHData))
                            {
                                /* Allocations de ressources spécifiques */
                                if(Gal_AllocAppResources(Twin))
                                {
                                    struct GlobalPrefs *gp=&Twin->CurrentGlobalPrefs;
                                    struct ProtocolPrefs *pp=&Twin->CurrentProtocolPrefs;
                                    BOOL IsHost=FALSE,IsPassword=FALSE;
                                    LONG Error;

                                    /* Lecture de la config */
                                    Cfg_Init(gp,pp,Twin->Host,&Twin->Port,Twin->Password);
                                    Error=Cfg_ReadConfig(gp,pp,(void *)argv,&Twin->AppPath,Twin->Host,&Twin->Port,Twin->Password,&IsHost,&IsPassword);

                                    if(!Error)
                                    {
                                        Gui_ChangeFontDef(&Twin->Font,gp->FontName,gp->FontSize);
                                        Gui_InitCompleteWindow(&Twin->WindowConnection.Window);
                                        Gui_InitCompleteWindow(&Twin->WindowStatus.Window);
                                        Gui_InitCompleteWindow(&Twin->WindowInformation.Window);
                                        Gui_InitCompleteWindow(&Twin->WindowOptions.Window);
                                        Gui_InitCompleteWindow(&Twin->WindowFileTransfer.Window);
                                        Twin->WindowOptions.Page=0;

                                        /* Lancement de TwinVNC */
                                        TwinVNCLoop(Twin,(!IsHost || !IsPassword?FALSE:TRUE));
                                    }
                                    else
                                    {
                                        if(Error>0) PrintFault(Error,NULL);
                                    }

                                    /* Libération potentielle du buffer du chemin complet de l'application */
                                    if(Twin->AppPath!=NULL) Sys_FreeMem((void *)Twin->AppPath);

                                    /* Nettoyage des ressources éventuellement allouées entre temps */
                                    Gal_FreeAllAppResources(Twin);
                                }

                                Sys_CloseClipboard();
                            }

                            FreeSignal(Twin->ClipboardSignal);
                        }

                        DeleteMsgPort(Twin->IconMPort);
                    }

                    DeleteMsgPort(Twin->MainMPort);
                }

                Gui_DeleteClasses(ToolBarClassList);
            }

            Sys_FreeMem((void *)Twin);
        }

        Sys_CloseAllLibs();
    }

    return 0;
}
