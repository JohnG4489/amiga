#include "global.h"
#include "system.h"
#include "general.h"
#include "twin.h"
#include "util.h"
#include "client.h"
#include "input.h"
#include "toolbar.h"
#include "config.h"
#include "twinvnc_const.h"
#include <graphics/gfxbase.h>


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    02-07-2017 (Seg)    Modif suite refonte artimétique 64 bits
    02-03-2016 (Seg)    Ajout raccourci AmigaR+K pour afficher la boîte de transfert de fichier
    29-01-2016 (Seg)    Personalisation de la touche de controle
    16-01-2016 (Seg)    Modification suite à la refonte de display.c
    01-03-2007 (Seg)    Ajout de la sauvegarde des options
    19-11-2006 (Seg)    Changement dans la gestion du mode NODISPLAY
    05-03-2006 (Seg)    Gestion du About + Ajout des fonction de gestion de la font
    18-02-2006 (Seg)    Changement de nom de la pseudo classe twin.c en general.c
                        + renommage des fonctions...
    03-07-2005 (Seg)    Ajout de la gestion des événements du port message
                        et ajout de la gestion des touches de contrôle.
    23-02-2005 (Seg)    Gestion du clipboard
    22-05-2004 (Seg)    Gestion des sockets et des messages client
*/


/* 3 bits */
const struct PixelFormat pixf3=
{
    8,  /* BitPerPixel */
    3,  /* Depth */
    1,  /* BigEndian */
    1,  /* TrueColor */
    1,  /* RedMax */
    1,  /* GreenMax */
    1,  /* BlueMax */
    2,  /* RedShift */
    1,  /* GreenShift */
    0,  /* BlueShift */
    0,0
};

/* 6 bits */
const struct PixelFormat pixf6=
{
    8,  /* BitPerPixel */
    6,  /* Depth */
    1,  /* BigEndian */
    1,  /* TrueColor */
    3,  /* RedMax */
    3,  /* GreenMax */
    3,  /* BlueMax */
    4,  /* RedShift */
    2,  /* GreenShift */
    0,  /* BlueShift */
    0,0
};

/* 8 bits */
const struct PixelFormat pixf8=
{
    8,  /* BitPerPixel */
    8,  /* Depth */
    1,  /* BigEndian */
    1,  /* TrueColor */
    7,  /* RedMax */
    7,  /* GreenMax */
    3,  /* BlueMax */
    5,  /* RedShift */
    2,  /* GreenShift */
    0,  /* BlueShift */
    0,0
};

/* 16bits */
const struct PixelFormat pixf16=
{
    16, /* BitPerPixel */
    16, /* Depth */
    1,  /* BigEndian */
    1,  /* TrueColor */
    31, /* RedMax */
    63, /* GreenMax */
    31, /* BlueMax */
    11, /* RedShift */
    5,  /* GreenShift */
    0,  /* BlueShift */
    0,0
};

/* 32 bits */
const struct PixelFormat pixf32=
{
    32, /* BitPerPixel */
    24, /* Depth */
    1,  /* BigEndian */
    1,  /* TrueColor */
    255,/* RedMax */
    255,/* GreenMax */
    255,/* BlueMax */
    16, /* RedShift */
    8,  /* GreenShift */
    0,  /* BlueShift */
    0,0
};


/***** Prototypes */
LONG Gal_WaitEvent(struct Twin *, ULONG *);
LONG Gal_WaitRead(struct Twin *, void *, LONG);
LONG Gal_WaitFixedRead(struct Twin *, void *, LONG, LONG);
//LONG Gal_WaitWrite(struct Twin *, void *, LONG);
LONG Gal_ManageMessagePort(struct Twin *, ULONG);
const struct PixelFormat *Gal_GetPixelFormatFromDepth(struct Twin *, ULONG);
struct Window *Gal_GetParentWindow(struct Twin *);
struct Screen *Gal_GetBestPublicScreen(struct Window *, BOOL *, ULONG *);
char *Gal_MakeDisplayTitle(struct Twin *);
char *Gal_MakeAppTitle(struct Twin *);
void Gal_RemoveIconApplication(struct Twin *);
LONG Gal_IconifyApplication(struct Twin *);
LONG Gal_ExpandApplication(struct Twin *);
void Gal_InitDisplay(struct Twin *);
LONG Gal_OpenDisplay(struct Twin *);
LONG Gal_ChangeDisplay(struct Twin *);
BOOL Gal_AllocAppResources(struct Twin *);
void Gal_FreeAllAppResources(struct Twin *);
BOOL Gal_GetScreenModeID(BOOL, ULONG *, ULONG *, LONG, LONG, BOOL *);
void Gal_HideOpennedWindow(struct Twin *, struct Screen *, ULONG);
BOOL Gal_ShowHiddenWindow(struct Twin *, ULONG);
BOOL Gal_OpenWindowConnection(struct Twin *);
BOOL Gal_EasyOpenWindowStatus(struct Twin *);
BOOL Gal_EasyOpenWindowInformation(struct Twin *);
BOOL Gal_EasyOpenWindowOptions(struct Twin *);
BOOL Gal_EasyOpenWindowFileTransfer(struct Twin *);

LONG P_Gal_SaveConfigAs(struct Twin *, struct GlobalPrefs *, struct ProtocolPrefs *);
LONG P_Gal_CheckNewGlobalPrefs(struct Twin *, struct GlobalPrefs *);
BOOL P_Gal_HookFuncOpenDisplay(void *);
void P_Gal_HookFuncCloseDisplay(void *);
LONG P_Gal_CheckControlKeys(struct Twin *, struct IntuiMessage *);
LONG P_Gal_ExecuteControlOp(struct Twin *, LONG);
LONG P_Gal_ConvertScreenErrorCode(ULONG);
const char *P_Gal_FindFileExtension(const char *);
//void WriteLog(const char *);


/*****
    Attente d'un événement client ou serveur
*****/

LONG __inline Gal_WaitEvent(struct Twin *Twin, ULONG *SigMask)
{
    LONG Result;

    *SigMask=(1UL<<Twin->MainMPort->mp_SigBit)|
             (1UL<<Twin->IconMPort->mp_SigBit)|
             (1UL<<Twin->ClipboardSignal);

    SOCKET_FDZERO(&Twin->Connect.FDSSignal);
    SOCKET_FDSET(Twin->Connect.Socket,&Twin->Connect.FDSSignal);

    if((Result=SocketWaitTags(
        SOCKETWAIT_Read,(ULONG)&Twin->Connect.FDSSignal,
        SOCKETWAIT_Wait,(ULONG)SigMask,
        TAG_DONE))>=0)
    {
        Result=(*SigMask || !Result)?RESULT_EVENT_CLIENT:RESULT_EVENT_SERVER;
    }

    return Result;
}


/*****
    Lecture des données et vérifications des événements
*****/

LONG Gal_WaitRead(struct Twin *Twin, void *Buffer, LONG SizeOfBuffer)
{
    LONG Result=RESULT_SUCCESS;
    struct Connect *Connect=&Twin->Connect;

    if(Twin->CurrentGlobalPrefs.Flag)
    {
        /* Lecture socket bloquante */
        do
        {
            Result=SocketRead(Connect->Socket,Buffer,SizeOfBuffer);
            Buffer=(void *)&((UBYTE *)Buffer)[Result];
            SizeOfBuffer-=Result;
        } while(Result>0 && SizeOfBuffer>0);
    }
    else
    {
        LONG Offset=0;

        while(Result>0)
        {
            ULONG SigMask;

            /* Attente d'un événement */
            Result=Gal_WaitEvent(Twin,&SigMask);
            if(Result==RESULT_EVENT_SERVER)
            {
                if((Result=SocketRead(Connect->Socket,&((char *)Buffer)[Offset],(long)(SizeOfBuffer-Offset)))>0)
                {
                    Offset+=Result;
                    if(Offset>=SizeOfBuffer) return (LONG)SizeOfBuffer;
                }
            }
            else
            {
                Result=Gal_ManageMessagePort(Twin,SigMask);
            }
        }
    }

    return Result;
}


/*****
    Lecture des données dans un buffer à ne pas dépasser
*****/

LONG Gal_WaitFixedRead(struct Twin *Twin, void *Buffer, LONG SizeOfBuffer, LONG Len)
{
    LONG ReadLen=SizeOfBuffer<=Len?SizeOfBuffer:Len;
    LONG Result=Gal_WaitRead(Twin,Buffer,ReadLen);

    if(Result>0)
    {
        UBYTE Tmp=0;

        while(Result>0 && ReadLen<Len) {Result=Gal_WaitRead(Twin,(void *)&Tmp,1); ReadLen++;}
        if(Result>0) Result=Len;
    }

    return Result;
}


/*****
    Ecriture des données
*****/
/*
LONG __inline Gal_WaitWrite(struct Twin *Twin, void *Buffer, LONG SizeOfBuffer)
{
    return SocketWrite(Twin->Connect.Socket,(char *)Buffer,(long)SizeOfBuffer);
}
*/

/*****
    Gestion des messages
*****/

LONG Gal_ManageMessagePort(struct Twin *Twin, ULONG SigMask)
{
    LONG Result=RESULT_SUCCESS;
    LONG ControlCode=GAL_CTRL_NONE;
    BOOL IsCloseWindowOptions=FALSE;
    BOOL IsCloseWindowInformation=FALSE;
    BOOL IsCloseWindowFileTransfer=FALSE;
    BOOL IsCheckGlobalPrefs=FALSE;

    /* Gestion des messages Window */
    if(SigMask & (1UL<<Twin->MainMPort->mp_SigBit))
    {
        struct IntuiMessage *IntuiMsg;

        while(Result>0 && (IntuiMsg=GT_GetIMsg(Twin->MainMPort))!=NULL)
        {
            /* Gestion des messages de la fenêtre de visualisation */
            if(IntuiMsg->IDCMPWindow==Twin->Display.ViewInfo.WindowBase)
            {
                LONG Code;

                switch(IntuiMsg->Class)
                {
                    case IDCMP_GADGETUP:
                        Code=TBar_ManageToolBarMessages(IntuiMsg);
                        break;

                    case IDCMP_CLOSEWINDOW:
                        Code=GAL_CTRL_EXIT;
                        break;

                    default:
                        Code=P_Gal_CheckControlKeys(Twin,IntuiMsg);
                        if(Code==GAL_CTRL_NONE)
                        {
                            if(Inp_ManageInputMessages(Twin,IntuiMsg)) Result=Inp_SendInputMessages(Twin);
                            if(!Disp_ManageWindowDisplayMessages(&Twin->Display,IntuiMsg)) Result=RESULT_NOT_ENOUGH_MEMORY;
                        }
                        break;
                }

                /* On évite d'écraser un ancien ControlCode par un GAL_CTRL_NONE */
                if(Code>0) ControlCode=Code;
            }
            /* Gestion des messages de la fenêtre de connexion */
            else if(IntuiMsg->IDCMPWindow==Twin->WindowConnection.Window.WindowBase)
            {
                LONG WinResult=Win_ManageWindowConnectionMessages(&Twin->WindowConnection,IntuiMsg);
                if(WinResult==WIN_CONNECTION_RESULT_OPTIONS) ControlCode=GAL_CTRL_OPTIONS;
                else if(Twin->WindowConnectionResult<=0) Twin->WindowConnectionResult=WinResult;
            }
            /* Gestion des messages de la fenêtre de statut */
            else if(IntuiMsg->IDCMPWindow==Twin->WindowStatus.Window.WindowBase)
            {
                if(Win_ManageWindowStatusMessages(&Twin->WindowStatus,IntuiMsg)==WIN_STATUS_RESULT_CANCEL)
                {
                    Result=RESULT_EXIT;
                }
            }
            /* Gestion de la boîte de transfert de fichier */
            else if(IntuiMsg->IDCMPWindow==Twin->WindowFileTransfer.Window.WindowBase)
            {
                struct WindowFileTransfer *wft=&Twin->WindowFileTransfer;

                switch(Win_ManageWindowFileTransferMessages(wft,IntuiMsg))
                {
                    case WIN_FT_RESULT_ERROR:
                        Result=RESULT_NOT_ENOUGH_MEMORY;
                        break;

                    case WIN_FT_RESULT_REFRESH:
                        Result=Clt_SetFileListRequest(Twin,wft->RemoteFileManager.CurrentPath,1);
                        break;

                    case WIN_FT_RESULT_DOWNLOAD1:
                        {
                            const struct FileItemNode *fi=Win_GetLocalFileInfo(wft);

                            if(fi!=NULL)
                            {
                                ULONG Offset64[2]={0,0};
                                Result=Clt_SetFileUploadRequest(Twin,
                                    wft->LocalFileManager.CurrentPath,
                                    wft->RemoteFileManager.CurrentPath,
                                    fi->Node.ln_Name,
                                    fi->Size64,
                                    fi->Time,
                                    Offset64,
                                    TRUE);
                            }
                        }
                        break;

                    case WIN_FT_RESULT_DOWNLOAD2:
                        {
                            /*const struct FileItemNode *fi=Win_GetRemoteFileInfo(wft);

                            if(fi!=NULL)
                            {
                                Result=Fsc_ScheduleRemoteStart(&Twin->FileList,wft->LocalFileManager.CurrentPath,wft->RemoteFileManager.CurrentPath,fi,FSC_ACTION_DOWNLOAD);
                                if(Result>0)
                                {
                                    Result=Fsc_ScheduleRemoteNext(&Twin->FileList,Twin);
                                }
                            }*/

                            const struct FileItemNode *fi=Win_GetRemoteFileInfo(wft);

                            if(fi!=NULL)
                            {
                                Result=Clt_SetFileDownloadRequest(Twin,
                                    wft->LocalFileManager.CurrentPath,
                                    wft->RemoteFileManager.CurrentPath,
                                    fi->Node.ln_Name,
                                    fi->Size64,
                                    fi->Time);
                            }
                        }
                        break;

                    case WIN_FT_RESULT_DELETE:
                        {
                            const struct FileItemNode *fi=Win_GetRemoteFileInfo(wft);

                            if(fi!=NULL)
                            {
                                Result=Clt_SetFileRemoveRequest(Twin,wft->RemoteFileManager.CurrentPath,fi->Node.ln_Name);
                            }
                        }
                        break;

                    case WIN_FT_RESULT_CLOSE:
                        IsCloseWindowFileTransfer=TRUE;
                        break;
                }
            }
            /* Gestion des messages de la fenêtre d'options */
            else if(IntuiMsg->IDCMPWindow==Twin->WindowOptions.Window.WindowBase)
            {
                switch(Win_ManageWindowOptionsMessages(&Twin->WindowOptions,IntuiMsg))
                {
                    case WIN_OPTIONS_RESULT_TEST:
                        IsCheckGlobalPrefs=TRUE;
                        Twin->IsCheckProtocolPrefs=TRUE;
                        Twin->CurrentProtocolPrefs=Twin->NewProtocolPrefs;
                        break;

                    case WIN_OPTIONS_RESULT_USE:
                        IsCloseWindowOptions=TRUE;
                        IsCheckGlobalPrefs=TRUE;
                        Twin->IsCheckProtocolPrefs=TRUE;
                        Twin->CurrentProtocolPrefs=Twin->NewProtocolPrefs;
                        break;

                    case WIN_OPTIONS_RESULT_CANCEL:
                        IsCloseWindowOptions=TRUE;
                        IsCheckGlobalPrefs=TRUE;
                        Twin->IsCheckProtocolPrefs=TRUE;
                        Twin->NewGlobalPrefs=Twin->SavedGlobalPrefs;
                        Twin->CurrentProtocolPrefs=Twin->SavedProtocolPrefs;
                        break;

                    case WIN_OPTIONS_RESULT_SAVEAS:
                        Result=P_Gal_SaveConfigAs(Twin,&Twin->NewGlobalPrefs,&Twin->NewProtocolPrefs);
                        break;
                }
            }
            /* Gestion des messages de la fenêtre d'information */
            else if(IntuiMsg->IDCMPWindow==Twin->WindowInformation.Window.WindowBase)
            {
                if(Win_ManageWindowInformationMessages(&Twin->WindowInformation,IntuiMsg)==WIN_INFORMATION_RESULT_CLOSE)
                {
                    IsCloseWindowInformation=TRUE;
                }
            }

            GT_ReplyIMsg(IntuiMsg);
        }
    }

    /* Gestion des messages de l'icône */
    if(SigMask & (1UL<<Twin->IconMPort->mp_SigBit))
    {
        struct AppMessage *AppMsg;

        while(Result>0 && (AppMsg=(struct AppMessage *)GetMsg(Twin->IconMPort))!=NULL)
        {
            if(AppMsg->am_Type==AMTYPE_APPICON)
            {
                Result=Gal_ExpandApplication(Twin);
            }

            ReplyMsg((struct Message *)AppMsg);
        }
    }

    /* Gestion du Clipboard */
    if(SigMask & (1UL<<Twin->ClipboardSignal))
    {
        Result=SetClientCutText(Twin,Twin->CurrentGlobalPrefs.IsNoClipboard);
    }

    /* Fermeture de la boîte d'options si nécessaire */
    if(IsCloseWindowOptions)
    {
        Win_CloseWindowOptions(&Twin->WindowOptions);
    }

    /* Fermeture de la boîte d'options si nécessaire */
    if(IsCloseWindowInformation)
    {
        Win_CloseWindowInformation(&Twin->WindowInformation);
    }

    /* Fermeture de la boîte de transfert de fichier si nécessaire */
    if(IsCloseWindowFileTransfer)
    {
        Win_CloseWindowFileTransfer(&Twin->WindowFileTransfer);
    }

    if(IsCheckGlobalPrefs)
    {
        Result=P_Gal_CheckNewGlobalPrefs(Twin,&Twin->NewGlobalPrefs);
    }

    if(Result>0 && ControlCode>0)
    {
        Result=P_Gal_ExecuteControlOp(Twin,ControlCode);
    }

    return Result;
}


/*****
    Permet d'obtenir le bon PixelFormat correspondant au Depth passé
    en paramètre.
*****/

const struct PixelFormat *Gal_GetPixelFormatFromDepth(struct Twin *Twin, ULONG Depth)
{
    const struct PixelFormat *pixf=&pixf32;

    if(Depth<3) pixf=&Twin->Connect.ServerPixelFormat;
    else
    {
        if(Depth==3) pixf=&pixf3;
        else
        {
            if(Depth<=6) pixf=&pixf6;
            else
            {
                if(Depth<=8) pixf=&pixf8;
                else
                {
                    if(Depth<=16) pixf=&pixf16;
                }
            }
        }
    }

    return pixf;
}


/*****
    Retourne un pointeur sur la fenêtre mère idéale de l'application.
    Cette fonction retourne NULL si aucune fenêtre n'est trouvée.
*****/

struct Window *Gal_GetParentWindow(struct Twin *Twin)
{
    struct ViewInfo *vi=&Twin->Display.ViewInfo;

    if(vi->DisplayType!=DISP_TYPE_CUSTOM_SCREEN)
    {
        return vi->WindowBase;
    }

    return NULL;
}


/*****
    Verrouille l'écran sur lequel doit s'afficher l'interface graphique
*****/

struct Screen *Gal_GetBestPublicScreen(struct Window *ParentWindow, BOOL *IsLocked, ULONG *VerticalRatio)
{
    struct Screen *Scr=NULL;

    *IsLocked=FALSE;
    *VerticalRatio=1;

    if(ParentWindow!=NULL) Scr=ParentWindow->WScreen;

    if(Scr==NULL)
    {
        Scr=LockPubScreen(NULL);
        if(Scr!=NULL) *IsLocked=TRUE;
    }

    if(Scr!=NULL)
    {
        struct DrawInfo *sdi=GetScreenDrawInfo(Scr);

        if(sdi!=NULL)
        {
            LONG X=(LONG)sdi->dri_Resolution.X;

            if(X>0) *VerticalRatio=(((LONG)sdi->dri_Resolution.Y<<16)/X+0x7fff)>>16;
            FreeScreenDrawInfo(Scr,sdi);
        }
    }

    return Scr;
}


/*****
    Permet de définir le titre de la fenêtre et de l'écran
*****/

char *Gal_MakeDisplayTitle(struct Twin *Twin)
{
    char *Title=Twin->Title;
    struct Connect *co=&Twin->Connect;
    const struct PixelFormat *pixf=Gal_GetPixelFormatFromDepth(Twin,Twin->RemoteProtocolPrefs.Depth);

    Sys_SPrintf(Title,__APPNAME__" - %s:%ld (%s) - %ldx%ldx%ld",
        Twin->Host,
        Twin->Port,
        co->ServerName,
        co->ServerWidth,
        co->ServerHeight,
        (LONG)pixf->Depth);

    return Title;
}


/*****
    Permet de définir le titre de l'icône d'iconification
*****/

char *Gal_MakeAppTitle(struct Twin *Twin)
{
    char *Title=Twin->Title;
    LONG Len=Sys_StrLen(Twin->Connect.ServerName);
    char *Name=Twin->Host;

    if(Len>0 && Len<25) Name=Twin->Connect.ServerName;
    Sys_SPrintf(Title,__APPNAME__" [%s:%ld]",Name,Twin->Port);

    return Title;
}


/*****
    Nettoyage des ressources
*****/

void Gal_RemoveIconApplication(struct Twin *Twin)
{
    if(Twin->AppIcon!=NULL)
    {
        RemoveAppIcon(Twin->AppIcon);
        Twin->AppIcon=NULL;
    }
}


/*****
    Iconification de l'application
*****/

LONG Gal_IconifyApplication(struct Twin *Twin)
{
    LONG Result=RESULT_NOT_ENOUGH_MEMORY;

    if(Twin->AppIcon==NULL)
    {
        struct GlobalPrefs *gp=&Twin->CurrentGlobalPrefs;
        struct DiskObject *icon=gp->AltIconifyIcon!=NULL?gp->AltIconifyIcon:&Twin->DefaultDiskObject;
        char *Title=Gal_MakeAppTitle(Twin);

        Twin->AppIcon=AddAppIcon(1,0,Title,Twin->IconMPort,0,icon,NULL);
        if(Twin->AppIcon!=NULL)
        {
            Disp_CloseDisplay(&Twin->Display);
            Result=RESULT_SUCCESS;
        }
    }

    return Result;
}


/*****
    Désiconification de l'application
*****/

LONG Gal_ExpandApplication(struct Twin *Twin)
{
    LONG Result=RESULT_SUCCESS;

    if(Twin->AppIcon)
    {
        Gal_RemoveIconApplication(Twin);
        Result=Gal_OpenDisplay(Twin);
    }

    return Result;
}


/*****
    Initialisation de l'affichage
*****/

void Gal_InitDisplay(struct Twin *Twin)
{
    struct GlobalPrefs *gp=&Twin->CurrentGlobalPrefs;

    Disp_Init(&Twin->Display,
        &P_Gal_HookFuncOpenDisplay,&P_Gal_HookFuncCloseDisplay,(void *)Twin,
        Twin->MainMPort,
        gp->Left,gp->Top,gp->Width,gp->Height);
}


/*****
    Ouverture de la zone d'affichage
*****/

LONG Gal_OpenDisplay(struct Twin *Twin)
{
    LONG Result=RESULT_SUCCESS;
    struct Display *disp=&Twin->Display;
    struct GlobalPrefs *gp=&Twin->CurrentGlobalPrefs;
    LONG OverlayThreshold=gp->OverlayThreshold;
    BOOL IsFullScreen=(gp->DisplayOption!=DISPLAY_OPTION_WINDOW)?TRUE:FALSE;

    if(!gp->IsOverlay) OverlayThreshold=-1;

    if(IsFullScreen && disp->FrameBuffer.BufferPtr!=NULL)
    {
        BOOL IsAskModeID=(gp->ScrModeSelectType==SCRMODE_SELECT_ASK && Twin->IsDisplayChanged?TRUE:FALSE);
        ULONG ModeID=gp->ScrModeID;
        BOOL IsCanceled;

        if(gp->ScrModeSelectType==SCRMODE_SELECT_AUTO) ModeID=INVALID_ID;
        if(!Gal_GetScreenModeID(
            IsAskModeID,&ModeID,&gp->ScrDepth,
            disp->FrameBuffer.Width,
            disp->FrameBuffer.Height,
            &IsCanceled))
        {
            Result=RESULT_NOT_ENOUGH_MEMORY;
        } else if(!IsCanceled) gp->ScrModeID=ModeID;
    }

    if(Result>0)
    {
        ULONG ErrorCode=0;
        BOOL IsScaled=(gp->ViewOption==VIEW_OPTION_SCALE)?TRUE:FALSE;
        BOOL IsShowToolBar=(gp->DisplayOption!=DISPLAY_OPTION_NODISPLAY)?gp->IsShowToolBar:FALSE;
        BOOL IsScreenBar=(gp->DisplayOption!=DISPLAY_OPTION_NODISPLAY)?gp->IsScreenBar:FALSE;

        if(!Disp_OpenDisplay(&Twin->Display,
            Gal_MakeAppTitle(Twin),
            IsFullScreen,
            IsScaled,gp->IsSmoothed,OverlayThreshold,
            IsShowToolBar,IsScreenBar,
            gp->ScrModeID,gp->ScrDepth,
            &ErrorCode))
        {
            Result=P_Gal_ConvertScreenErrorCode(ErrorCode);
        }
        else
        {
            if(IsFullScreen) Twin->IsDisplayChanged=FALSE;
        }
    }

    return Result;
}


/*****
    Changement des paramètres de l'écran
*****/

LONG Gal_ChangeDisplay(struct Twin *Twin)
{
    LONG Result=RESULT_SUCCESS;
    struct Display *disp=&Twin->Display;
    struct GlobalPrefs *gp=&Twin->CurrentGlobalPrefs;
    LONG OverlayThreshold=gp->OverlayThreshold;
    BOOL IsFullScreen=(gp->DisplayOption!=DISPLAY_OPTION_WINDOW)?TRUE:FALSE;

    if(!gp->IsOverlay) OverlayThreshold=-1;

    if(IsFullScreen && disp->FrameBuffer.BufferPtr!=NULL)
    {
        BOOL IsAskModeID=(gp->ScrModeSelectType==SCRMODE_SELECT_ASK && Twin->IsDisplayChanged?TRUE:FALSE);
        ULONG ModeID=gp->ScrModeID;
        BOOL IsCanceled;

        if(gp->ScrModeSelectType==SCRMODE_SELECT_AUTO) ModeID=INVALID_ID;
        if(!Gal_GetScreenModeID(
            IsAskModeID,&ModeID,&gp->ScrDepth,
            disp->FrameBuffer.Width,
            disp->FrameBuffer.Height,
            &IsCanceled))
        {
            Result=RESULT_NOT_ENOUGH_MEMORY;
        } else if(!IsCanceled) gp->ScrModeID=ModeID;
    }

    if(Result>0)
    {
        ULONG ErrorCode=0;
        BOOL IsScaled=(gp->ViewOption==VIEW_OPTION_SCALE)?TRUE:FALSE;
        BOOL IsShowToolBar=(gp->DisplayOption!=DISPLAY_OPTION_NODISPLAY)?gp->IsShowToolBar:FALSE;
        BOOL IsScreenBar=(gp->DisplayOption!=DISPLAY_OPTION_NODISPLAY)?gp->IsScreenBar:FALSE;

        if(!Disp_ChangeDisplay(disp,
            IsFullScreen,
            IsScaled,gp->IsSmoothed,OverlayThreshold,
            IsShowToolBar,IsScreenBar,
            gp->ScrModeID,gp->ScrDepth,
            &ErrorCode))
        {
            Result=P_Gal_ConvertScreenErrorCode(ErrorCode);
        }
        else
        {
            if(IsFullScreen) Twin->IsDisplayChanged=FALSE;
        }
    }

    return Result;
}


/*****
    Allocation de quelques requesters
*****/

BOOL Gal_AllocAppResources(struct Twin *Twin)
{
    BOOL IsSuccess=FALSE;

    Fmg_InitFileManager(&Twin->WindowFileTransfer.LocalFileManager);
    Fmg_InitFileManager(&Twin->WindowFileTransfer.RemoteFileManager);
    Fsc_InitFileScheduler(&Twin->FileList);

    if(Util_ZLibInit(&Twin->Stream)==Z_OK)
    {
        Twin->IsStreamOk=TRUE;
        if(Fmg_RootCurrentPath(&Twin->WindowFileTransfer.LocalFileManager,FALSE))
        {
            if(Fmg_RootCurrentPath(&Twin->WindowFileTransfer.RemoteFileManager,TRUE))
            {
                Twin->FileReq=(struct FileRequester *)AllocAslRequest(ASL_FileRequest,TAG_DONE);
                Twin->SMReq=(struct ScreenModeRequester *)AllocAslRequest(ASL_ScreenModeRequest,TAG_DONE);
                Twin->FontReq=(struct FontRequester *)AllocAslRequest(ASL_FontRequest,TAG_DONE);

                if(Twin->FileReq!=NULL || Twin->SMReq!=NULL || Twin->FontReq!=NULL) IsSuccess=TRUE;
            }
        }
    }

    if(!IsSuccess) Gal_FreeAllAppResources(Twin);

    return IsSuccess;
}


/*****
    Permet de libérer les ressources allouées apres la phase d'initialisation
    de la fonction main(), c'est-à-dire pendant le fonctionnement de TwinVNC.
*****/

void Gal_FreeAllAppResources(struct Twin *Twin)
{
    struct GlobalPrefs *gp=&Twin->CurrentGlobalPrefs;

    if(Twin->IsStreamOk) Util_ZLibEnd(&Twin->Stream);
    Twin->IsStreamOk=FALSE;

    Win_CloseWindowStatus(&Twin->WindowStatus);
    Win_CloseWindowOptions(&Twin->WindowOptions);
    Win_CloseWindowInformation(&Twin->WindowInformation);
    Win_CloseWindowFileTransfer(&Twin->WindowFileTransfer);

    Gal_RemoveIconApplication(Twin);
    if(gp->AltIconifyIcon!=NULL)
    {
        FreeDiskObject(gp->AltIconifyIcon);
        gp->AltIconifyIcon=NULL;
    }

    Gui_FlushFontDef(&Twin->Font);

    if(Twin->FileReq!=NULL) FreeAslRequest((APTR)Twin->FileReq);
    if(Twin->SMReq!=NULL) FreeAslRequest((APTR)Twin->SMReq);
    if(Twin->FontReq!=NULL) FreeAslRequest((APTR)Twin->FontReq);
    Twin->FileReq=NULL;
    Twin->SMReq=NULL;
    Twin->FontReq=NULL;

    Fmg_FlushFileManagerResources(&Twin->WindowFileTransfer.LocalFileManager,TRUE);
    Fmg_FlushFileManagerResources(&Twin->WindowFileTransfer.RemoteFileManager,TRUE);
    Fsc_FlushFileSchedulerResources(&Twin->FileList);

    Clt_FlushFTFullPathFileName(Twin);
}


/*****
    Recherche du ModeID pour l'écran
*****/

BOOL Gal_GetScreenModeID(BOOL IsAskModeID, ULONG *ModeID, ULONG *Depth, LONG Width, LONG Height, BOOL *IsCanceled)
{
    BOOL IsSuccess=TRUE;

    *IsCanceled=FALSE;
    if(*ModeID==INVALID_ID || IsAskModeID)
    {
        ULONG LocalDepth=(*Depth>24)?24:*Depth;
        ULONG NewModeID;

        /* Récupération du ScreenModeID via graphics.library */
        NewModeID=BestModeID(
            BIDTAG_NominalWidth,(ULONG)Width,
            BIDTAG_NominalHeight,(ULONG)Height,
            BIDTAG_Depth,LocalDepth,
            //BIDTAG_MonitorID,*ModeID,
            TAG_DONE);

        if(NewModeID==INVALID_ID || IsAskModeID)
        {
            /* Récupération du ScreenModeID via un requester ASL */
            APTR ReqPtr=AllocAslRequestTags(ASL_ScreenModeRequest,TAG_DONE);

            IsSuccess=FALSE;
            if(ReqPtr)
            {
                BOOL Result=AslRequestTags(ReqPtr,
                    ASLSM_InitialDisplayID,NewModeID,
                    ASLSM_InitialDisplayDepth,LocalDepth,
                    ASLSM_InitialDisplayWidth,(ULONG)Width,
                    ASLSM_InitialDisplayHeight,(ULONG)Height,
                    ASLSM_DoDepth,TRUE,
                    TAG_DONE);

                if(Result)
                {
                    *Depth=(ULONG)((struct ScreenModeRequester *)ReqPtr)->sm_DisplayDepth;
                    *ModeID=((struct ScreenModeRequester *)ReqPtr)->sm_DisplayID;
                    IsSuccess=TRUE;
                }
                else if(!IoErr())
                {
                    *IsCanceled=TRUE;
                    IsSuccess=TRUE;
                }

                FreeAslRequest(ReqPtr);
            }
        }
        else
        {
            *ModeID=NewModeID;
        }
    }

    return IsSuccess;
}


/*****
    Permet de fermer les fenêtres desirées sur l'écran spécifié.
    Les fenêtres qui seront fermées pourront être à nouveau ouvertes
    par Gal_ShowHiddenWindow().
*****/

void Gal_HideOpennedWindow(struct Twin *Twin, struct Screen *ScrPtr, ULONG Flags)
{
    ULONG i;

    for(i=0;i<COUNTOF_POTENTIALY_HIDDEN_WINDOWS;i++) Twin->HiddenWindowOrder[i]=HIDDENWINDOW_NONE;

    Forbid();
    if(ScrPtr!=NULL)
    {
        /* Récupération de l'ordre de disposition des fenêtres */
        struct Layer *layer=ScrPtr->LayerInfo.top_layer;

        while(layer->back!=NULL) layer=layer->back;

        for(i=0;layer!=NULL && i<COUNTOF_POTENTIALY_HIDDEN_WINDOWS;layer=layer->front)
        {
            struct Window *win=(struct Window *)layer->Window;

            if(win!=NULL)
            {
                if(win==Twin->WindowConnection.Window.WindowBase && (Flags&HIDDENWINDOW_FLG_CONNECTION))
                {
                    Twin->HiddenWindowOrder[i++]=HIDDENWINDOW_FLG_CONNECTION;
                }
                if(win==Twin->WindowStatus.Window.WindowBase && (Flags&HIDDENWINDOW_FLG_STATUS))
                {
                    Twin->HiddenWindowOrder[i++]=HIDDENWINDOW_FLG_STATUS;
                }
                if(win==Twin->WindowInformation.Window.WindowBase && (Flags&HIDDENWINDOW_FLG_INFORMATION))
                {
                    Twin->HiddenWindowOrder[i++]=HIDDENWINDOW_FLG_INFORMATION;
                }
                if(win==Twin->WindowOptions.Window.WindowBase && (Flags&HIDDENWINDOW_FLG_OPTIONS))
                {
                    Twin->HiddenWindowOrder[i++]=HIDDENWINDOW_FLG_OPTIONS;
                }
                if(win==Twin->WindowFileTransfer.Window.WindowBase && (Flags&HIDDENWINDOW_FLG_FILETRANSFER))
                {
                    Twin->HiddenWindowOrder[i++]=HIDDENWINDOW_FLG_FILETRANSFER;
                }
            }
        }
    }
    Permit();

    /* Fermeture des fenêtres */
    if(Flags&HIDDENWINDOW_FLG_CONNECTION) Win_CloseWindowConnection(&Twin->WindowConnection);
    if(Flags&HIDDENWINDOW_FLG_STATUS) Win_CloseWindowStatus(&Twin->WindowStatus);
    if(Flags&HIDDENWINDOW_FLG_INFORMATION) Win_CloseWindowInformation(&Twin->WindowInformation);
    if(Flags&HIDDENWINDOW_FLG_OPTIONS) Win_CloseWindowOptions(&Twin->WindowOptions);
    if(Flags&HIDDENWINDOW_FLG_FILETRANSFER) Win_CloseWindowFileTransfer(&Twin->WindowFileTransfer);
}


/*****
    Réouverture des fenêtres fermées par Gal_HideOpennedWindow().
*****/

BOOL Gal_ShowHiddenWindow(struct Twin *Twin, ULONG Flags)
{
    BOOL IsSuccess=TRUE;
    LONG i;

    /* Affichage des fenêtres dans l'ordre original de profondeur */
    for(i=0;IsSuccess && i<COUNTOF_POTENTIALY_HIDDEN_WINDOWS;i++)
    {
        switch(Twin->HiddenWindowOrder[i])
        {
            case HIDDENWINDOW_FLG_CONNECTION:
                if(Flags&HIDDENWINDOW_FLG_CONNECTION)
                {
                    IsSuccess=Gal_EasyOpenWindowConnection(Twin);
                    Twin->HiddenWindowOrder[i]=HIDDENWINDOW_NONE;
                }
                break;

            case HIDDENWINDOW_FLG_STATUS:
                if(Flags&HIDDENWINDOW_FLG_STATUS)
                {
                    IsSuccess=Gal_EasyOpenWindowStatus(Twin);
                    Twin->HiddenWindowOrder[i]=HIDDENWINDOW_NONE;
                }
                break;

            case HIDDENWINDOW_FLG_INFORMATION:
                if(Flags&HIDDENWINDOW_FLG_INFORMATION)
                {
                    IsSuccess=Gal_EasyOpenWindowInformation(Twin);
                    Twin->HiddenWindowOrder[i]=HIDDENWINDOW_NONE;
                }
                break;

            case HIDDENWINDOW_FLG_OPTIONS:
                if(Flags&HIDDENWINDOW_FLG_OPTIONS)
                {
                    IsSuccess=Gal_EasyOpenWindowOptions(Twin);
                    Twin->HiddenWindowOrder[i]=HIDDENWINDOW_NONE;
                }
                break;

            case HIDDENWINDOW_FLG_FILETRANSFER:
                if(Flags&HIDDENWINDOW_FLG_FILETRANSFER)
                {
                    IsSuccess=Gal_EasyOpenWindowFileTransfer(Twin);
                    Twin->HiddenWindowOrder[i]=HIDDENWINDOW_NONE;
                }
                break;
        }
    }

    return IsSuccess;
}


/*****
    Ouverture facile de la boîte de connexion
*****/

BOOL Gal_EasyOpenWindowConnection(struct Twin *Twin)
{
    BOOL IsSuccess=TRUE;
    struct Window *win=Twin->WindowConnection.Window.WindowBase;

    if(win==NULL)
    {
        IsSuccess=Win_OpenWindowConnection(
            &Twin->WindowConnection,Gal_GetParentWindow(Twin),
            Twin->Host,&Twin->Port,Twin->Password,
            &Twin->CurrentGlobalPrefs,RFB_PORT,
            &Twin->Font,Twin->MainMPort);
    }
    else
    {
        ActivateWindow(win);
        WindowToFront(win);
    }

    return IsSuccess;
}


/*****
    Ouverture facile de la boîte de statut
*****/

BOOL Gal_EasyOpenWindowStatus(struct Twin *Twin)
{
    BOOL IsSuccess=TRUE;
    struct Window *win=Twin->WindowStatus.Window.WindowBase;

    if(win==NULL)
    {
        return Win_OpenWindowStatus(
            &Twin->WindowStatus,Gal_GetParentWindow(Twin),
            &Twin->Font,Twin->MainMPort);
    }
    else
    {
        ActivateWindow(win);
        WindowToFront(win);
    }

    return IsSuccess;
}


/*****
    Ouverture facile de la boîte d'information
*****/

BOOL Gal_EasyOpenWindowInformation(struct Twin *Twin)
{
    BOOL IsSuccess=TRUE;
    struct Window *win=Twin->WindowInformation.Window.WindowBase;

    if(win==NULL)
    {
        return Win_OpenWindowInformation(
            &Twin->WindowInformation,Gal_GetParentWindow(Twin),
            &Twin->Connect,Twin->Host,
            &Twin->Font,Twin->MainMPort);
    }
    else
    {
        ActivateWindow(win);
        WindowToFront(win);
    }

    return IsSuccess;
}


/*****
    Ouverture de la boîte d'options
*****/

BOOL Gal_EasyOpenWindowOptions(struct Twin *Twin)
{
    BOOL IsSuccess=TRUE;
    struct Window *win=Twin->WindowOptions.Window.WindowBase;

    if(win==NULL)
    {
        return Win_OpenWindowOptions(
            &Twin->WindowOptions,Gal_GetParentWindow(Twin),
            &Twin->NewProtocolPrefs,&Twin->NewGlobalPrefs,
            Twin->SMReq,Twin->FontReq,
            &Twin->Font,Twin->MainMPort);
    }
    else
    {
        ActivateWindow(win);
        WindowToFront(win);
    }

    return IsSuccess;
}


/*****
    Ouverture facile de la boîte de transfert de fichier
*****/

BOOL Gal_EasyOpenWindowFileTransfer(struct Twin *Twin)
{
    BOOL IsSuccess=TRUE;
    struct Window *win=Twin->WindowFileTransfer.Window.WindowBase;

    if(win==NULL)
    {
        return Win_OpenWindowFileTransfer(
            &Twin->WindowFileTransfer,Gal_GetParentWindow(Twin),
            &Twin->Font,Twin->MainMPort,
            Twin->Connect.TightCapClient);
    }
    else
    {
        ActivateWindow(win);
        WindowToFront(win);
    }

    return IsSuccess;
}


/*****
    Sauvegarde de la config
*****/

LONG P_Gal_SaveConfigAs(struct Twin *Twin, struct GlobalPrefs *gp, struct ProtocolPrefs *pp)
{
    LONG Result=RESULT_NOT_ENOUGH_MEMORY;
    struct Window *win=Gal_GetParentWindow(Twin);

    if(AslRequestTags((APTR)Twin->FileReq,
        ASLFR_Window,(ULONG)win,
        ASLFR_SleepWindow,TRUE,
        ASLFR_InitialPattern,"#?.info",
        ASLFR_TitleText,Sys_GetString(LOC_OPTIONS_SAVEAS_TITLE,"Save current configuration"),
        TAG_DONE))
    {
        BPTR DirLock=Lock((Twin->FileReq)->fr_Drawer,ACCESS_READ);

        if(DirLock!=MKBADDR(NULL))
        {
            BPTR PrevLock=CurrentDir(DirLock);
            char *Suffix=".info";
            char *OrgFileName=(Twin->FileReq)->fr_File;
            LONG BufLen=Sys_StrLen(OrgFileName)+Sys_StrLen(Suffix)+sizeof(char);
            char *AltFileName=Sys_AllocMem(BufLen);

            if(AltFileName!=NULL)
            {
                BOOL IsWriteFile=TRUE;
                BPTR FileLock;
                char *FileExt;

                Sys_StrCopy(AltFileName,OrgFileName,BufLen);

                /* Suppression de l'extension ".info" si elle existe */
                if((FileExt=(char *)P_Gal_FindFileExtension(AltFileName))!=NULL)
                {
                    if(Sys_StrCmp(FileExt,Suffix)==0) *FileExt=0;
                }

                /* Ajout de l'extension ".info" de TwinVNC */
                Sys_StrCopy(&AltFileName[Sys_StrLen(AltFileName)],Suffix,Sys_StrLen(Suffix)+sizeof(char));

                /* Vérification si le fichier existe */
                FileLock=Lock(AltFileName,ACCESS_WRITE);
                if(FileLock!=MKBADDR(NULL))
                {
                    struct EasyStruct Msg;

                    Msg.es_StructSize=sizeof (struct EasyStruct);
                    Msg.es_Flags=0;
                    Msg.es_Title=(STRPTR)Sys_GetString(LOC_SAVEAS_INFO_TITLE,"Write configuration message...");
                    Msg.es_TextFormat=(STRPTR)Sys_GetString(LOC_SAVEAS_INFO_MESSAGE,"The \"%s\" file already exists");
                    Msg.es_GadgetFormat=(STRPTR)Sys_GetString(LOC_SAVEAS_INFO_BUTTONS,"Overwrite|Cancel");
                    if(EasyRequest(win,&Msg,NULL,AltFileName)==0) IsWriteFile=FALSE;

                    UnLock(FileLock);
                }

                Result=RESULT_SUCCESS;
                if(IsWriteFile)
                {
                    struct WindowConnection *wc=&Twin->WindowConnection;
                    BOOL IsForcePassword=(Twin->Connect.Socket!=NULL)?TRUE:FALSE;

                    /* Récupération de l'ip/port et du mot de passe courant */
                    if(wc->Window.WindowBase!=NULL)
                    {
                        Sys_StrCopy(Twin->Host,Win_GetWindowConnectionHost(wc,&Twin->Port),SIZEOF_HOST_BUFFER);
                        Sys_StrCopy(Twin->Password,Win_GetWindowConnectionPassword(wc),SIZEOF_PASSWORD_BUFFER);
                    }

                    /* Récupération des dimensions de la fenêtre */
                    Disp_GetDisplayDimensions(&Twin->Display,&gp->Left,&gp->Top,&gp->Width,&gp->Height);

                    /* On supprime l'extension ajoutée, car Cfg_WriteConfig n'en veut pas */
                    FileExt=(char *)P_Gal_FindFileExtension(AltFileName);
                    *FileExt=0;

                    if(!Cfg_WriteConfig(gp,pp,Twin->AppPath,Twin->Host,Twin->Port,Twin->Password,IsForcePassword,AltFileName))
                    {
                        if(IoErr())
                        {
                            struct EasyStruct Msg;
                            char Message[64];

                            Msg.es_StructSize=sizeof (struct EasyStruct);
                            Msg.es_Flags=0;
                            Msg.es_Title=(STRPTR)Sys_GetString(LOC_SAVEAS_ERROR_TITLE,"Write configuration error...");
                            Msg.es_TextFormat=Message;
                            Msg.es_GadgetFormat=(STRPTR)Sys_GetString(LOC_SAVEAS_ERROR_BUTTONS,"Ok");
                            Fault(IoErr(),NULL,Message,sizeof(Message));
                            EasyRequest(win,&Msg,NULL,AltFileName);
                        } else Result=RESULT_NOT_ENOUGH_MEMORY;
                    }
                }

                Sys_FreeMem(AltFileName);
            }

            CurrentDir(PrevLock);
            UnLock(DirLock);
        }
    }
    else
    {
        if(!IoErr()) Result=RESULT_SUCCESS;
    }

    return Result;
}


/*****
    Vérification des options
*****/

LONG P_Gal_CheckNewGlobalPrefs(struct Twin *Twin, struct GlobalPrefs *NewPrefs)
{
    LONG Result=RESULT_SUCCESS;
    struct GlobalPrefs *gp=&Twin->CurrentGlobalPrefs;
    BOOL IsChangeDisplay=FALSE;

    if(NewPrefs->ScrModeSelectType!=gp->ScrModeSelectType ||
       NewPrefs->ScrModeID!=gp->ScrModeID ||
       NewPrefs->ScrDepth!=gp->ScrDepth ||
       NewPrefs->DisplayOption!=gp->DisplayOption ||
       NewPrefs->IsShowToolBar!=gp->IsShowToolBar ||
       NewPrefs->IsScreenBar!=gp->IsScreenBar ||
       NewPrefs->ViewOption!=gp->ViewOption ||
       NewPrefs->IsSmoothed!=gp->IsSmoothed ||
       NewPrefs->IsOverlay!=gp->IsOverlay ||
       NewPrefs->OverlayThreshold!=gp->OverlayThreshold)
    {
        IsChangeDisplay=TRUE;
    }

    if(NewPrefs->FontSize!=gp->FontSize || Sys_StrCmp(NewPrefs->FontName,gp->FontName)!=0)
    {
        struct Screen *ScrPtr=NULL;

        if(Twin->Display.ViewInfo.WindowBase!=NULL) ScrPtr=Twin->Display.ViewInfo.WindowBase->WScreen;
        if(ScrPtr==NULL && Twin->WindowConnection.Window.WindowBase!=NULL) ScrPtr=Twin->WindowConnection.Window.WindowBase->WScreen;

        Gal_HideOpennedWindow(Twin,ScrPtr,HIDDENWINDOW_ALL);
        Gui_ChangeFontDef(&Twin->Font,NewPrefs->FontName,NewPrefs->FontSize);
        if(!Gal_ShowHiddenWindow(Twin,HIDDENWINDOW_ALL)) Result=RESULT_NOT_ENOUGH_MEMORY;
    }

    *gp=*NewPrefs;

    if(IsChangeDisplay && Result>0)
    {
        Result=Gal_ChangeDisplay(Twin);
    }

    Disp_SetAmigaCursor(&Twin->Display,VIEWCURSOR_NORMAL,0,0,TRUE);

    return Result;
}


/*****
    Hook d'ouverture de l'affichage
*****/

BOOL P_Gal_HookFuncOpenDisplay(void *UserData)
{
    struct Twin *Twin=(struct Twin *)UserData;

    return Gal_ShowHiddenWindow(Twin,HIDDENWINDOW_ALL);
}


/*****
    Hook de fermeture de l'affichage
*****/

void P_Gal_HookFuncCloseDisplay(void *UserData)
{
    struct Twin *Twin=(struct Twin *)UserData;
    struct Window *win=Twin->Display.ViewInfo.WindowBase;

    if(win!=NULL)
    {
        Gal_HideOpennedWindow(Twin,win->WScreen,HIDDENWINDOW_ALL);
    }
}


/*****
    Test des touches de contrôle de TwinVNC
*****/

LONG P_Gal_CheckControlKeys(struct Twin *Twin, struct IntuiMessage *msg)
{
    LONG ControlCode=GAL_CTRL_NONE;
    LONG ControlKey=Twin->CurrentGlobalPrefs.ControlKey;
    LONG Qualifier=(LONG)(msg->Qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT|IEQUALIFIER_CONTROL|IEQUALIFIER_LALT|IEQUALIFIER_RALT|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND));

    if((Qualifier&ControlKey)==ControlKey && msg->Class==IDCMP_RAWKEY)
    {
        struct InputEvent ie;
        UBYTE Buffer[16];

        /* On remap le code Raw pour savoir quelle touche a été appuyée */
        ie.ie_Class=IECLASS_RAWKEY;
        ie.ie_SubClass=0;
        ie.ie_Code=msg->Code;
        ie.ie_Qualifier=Qualifier&(~ControlKey);
        ie.ie_EventAddress=(APTR *)*((ULONG *)msg->IAddress);

        if(MapRawKey(&ie,(STRPTR)Buffer,sizeof(Buffer),NULL)==1)
        {
            switch(Buffer[0])
            {
                case 'q': /* Pour quitter TwinVNC */
                case 'Q':
                    ControlCode=GAL_CTRL_EXIT;
                    break;

                case 'r': /* Pour rafraichir l'écran */
                case 'R':
                    ControlCode=GAL_CTRL_REFRESH;
                    break;

                case 'i': /* Pour iconifier l'application */
                case 'I':
                    ControlCode=GAL_CTRL_ICONIFY;
                    break;

                case 'f': /* Pour switcher fullscreen/window */
                case 'F':
                    ControlCode=GAL_CTRL_SWITCH_VIEW;
                    break;

                case 'd': /* Pour switcher No Display/Display */
                case 'D':
                    ControlCode=GAL_CTRL_SWITCH_DISPLAY;
                    break;

                case 's': /* Pour switcher scale/pas scale */
                case 'S':
                    ControlCode=GAL_CTRL_SWITCH_SCALE;
                    break;

                case 't': /* Pour afficher la toolbar */
                case 'T':
                    ControlCode=GAL_CTRL_SWITCH_TOOLBAR;
                    break;

                case 'b': /* Pour afficher la toolbar */
                case 'B':
                    ControlCode=GAL_CTRL_SWITCH_SCREENBAR;
                    break;

                case 'o': /* Pour afficher la boîte d'options */
                case 'O':
                    ControlCode=GAL_CTRL_OPTIONS;
                    break;

                case 'k': /* Pour afficher la boîte de transfert de fichier */
                case 'K':
                    ControlCode=GAL_CTRL_FILE_TRANSFER;
                    break;

                case 'p': /* Pour retailler proportionnellement la vue */
                case 'P':
                    ControlCode=GAL_CTRL_VIEW_RATIO;
                    break;

                case 13:  /* Pour retailler la fenêtre à 100% */
                    ControlCode=GAL_CTRL_RESIZE_FULL;
                    break;
            }
        }
    }

    return ControlCode;
}


/*****
    Exécution des operations de contrôle de TwinVNC
*****/

LONG P_Gal_ExecuteControlOp(struct Twin *Twin, LONG ControlCode)
{
    LONG Result=RESULT_SUCCESS;
    struct GlobalPrefs *gp=&Twin->CurrentGlobalPrefs;
    struct Display *disp=&Twin->Display;

    switch(ControlCode)
    {
        case GAL_CTRL_EXIT:
            Result=RESULT_EXIT;
            break;

        case GAL_CTRL_ICONIFY:
            Result=Gal_IconifyApplication(Twin);
            break;

        case GAL_CTRL_OPTIONS:
            if(Twin->WindowOptions.Window.WindowBase==NULL)
            {
                Twin->SavedGlobalPrefs=*gp;
                Twin->SavedProtocolPrefs=Twin->CurrentProtocolPrefs;
                Twin->NewGlobalPrefs=*gp;
                Twin->NewProtocolPrefs=Twin->CurrentProtocolPrefs;
                if(Twin->WindowOptions.Page==4) Twin->WindowOptions.Page=0;
            }
            if(Gal_EasyOpenWindowOptions(Twin))
            {
                if(Twin->WindowOptions.Page==4) Win_SetOptionsPage(&Twin->WindowOptions,0);
            } else Result=RESULT_NOT_ENOUGH_MEMORY;
            break;

        case GAL_CTRL_ABOUT:
            if(Twin->WindowOptions.Window.WindowBase==NULL)
            {
                Twin->SavedGlobalPrefs=*gp;
                Twin->SavedProtocolPrefs=Twin->CurrentProtocolPrefs;
                Twin->NewGlobalPrefs=*gp;
                Twin->NewProtocolPrefs=Twin->CurrentProtocolPrefs;
                Twin->WindowOptions.Page=4;
            }
            if(Gal_EasyOpenWindowOptions(Twin))
            {
                Win_SetOptionsPage(&Twin->WindowOptions,4);
            } else Result=RESULT_NOT_ENOUGH_MEMORY;
            break;

        case GAL_CTRL_INFORMATION:
            if(!Gal_EasyOpenWindowInformation(Twin))
            {
                Result=RESULT_NOT_ENOUGH_MEMORY;
            }
            break;

        case GAL_CTRL_SWITCH_VIEW:
            gp->DisplayOption=(gp->DisplayOption!=DISPLAY_OPTION_WINDOW)?DISPLAY_OPTION_WINDOW:DISPLAY_OPTION_FULLSCREEN;
            Twin->SavedGlobalPrefs.DisplayOption=gp->DisplayOption;
            Twin->NewGlobalPrefs.DisplayOption=gp->DisplayOption;
            Win_RefreshOptionsPage(&Twin->WindowOptions);
            Result=Gal_ChangeDisplay(Twin);
            break;

        case GAL_CTRL_SWITCH_DISPLAY:
            if(gp->DisplayOption!=DISPLAY_OPTION_NODISPLAY)
            {
                disp->PrevDisplayOption=gp->DisplayOption;
                gp->DisplayOption=DISPLAY_OPTION_NODISPLAY;
            }
            else
            {
                gp->DisplayOption=disp->PrevDisplayOption;
            }
            Twin->SavedGlobalPrefs.DisplayOption=gp->DisplayOption;
            Twin->NewGlobalPrefs.DisplayOption=gp->DisplayOption;
            Win_RefreshOptionsPage(&Twin->WindowOptions);
            Result=Gal_ChangeDisplay(Twin);
            break;

        case GAL_CTRL_SWITCH_SCALE:
            gp->ViewOption=(gp->ViewOption!=VIEW_OPTION_SCALE)?VIEW_OPTION_SCALE:VIEW_OPTION_NORMAL;
            Twin->SavedGlobalPrefs.ViewOption=gp->ViewOption;
            Twin->NewGlobalPrefs.ViewOption=gp->ViewOption;
            Win_RefreshOptionsPage(&Twin->WindowOptions);
            Result=Gal_ChangeDisplay(Twin);
            break;

        case GAL_CTRL_SWITCH_SCREENBAR:
            gp->IsScreenBar=(gp->IsScreenBar?FALSE:TRUE);
            Twin->SavedGlobalPrefs.IsScreenBar=gp->IsScreenBar;
            Twin->NewGlobalPrefs.IsScreenBar=gp->IsScreenBar;
            Win_RefreshOptionsPage(&Twin->WindowOptions);
            Result=Gal_ChangeDisplay(Twin);
            break;

        case GAL_CTRL_RESIZE_FULL:
            if(!Disp_ResizeView(disp,gp->Width,gp->Height,FALSE)) Result=RESULT_NOT_ENOUGH_MEMORY;
            break;

        case GAL_CTRL_VIEW_RATIO:
            if(disp->ViewInfo.DisplayType!=DISP_TYPE_WINDOW)
            {
                struct Window *win=disp->ViewInfo.WindowBase;
                LONG NewWidth=(LONG)win->WScreen->Width;
                LONG NewHeight=(LONG)win->WScreen->Height-(LONG)win->TopEdge-disp->ViewInfo.BorderTop;
                disp->IsInitialFullScreenProportional^=TRUE;
                if(!Disp_ResizeView(disp,NewWidth,NewHeight,disp->IsInitialFullScreenProportional)) Result=RESULT_NOT_ENOUGH_MEMORY;
            }
            else
            {
                /* NOTE: Pas de flip flop en mode window! */
                if(!Disp_ResizeView(disp,disp->ViewInfo.ViewWidth,disp->ViewInfo.ViewHeight,TRUE)) Result=RESULT_NOT_ENOUGH_MEMORY;
            }
            break;

        case GAL_CTRL_REFRESH:
            Twin->IsRefreshIncremental=FALSE;
            break;

        case GAL_CTRL_RECONNECT:
            Result=RESULT_RECONNECT;
            break;

        case GAL_CTRL_FILE_TRANSFER:
            Result=RESULT_NOT_ENOUGH_MEMORY;
            if(Gal_EasyOpenWindowFileTransfer(Twin))
            {
                Result=Clt_SetFileListRequest(Twin,Twin->WindowFileTransfer.RemoteFileManager.CurrentPath,1);
            }
            break;

        case GAL_CTRL_SWITCH_TOOLBAR:
            if(disp->ViewInfo.DisplayType!=DISP_TYPE_CUSTOM_SCREEN)
            {
                gp->IsShowToolBar=(gp->IsShowToolBar?FALSE:TRUE);
                Twin->SavedGlobalPrefs.IsShowToolBar=gp->IsShowToolBar;
                Twin->NewGlobalPrefs.IsShowToolBar=gp->IsShowToolBar;
                Win_RefreshOptionsPage(&Twin->WindowOptions);
                Result=Gal_ChangeDisplay(Twin);
            }
            break;
    }

    return Result;
}


/*****
    Conversion du code d'erreur d'ouverture d'écran
*****/

LONG P_Gal_ConvertScreenErrorCode(ULONG ErrorCode)
{
    LONG Result;

    switch(ErrorCode)
    {
        case OSERR_NOMONITOR:       /* named monitor spec not available */
        case OSERR_NOCHIPS:         /* you need newer custom chips */
        case OSERR_UNKNOWNMODE:     /* don't recognize mode asked for */
        case OSERR_NOTAVAILABLE:    /* Mode not available for other reason */
            Result=RESULT_BAD_SCREEN_MODEID;
            break;

        case OSERR_NOCHIPMEM:       /* couldn't get chipmem */
            Result=RESULT_NOT_ENOUGH_VIDEO_MEMORY;
            break;

        case OSERR_TOODEEP:         /* Screen deeper than HW supports */
            Result=RESULT_BAD_SCREEN_DEPTH;
            break;

        case OSERR_PUBNOTUNIQUE:    /* public screen name already used */
            Result=RESULT_CANT_OPEN_SCREEN;
            break;

        case OSERR_NOMEM:           /* couldn't get normal memory */
        case OSERR_ATTACHFAIL:      /* Failed to attach screens */
        default:
            Result=RESULT_NOT_ENOUGH_MEMORY;
            break;
    }

    return Result;
}


/*****
    Retourne un pointeur sur le caractère '.' du suffixe et NULL s'il n'y a pas de suffixe.
*****/

const char *P_Gal_FindFileExtension(const char *FileName)
{
    LONG Len=Sys_StrLen(FileName);

    while(--Len>=0)
    {
        const char *Ptr=&FileName[Len];
        if(*Ptr=='.') return Ptr;
    }

    return NULL;
}


/*
//#include <stdio.h>
//#include <string.h>
//#include <time.h>
void WriteLog(const char *Msg)
{
    BOOL IsSuccess=TRUE;
    char Path[]="dh6:twinvnc.log";
    BOOL IsOk=FALSE;
    ULONG i=0;

    while(IsOk==FALSE && i<10)
    {
        FILE *h=fopen(Path,"a+");

        if(h!=NULL)
        {
            static char Tmp[128];
            time_t t=time(NULL);
            struct tm *tm=localtime(&t);
            int Len=0;

            if(tm!=NULL)
            {
                sprintf(Tmp,"%02d/%02d/%04d %02d:%02d:%02d (%ld) ",tm->tm_mday,tm->tm_mon+1,tm->tm_year+1900,tm->tm_hour,tm->tm_min,tm->tm_sec,i);
                Len=strlen(Tmp);
                if(fwrite(Tmp,sizeof(char),Len,h)!=(size_t)Len) IsSuccess=FALSE;
            }

            Len=strlen(Msg);
            if(fwrite(Msg,sizeof(char),Len,h)!=(size_t)Len) IsSuccess=FALSE;

            sprintf(Tmp,"\n");
            Len=strlen(Tmp);
            if(fwrite(Tmp,sizeof(char),Len,h)!=(size_t)Len) IsSuccess=FALSE;

            fclose(h);
            IsOk=TRUE;
        }
        else
        {
            Delay(100);
        }
    }
}
*/
