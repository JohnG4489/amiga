#include "global.h"
#include "system.h"
#include "windowfiletransfer.h"
#include "general.h"
#include "gui.h"
#include "twinvnc_const.h"
#include "rfb.h"
#include "twingadgetclass.h"
#include "icons_ft.h"
#include "display.h"
#include <intuition/gadgetclass.h>


/*
    02-07-2017 (Seg)    Modif suite refonte arithmétique 64 bits
    02-05-2016 (Seg)    Refonte de la boîte pour la prochaine version
    15-12-2007 (Seg)    Création de la boîte de transfert de fichiers
*/


/***** Prototypes */
BOOL Win_OpenWindowFileTransfer(struct WindowFileTransfer *, struct Window *, struct FontDef *, struct MsgPort *, ULONG);
void Win_CloseWindowFileTransfer(struct WindowFileTransfer *);
LONG Win_ManageWindowFileTransferMessages(struct WindowFileTransfer *, struct IntuiMessage *);

struct FileManager *Win_BeginRemoteFileManagerRefresh(struct WindowFileTransfer *);
void Win_EndRemoteFileManagerRefresh(struct WindowFileTransfer *);
const struct FileItemNode *Win_GetRemoteFileInfo(struct WindowFileTransfer *);

struct FileManager *Win_BeginLocalFileManagerRefresh(struct WindowFileTransfer *);
void Win_EndLocalFileManagerRefresh(struct WindowFileTransfer *);
const struct FileItemNode *Win_GetLocalFileInfo(struct WindowFileTransfer *);

void Win_SetGaugeProgress(struct WindowFileTransfer *, ULONG);
BOOL Win_RefreshFileList(struct WindowFileTransfer *);

void Win_InitWindowFileTransferGadgets(struct FontDef *, ULONG);
void Win_UpdateFileTransfertGadgetsAttr(struct WindowFileTransfer *, ULONG);
BOOL Win_SetLocalList(struct WindowFileTransfer *, const char *, BOOL *);
BOOL Win_CheckDoubleClick(struct WindowFileTransfer *, LONG);

M_HOOK_PROTO(ListHook, struct Hook *, struct Node *, struct LVDrawMsg *);


/***** Defines locaux */
#define WIN_WIDTH               620
#define WIN_HEIGHT              370


/***** Variables globales */
struct TagItem FileTransferNoTag[]=
{
    {TAG_DONE,0UL}
};

struct TagItem FileTransferTextTag[]=
{
    {GTTX_Clipped,TRUE},
    {GTTX_Border,TRUE},
    {TAG_DONE,0UL}
};

struct TagItem FileTransferListTag[]=
{
    {GTLV_CallBack,(ULONG)NULL},
    {GTLV_ShowSelected,(ULONG)NULL},
    //{GTLV_MakeVisible,5},
    //{GTLV_Top,5},
    {TAG_DONE,0UL}
};

struct TagItem HistoryListTag[]=
{
    {TAG_DONE,0UL}
};


/***** Paramètres des gadgets */
struct TextAttr FileTransferStdTextAttr={NULL,0,0,0};
struct CustGadgetAttr FileTransferTextGadgetAttr        ={-1,"",&FileTransferStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr FileTransferRootGadgetAttr        ={LOC_FT_ROOT,"Root",&FileTransferStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr FileTransferMkDirGadgetAttr       ={LOC_FT_MKDIR,"MkDir",&FileTransferStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr FileTransferRenameGadgetAttr      ={LOC_FT_RENAME,"Ren",&FileTransferStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr FileTransferDeleteGadgetAttr      ={LOC_FT_DELETE,"Del",&FileTransferStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr FileTransferParentGadgetAttr      ={-1,"<-",&FileTransferStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr FileTransferRefreshGadgetAttr     ={-1,"R",&FileTransferStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr FileTransferUploadGadgetAttr      ={-1,">>",&FileTransferStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr FileTransferDownloadGadgetAttr    ={-1,"<<",&FileTransferStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr FileTransferAbortGadgetAttr       ={LOC_FT_ABORT,"Abort",&FileTransferStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr FileTransferHideGadgetAttr        ={LOC_FT_HIDE,"Hide",&FileTransferStdTextAttr,PLACETEXT_IN};


/***** Liste des gadgets */
enum
{
    TAGID_REMOTE,
    TAGID_LOCAL,

    TAGID_ROOT1,
    TAGID_MKDIR1,
    TAGID_RENAME1,
    TAGID_DELETE1,
    TAGID_PARENT1,
    TAGID_REFRESH1,
    TAGID_DOWNLOAD1,

    TAGID_ROOT2,
    TAGID_MKDIR2,
    TAGID_RENAME2,
    TAGID_DELETE2,
    TAGID_PARENT2,
    TAGID_REFRESH2,
    TAGID_DOWNLOAD2,

    TAGID_ABORT,
    TAGID_HISTORY,
    TAGID_HIDE
};

#define HISTORY_W       (WIN_WIDTH-30)
#define HISTORY_H       (5*8+11)
#define HISTORY_X       15
#define HISTORY_Y       (WIN_HEIGHT-HISTORY_H-55)

#define LISTER_LINE     23
#define LISTER_W        (WIN_WIDTH/2-16)
#define LISTER_H        (LISTER_LINE*8+11)
#define LISTER1_X       15
#define LISTER2_X       (WIN_WIDTH/2+2)
#define LISTER_OPTION_Y (LISTER_H+18)
#define LISTER_OPTION_W 26
#define LISTER_OPTION_H 26

struct TagItem IconHomeTag[]=
{
    {TIMG_Image,(ULONG)IconsFileData},
    {TIMG_ColorMapRGB,(ULONG)IconsFilePalette},
    {TIMG_PosX,0UL},
    {TIMG_Width,24UL},
    {TIMG_Height,24UL},
    {TAG_DONE,0}
};

struct TagItem IconParentTag[]=
{
    {TIMG_Image,(ULONG)IconsFileData},
    {TIMG_ColorMapRGB,(ULONG)IconsFilePalette},
    {TIMG_PosX,24UL},
    {TIMG_Width,24UL},
    {TIMG_Height,24UL},
    {TAG_DONE,0}
};

struct TagItem IconRefreshTag[]=
{
    {TIMG_Image,(ULONG)IconsFileData},
    {TIMG_ColorMapRGB,(ULONG)IconsFilePalette},
    {TIMG_PosX,48UL},
    {TIMG_Width,24UL},
    {TIMG_Height,24UL},
    {TAG_DONE,0}
};

struct TagItem IconMkDirTag[]=
{
    {TIMG_Image,(ULONG)IconsFileData},
    {TIMG_ColorMapRGB,(ULONG)IconsFilePalette},
    {TIMG_PosX,72UL},
    {TIMG_Width,24UL},
    {TIMG_Height,24UL},
    {TAG_DONE,0}
};

struct TagItem IconRenameTag[]=
{
    {TIMG_Image,(ULONG)IconsFileData},
    {TIMG_ColorMapRGB,(ULONG)IconsFilePalette},
    {TIMG_PosX,96UL},
    {TIMG_Width,24UL},
    {TIMG_Height,24UL},
    {TAG_DONE,0}
};

struct TagItem IconDeleteTag[]=
{
    {TIMG_Image,(ULONG)IconsFileData},
    {TIMG_ColorMapRGB,(ULONG)IconsFilePalette},
    {TIMG_PosX,120UL},
    {TIMG_Width,24UL},
    {TIMG_Height,24UL},
    {TAG_DONE,0}
};

struct TagItem IconDownloadTag[]=
{
    {TIMG_Image,(ULONG)IconsFileData},
    {TIMG_ColorMapRGB,(ULONG)IconsFilePalette},
    {TIMG_PosX,144UL},
    {TIMG_Width,24UL},
    {TIMG_Height,24UL},
    {TAG_DONE,0}
};

struct TagItem IconUploadTag[]=
{
    {TIMG_Image,(ULONG)IconsFileData},
    {TIMG_ColorMapRGB,(ULONG)IconsFilePalette},
    {TIMG_PosX,168UL},
    {TIMG_Width,24UL},
    {TIMG_Height,24UL},
    {TAG_DONE,0}
};

extern struct CustomClass ToolBarClassList[];

struct CustomGadget FileTransferGadgetList[]=
{
    {LISTVIEW_KIND,TAGID_LOCAL,FileTransferListTag,  LISTER1_X,20,LISTER_W,LISTER_H,0,0,0,0,(APTR)&FileTransferTextGadgetAttr,0,NULL,NULL},
    {LISTVIEW_KIND,TAGID_REMOTE,FileTransferListTag, LISTER2_X,20,LISTER_W,LISTER_H,0,0,0,0,(APTR)&FileTransferTextGadgetAttr,0,NULL,NULL},

    {GENERIC_KIND,TAGID_ROOT1,IconHomeTag,           0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_ROOT,"Back to the root folder",NULL},
    {GENERIC_KIND,TAGID_MKDIR1,IconMkDirTag,         0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_MKDIR,"Create a new folder",NULL},
    {GENERIC_KIND,TAGID_RENAME1,IconRenameTag,       0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_RENAME,"Rename file",NULL},
    {GENERIC_KIND,TAGID_DELETE1,IconDeleteTag,       0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_DELETE,"Delete file",NULL},
    {GENERIC_KIND,TAGID_PARENT1,IconParentTag,       0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_PARENT,"Back to parent folder",NULL},
    {GENERIC_KIND,TAGID_REFRESH1,IconRefreshTag,     0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_REFRESH,"Refresh list",NULL},
    {GENERIC_KIND,TAGID_DOWNLOAD1,IconUploadTag,     0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_UPLOAD,"Send to server",NULL},

    {GENERIC_KIND,TAGID_ROOT2,IconHomeTag,           0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_ROOT,"Back to the root folder",NULL},
    {GENERIC_KIND,TAGID_MKDIR2,IconMkDirTag,         0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_MKDIR,"Create a new folder",NULL},
    {GENERIC_KIND,TAGID_RENAME2,IconRenameTag,       0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_RENAME,"Rename file",NULL},
    {GENERIC_KIND,TAGID_DELETE2,IconDeleteTag,       0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_DELETE,"Delete file",NULL},
    {GENERIC_KIND,TAGID_PARENT2,IconParentTag,       0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_PARENT,"Back to parent folder",NULL},
    {GENERIC_KIND,TAGID_REFRESH2,IconRefreshTag,     0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_REFRESH,"Refresh list",NULL},
    {GENERIC_KIND,TAGID_DOWNLOAD2,IconDownloadTag,   0,LISTER_OPTION_Y,LISTER_OPTION_W,LISTER_OPTION_H,0,0,0,0,(APTR)&ToolBarClassList[0],LOC_FT_TOOLTIP_DOWNLOAD,"Download to local",NULL},

    {LISTVIEW_KIND,TAGID_HISTORY,HistoryListTag,     HISTORY_X,HISTORY_Y,HISTORY_W,HISTORY_H,0,0,0,0,(APTR)&FileTransferTextGadgetAttr,-1,NULL,NULL},

    {BUTTON_KIND,TAGID_ABORT,FileTransferNoTag,      WIN_WIDTH-95,WIN_HEIGHT-40,90,16,0,0,0,0,(APTR)&FileTransferAbortGadgetAttr,-1,NULL,NULL},

    {BUTTON_KIND,TAGID_HIDE,FileTransferNoTag,       5,WIN_HEIGHT-20,90,16,0,0,0,0,(APTR)&FileTransferHideGadgetAttr,-1,NULL,NULL},

    {KIND_DONE}
};


/***** Définitions des motifs */
struct CustMotifAttr GroupFileSystem={LOC_FT_LOCALFS_REMOTEFS,"Local file system <-> Remote file system",&FileTransferStdTextAttr};
struct CustMotifAttr GroupHistory={LOC_FT_HISTORY,"History",&FileTransferStdTextAttr};

struct CustomMotif FileTransferMotifList[]=
{
    {MOTIF_BEVELBOX_DUAL,5,10,WIN_WIDTH-10,LISTER_OPTION_Y+LISTER_OPTION_H-1,0,0,0,0,(APTR)&GroupFileSystem},
    {MOTIF_BEVELBOX_DUAL,5,HISTORY_Y-10,WIN_WIDTH-10,HISTORY_H+15,0,0,0,0,(APTR)&GroupHistory},
    {MOTIF_BEVELBOX_RECESSED,5,WIN_HEIGHT-40,WIN_WIDTH-105,16,0,0,0,0,(APTR)NULL},
    {MOTIF_DONE}
};


/*****
    Ouverture d'une fenêtre de transfert de fichier
*****/

BOOL Win_OpenWindowFileTransfer(
    struct WindowFileTransfer *wft,
    struct Window *ParentWindow,
    struct FontDef *fd,
    struct MsgPort *MPort,
    ULONG ClientCaps)
{
    BOOL IsSuccess=FALSE,IsLocked;
    ULONG VerticalRatio=0;
    struct Screen *Scr=Gal_GetBestPublicScreen(ParentWindow,&IsLocked,&VerticalRatio);

    if(Scr!=NULL)
    {
        LONG WinWidth=WIN_WIDTH,WinHeight=WIN_HEIGHT;

        wft->ParentWindow=ParentWindow;
        wft->ListHook.h_Entry=(HOOKFUNC)&ListHook;
        wft->ListHook.h_SubEntry=NULL;
        wft->ListHook.h_Data=(APTR)wft;
        FileTransferListTag[0].ti_Data=(ULONG)&wft->ListHook;

        Gui_GetBestWindowSize(&WinWidth,&WinHeight,VerticalRatio);
        Win_InitWindowFileTransferGadgets(fd,VerticalRatio);

        IsSuccess=TRUE;
        if(Disp_GetScreenType(Scr)==SCRTYPE_GRAPHICS)
        {
            /* On alloue des couleurs sur un écran 8 bits ou moins */
            wft->ColorID=Gui_AllocPalettes(Scr->ViewPort.ColorMap,(ULONG *)IconsFilePalette,sizeof(IconsFilePalette)/sizeof(ULONG));
            if(wft->ColorID==NULL) IsSuccess=FALSE;
        }

        if(IsSuccess)
        {
            IsSuccess=Gui_OpenCompleteWindow(
                &wft->Window,Scr,MPort,
                __APPNAME__,Sys_GetString(LOC_FT_TITLE,"File transfer"),
                -1,-1,WinWidth,WinHeight,
                FileTransferGadgetList,FileTransferMotifList,
                wft->ColorID);

            if(IsSuccess)
            {
                struct Window *win=wft->Window.WindowBase;
                ModifyIDCMP(win,win->IDCMPFlags|IDCMP_MOUSEBUTTONS);

                /* Rafraîchissement de la liste locale */
                IsSuccess=Win_SetLocalList(wft,wft->LocalFileManager.CurrentPath,NULL);

                /* Rafraîchissement de la liste distante */
                Win_EndRemoteFileManagerRefresh(wft);

                Win_UpdateFileTransfertGadgetsAttr(wft,ClientCaps);
                Win_SetGaugeProgress(wft,wft->GaugeProgress);
            }
        }

        if(IsLocked) UnlockPubScreen(NULL,Scr);
    }

    if(!IsSuccess) Win_CloseWindowFileTransfer(wft);

    return IsSuccess;
}


/*****
    Fermeture de la fenêtre de transfert de fichier
*****/

void Win_CloseWindowFileTransfer(struct WindowFileTransfer *wft)
{
    struct Screen *Scr=NULL;

    if(wft->Window.WindowBase!=NULL) Scr=wft->Window.WindowBase->WScreen;

    /* Fermeture de la fenêtre */
    Gui_CloseCompleteWindow(&wft->Window);

    if(wft->ColorID!=NULL && Scr!=NULL)
    {
        Gui_FreePalettes(Scr->ViewPort.ColorMap,wft->ColorID);
        wft->ColorID=NULL;
    }
}


/*****
    Traitement des événements de la fenêtre de statut
*****/

LONG Win_ManageWindowFileTransferMessages(struct WindowFileTransfer *wft, struct IntuiMessage *msg)
{
    LONG Result=WIN_FT_RESULT_NONE;
    struct Window *win=wft->Window.WindowBase;

    GT_RefreshWindow(win,NULL);

    switch(msg->Class)
    {
        case IDCMP_GADGETUP:
            {
                LONG GadID=((struct Gadget *)msg->IAddress)->GadgetID;

                switch(GadID)
                {
                    case TAGID_LOCAL:
                        {
                            struct FileItemNode *fi=Fmg_GetFileItemNodeFromIndex(&wft->LocalFileManager,(ULONG)msg->Code);

                            if(Win_CheckDoubleClick(wft,(LONG)fi) && fi!=NULL)
                            {
                                if(fi->Flags==FMNG_FLAG_DIR)
                                {
                                    Result=WIN_FT_RESULT_ERROR;
                                    if(Fmg_AppendToCurrentPath(&wft->LocalFileManager,fi->Node.ln_Name,FALSE))
                                    {
                                        BOOL IsBadPath;
                                        if(Win_SetLocalList(wft,wft->LocalFileManager.CurrentPath,&IsBadPath)) Result=WIN_FT_RESULT_NONE;
                                        if(IsBadPath) Fmg_ParentCurrentPath(&wft->LocalFileManager,FALSE);
                                    }
                                }
                                //fi->IsSelected=!fi->IsSelected;
                            }
                        }
                        break;

                    case TAGID_PARENT1:
                        Fmg_ParentCurrentPath(&wft->LocalFileManager,FALSE);
                        if(!Win_SetLocalList(wft,wft->LocalFileManager.CurrentPath,NULL)) Result=WIN_FT_RESULT_ERROR;
                        break;

                    case TAGID_ROOT1:
                        Result=WIN_FT_RESULT_ERROR;
                        if(Fmg_RootCurrentPath(&wft->LocalFileManager,FALSE))
                        {
                            if(Win_SetLocalList(wft,wft->LocalFileManager.CurrentPath,NULL)) Result=WIN_FT_RESULT_NONE;
                        }
                        break;

                    case TAGID_REFRESH1:
                        if(!Win_SetLocalList(wft,wft->LocalFileManager.CurrentPath,NULL)) Result=WIN_FT_RESULT_ERROR;
                        break;

                    case TAGID_DOWNLOAD1:
                        Result=WIN_FT_RESULT_DOWNLOAD1;
                        break;

                    case TAGID_REMOTE:
                        {
                            struct FileItemNode *fi=Fmg_GetFileItemNodeFromIndex(&wft->RemoteFileManager,(ULONG)msg->Code);

                            if(Win_CheckDoubleClick(wft,(LONG)fi) && fi!=NULL)
                            {
                                if(fi->Flags==FMNG_FLAG_DIR)
                                {
                                    if(Fmg_AppendToCurrentPath(&wft->RemoteFileManager,fi->Node.ln_Name,TRUE)) Result=WIN_FT_RESULT_REFRESH;
                                    else Result=WIN_FT_RESULT_ERROR;
                                }
                                //fi->IsSelected=!fi->IsSelected;
                            }
                        }
                        break;

                    case TAGID_PARENT2:
                        Fmg_ParentCurrentPath(&wft->RemoteFileManager,TRUE);
                        Result=WIN_FT_RESULT_REFRESH;
                        break;

                    case TAGID_ROOT2:
                        if(Fmg_RootCurrentPath(&wft->RemoteFileManager,TRUE)) Result=WIN_FT_RESULT_REFRESH;
                        else Result=WIN_FT_RESULT_ERROR;
                        break;

                    case TAGID_REFRESH2:
                        Result=WIN_FT_RESULT_REFRESH;
                        break;

                    case TAGID_DOWNLOAD2:
                        Result=WIN_FT_RESULT_DOWNLOAD2;
                        break;

                    case TAGID_DELETE2:
                        Result=WIN_FT_RESULT_DELETE;
                        break;

                    case TAGID_HIDE:
                        Result=WIN_FT_RESULT_CLOSE;
                        break;
                }
            }
            break;

        case IDCMP_MOUSEBUTTONS:
            if(msg->Code==MENUDOWN)
            {
                struct Gadget *Gadget1Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_LOCAL);
                struct Gadget *Gadget2Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_REMOTE);

                if(Gui_IsGadgetHit(Gadget2Ptr,msg->MouseX,msg->MouseY))
                {
                    Fmg_ParentCurrentPath(&wft->RemoteFileManager,TRUE);
                    Result=WIN_FT_RESULT_REFRESH;
                }
                else if(Gui_IsGadgetHit(Gadget1Ptr,msg->MouseX,msg->MouseY))
                {
                    Fmg_ParentCurrentPath(&wft->LocalFileManager,FALSE);
                    if(!Win_SetLocalList(wft,wft->LocalFileManager.CurrentPath,NULL)) Result=WIN_FT_RESULT_ERROR;
                }
            }
            break;

        case IDCMP_GADGETHELP:
            {
                struct Gadget *GadgetPtr=(struct Gadget *)msg->IAddress;
                char *Title=wft->Window.WindowTitle;

                if(GadgetPtr!=NULL && GadgetPtr->UserData!=NULL) Title=(char *)GadgetPtr->UserData;
                TVNC_SetWindowTitles(win,Title,NULL);
            }
            break;

        case IDCMP_CLOSEWINDOW:
            Result=WIN_FT_RESULT_CLOSE;
            break;
    }

    return Result;
}


/*****
    Permet d'obtenir un pointeur sécurisé sur la liste des fichiers distants
*****/

struct FileManager *Win_BeginRemoteFileManagerRefresh(struct WindowFileTransfer *wft)
{
    struct Window *win=wft->Window.WindowBase;

    if(win!=NULL)
    {
        struct Gadget *GadgetPtr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_REMOTE);

        if(GadgetPtr!=NULL)
        {
            GT_SetGadgetAttrs(GadgetPtr,win,NULL,GTLV_Labels,~0,TAG_DONE);
        }
    }

    return &wft->RemoteFileManager;
}


/*****
    Termine le traitement de la liste des fichiers distants
*****/

void Win_EndRemoteFileManagerRefresh(struct WindowFileTransfer *wft)
{
    struct Window *win=wft->Window.WindowBase;
    struct Gadget *GadgetPtr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_REMOTE);

    if(GadgetPtr!=NULL && win!=NULL)
    {
        GT_SetGadgetAttrs(GadgetPtr,win,NULL,
            GTLV_Labels,&wft->RemoteFileManager.FileItemList,
            GTLV_Selected,~0,
            TAG_DONE);
        GT_RefreshWindow(win,NULL);
    }
}


/*****
    Retourne le nom de fichier actuellement sélectionné sur le serveur distant
*****/

const struct FileItemNode *Win_GetRemoteFileInfo(struct WindowFileTransfer *wft)
{
    ULONG Idx=~0;
    struct Gadget *Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_REMOTE);

    GT_GetGadgetAttrs(Ptr,NULL,NULL,GTLV_Selected,&Idx,TAG_DONE);

    if(Idx!=~0)
    {
        return Fmg_GetFileItemNodeFromIndex(&wft->RemoteFileManager,Idx);
    }

    return NULL;
}


/*****
    Permet d'obtenir un pointeur sécurisé sur la liste des fichiers locaux
*****/

struct FileManager *Win_BeginLocalFileManagerRefresh(struct WindowFileTransfer *wft)
{
    struct Window *win=wft->Window.WindowBase;

    if(win!=NULL)
    {
        struct Gadget *GadgetPtr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_LOCAL);

        if(GadgetPtr!=NULL)
        {
            GT_SetGadgetAttrs(GadgetPtr,win,NULL,GTLV_Labels,~0,TAG_DONE);
        }
    }

    return &wft->LocalFileManager;
}


/*****
    Termine le traitement de la liste des fichiers locaux
*****/

void Win_EndLocalFileManagerRefresh(struct WindowFileTransfer *wft)
{
    struct Window *win=wft->Window.WindowBase;
    struct Gadget *GadgetPtr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_LOCAL);

    if(GadgetPtr!=NULL && win!=NULL)
    {
        GT_SetGadgetAttrs(GadgetPtr,win,NULL,
            GTLV_Labels,&wft->LocalFileManager.FileItemList,
            GTLV_Selected,~0,
            TAG_DONE);
        GT_RefreshWindow(win,NULL);
    }
}


/*****
    Retourne le nom de fichier actuellement sélectionné en local
*****/

const struct FileItemNode *Win_GetLocalFileInfo(struct WindowFileTransfer *wft)
{
    ULONG Idx=~0;
    struct Gadget *Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_LOCAL);

    GT_GetGadgetAttrs(Ptr,NULL,NULL,GTLV_Selected,&Idx,TAG_DONE);

    if(Idx!=~0)
    {
        return Fmg_GetFileItemNodeFromIndex(&wft->LocalFileManager,Idx);
    }

    return NULL;
}


/*****
    Mise à jour de la jauge de transfert
*****/

void Win_SetGaugeProgress(struct WindowFileTransfer *wft, ULONG Perthousand)
{
    struct Window *win=wft->Window.WindowBase;
    struct CustomMotif *MotifPtr=Gui_GetCustomMotifPtr(FileTransferMotifList,2);

    wft->GaugeProgress=Perthousand;
    if(win!=NULL && MotifPtr!=NULL)
    {
        struct RastPort *rp=win->RPort;
        UWORD Left=win->BorderLeft+MotifPtr->Left+2;
        UWORD Top=win->BorderTop+MotifPtr->Top+1;
        UWORD Right=Left+MotifPtr->Width-5;
        UWORD Bottom=Top+MotifPtr->Height-3;
        UWORD RightProp=(Right-Left+1)*Perthousand/1000+Left;

        if(Perthousand>0)
        {
            SetAPen(rp,3);
            RectFill(rp,Left,Top,RightProp,Bottom);
        }

        if(RightProp<Right)
        {
            SetAPen(rp,0);
            RectFill(rp,RightProp+1,Top,Right,Bottom);
        }
    }
}


/*****
*****/

BOOL Win_RefreshFileList(struct WindowFileTransfer *wft)
{
    Win_SetLocalList(wft,wft->LocalFileManager.CurrentPath,NULL);
    return TRUE;
}


/*****
    Initialisation des proportions des gadgets en fonctons des ressources
*****/

void Win_InitWindowFileTransferGadgets(struct FontDef *fd, ULONG VerticalRatio)
{
    static const ULONG GadgetIDOperationList1[]={TAGID_ROOT1,TAGID_PARENT1,TAGID_REFRESH1,TAGID_MKDIR1,TAGID_RENAME1,TAGID_DELETE1,TAGID_DOWNLOAD1,(ULONG)-1};
    static const ULONG GadgetIDOperationList2[]={TAGID_DOWNLOAD2,TAGID_DELETE2,TAGID_RENAME2,TAGID_MKDIR2,TAGID_REFRESH2,TAGID_PARENT2,TAGID_ROOT2,(ULONG)-1};
    static const ULONG GadgetIDWindowList[]={TAGID_HIDE,(ULONG)-1};

    Gui_CopyObjectPosition(FileTransferGadgetList,FileTransferMotifList,VerticalRatio);
    Gui_InitTextAttr(fd,&FileTransferStdTextAttr,FS_NORMAL);

    Gui_SpreadGadgetsHorizontaly(LISTER1_X,230,FileTransferGadgetList,GadgetIDOperationList1);
    Gui_SpreadGadgetsHorizontaly(LISTER2_X+LISTER_W-230,230,FileTransferGadgetList,GadgetIDOperationList2);

    Gui_DisposeGadgetsHorizontaly(0,WIN_WIDTH,FileTransferGadgetList,GadgetIDWindowList,TRUE);
}


/*****
    Mise à jour des gadgets en fonction des paramètres
*****/

void Win_UpdateFileTransfertGadgetsAttr(struct WindowFileTransfer *wft, ULONG ClientCaps)
{
    struct Window *win=wft->Window.WindowBase;
    struct Gadget *Ptr;

    Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_MKDIR1);
    SetGadgetAttrs(Ptr,wft->Window.WindowBase,NULL,GA_Disabled,TRUE,TAG_DONE);

    Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_DELETE1);
    SetGadgetAttrs(Ptr,wft->Window.WindowBase,NULL,GA_Disabled,TRUE,TAG_DONE);

    Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_RENAME1);
    SetGadgetAttrs(Ptr,wft->Window.WindowBase,NULL,GA_Disabled,TRUE,TAG_DONE);

    //Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_DOWNLOAD1);
    //SetGadgetAttrs(Ptr,wft->Window.WindowBase,NULL,GA_Disabled,TRUE,TAG_DONE);

    Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_MKDIR2);
    SetGadgetAttrs(Ptr,wft->Window.WindowBase,NULL,GA_Disabled,TRUE,TAG_DONE);

    if(!(ClientCaps&FLG_TIGHT2_REMOVE_REQUEST))
    {
        Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_DELETE2);
        SetGadgetAttrs(Ptr,wft->Window.WindowBase,NULL,GA_Disabled,TRUE,TAG_DONE);
    }

    Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_RENAME2);
    SetGadgetAttrs(Ptr,wft->Window.WindowBase,NULL,GA_Disabled,TRUE,TAG_DONE);

    //Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_DOWNLOAD2);
    //SetGadgetAttrs(Ptr,wft->Window.WindowBase,NULL,GA_Disabled,TRUE,TAG_DONE);

    Ptr=Gui_GetGadgetPtr(FileTransferGadgetList,TAGID_ABORT);
    SetGadgetAttrs(Ptr,wft->Window.WindowBase,NULL,GA_Disabled,TRUE,TAG_DONE);

    RefreshGadgets(wft->Window.GadgetsBase->FirstGadget,win,NULL);
    GT_RefreshWindow(win,NULL);
}


/*****
    Initialisation de la liste des fichiers en local
*****/

BOOL Win_SetLocalList(struct WindowFileTransfer *wft, const char *DirPath, BOOL *IsBadPath)
{
    BOOL Result=TRUE;
    struct FileManager *fm=Win_BeginLocalFileManagerRefresh(wft);

    if(IsBadPath!=NULL) *IsBadPath=FALSE;

    if(DirPath==NULL || Sys_StrCmp(DirPath,"")==0)
    {
        struct DosList *dl=LockDosList(LDF_DEVICES|LDF_READ);

        Fmg_FlushFileManagerResources(fm,FALSE);
        while((dl=NextDosEntry(dl,LDF_DEVICES))!=NULL)
        {
            if(dl->dol_Task!=NULL)
            {
                const char *Name=BADDR(dl->dol_Name);
                ULONG Size64[2]={0,0};
                LONG i;

                for(i=0; i<(LONG)(UBYTE)Name[0]; i++) wft->FileItemTextTmp[i]=Name[i+1];
                wft->FileItemTextTmp[i++]=':';
                wft->FileItemTextTmp[i]=0;
                if(!Fmg_AddItemToFileList(fm,wft->FileItemTextTmp,Size64,0,FMNG_FLAG_DIR)) Result=FALSE;
            }
        }

        UnLockDosList(LDF_DEVICES|LDF_READ);
    }
    else
    {
        BPTR lock=Lock((STRPTR)DirPath,ACCESS_READ);

        if(lock!=NULL)
        {
            struct FileInfoBlock *fib=(struct FileInfoBlock *)AllocDosObjectTags(DOS_FIB,TAG_DONE);

            if(fib!=NULL)
            {
                BPTR oldlock=CurrentDir(lock);

                Fmg_FlushFileManagerResources(fm,FALSE);
                if(Examine(lock,fib))
                {
                    while(ExNext(lock,fib))
                    {
                        struct DateStamp *ds=&fib->fib_Date;
                        LONG Time=ds->ds_Days*24*60*60+ds->ds_Minute*60+ds->ds_Tick/TICKS_PER_SECOND;
                        ULONG Size64[2];

                        Size64[0]=0;
                        Size64[1]=fib->fib_Size;
                        if(!Fmg_AddItemToFileList(fm,
                            fib->fib_FileName,
                            Size64,
                            Time,
                            fib->fib_DirEntryType>0?FMNG_FLAG_DIR:FMNG_FLAG_FILE)) Result=FALSE;
                    }
                }

                CurrentDir(oldlock);
                FreeDosObject(DOS_FIB,(void *)fib);
            }

            UnLock(lock);
        } else if(IsBadPath!=NULL) *IsBadPath=TRUE;
    }

    /* Post prod + Réaffectation de la liste dans le gadget list */
    Fmg_SortFileItemList(fm);
    Win_EndLocalFileManagerRefresh(wft);

    return Result;
}


/*****
    Vérification du double click
*****/

BOOL Win_CheckDoubleClick(struct WindowFileTransfer *wft, LONG Key)
{
    BOOL IsDoubleClick=FALSE;
    ULONG CurrentSecs,CurrentMicros;

    CurrentTime(&CurrentSecs,&CurrentMicros);
    if((void *)Key==wft->LastFIN) IsDoubleClick=DoubleClick(wft->LastSeconds,wft->LastMicros,CurrentSecs,CurrentMicros);
    wft->LastSeconds=CurrentSecs;
    wft->LastMicros=CurrentMicros;
    wft->LastFIN=(void *)Key;

    return IsDoubleClick;
}


/*****
    Hook pour la gestion des listes
*****/

M_HOOK_FUNC(ListHook, struct Hook *hook, struct Node *NodePtr, struct LVDrawMsg *dm)
{
    ULONG Result=LVCB_UNKNOWN;

    if(dm->lvdm_MethodID==LV_DRAW)
    {
        struct WindowFileTransfer *wft=(struct WindowFileTransfer *)hook->h_Data;
        struct RastPort *rp=dm->lvdm_RastPort;
        struct FileItemNode *fi=(struct FileItemNode *)NodePtr;
        struct IntuiText Text;
        struct TextExtent te;
        char *SupInfoText=wft->FileItemTextTmp;
        LONG SupInfoMinX;

        /* Effacement de la zone */
        if(dm->lvdm_State==LVR_SELECTED) SetAPen(rp,3); else SetAPen(rp,0);
        RectFill(rp,dm->lvdm_Bounds.MinX,dm->lvdm_Bounds.MinY,dm->lvdm_Bounds.MaxX,dm->lvdm_Bounds.MaxY);

        /* On détermine la couleur du texte et le supplément d'information
           à mettre en fonction du type de l'item.
        */
        if(fi->Flags==FMNG_FLAG_DIR)
        {
            Text.FrontPen=2;
            Sys_StrCopy(SupInfoText,Sys_GetString(LOC_FT_FOLDER,"Folder"),sizeof(wft->FileItemTextTmp));
        }
        else
        {
            long UnitPart=(long)fi->Size64[1],DecPart=0;
            char Tmp[32]="";
            const char *Measure="";

            Text.FrontPen=1;
            /* Si au moins 1 Go */
            if(fi->Size64[0]>0 || fi->Size64[1]>=1*1024*1024*1024)
            {
                /* Affichage des Gigas */
                UnitPart=(long)((fi->Size64[1]>>30)+(fi->Size64[0]<<2));
                DecPart=(((fi->Size64[1]>>14)&0xffff)*10)/0x10000;
                Measure=Sys_GetString(LOC_FT_MES_GIGA,"GB");
            }
            /* Si au moins 1 Mo */
            else if(fi->Size64[1]>=1*1024*1024)
            {
                /* Affichage des Mégas */
                UnitPart=(long)(fi->Size64[1]>>20);
                DecPart=(((fi->Size64[1]>>4)&0xffff)*10)/0x10000;
                Measure=Sys_GetString(LOC_FT_MES_MEGA,"MB");
            }
            /* Si au moins 1 Ko */
            else if(fi->Size64[1]>=1*1024)
            {
                /* Affichage des Kilos */
                UnitPart=(long)(fi->Size64[1]>>10);
                DecPart=(((fi->Size64[1]>>2)&0xff)*10)/0x100;
                Measure=Sys_GetString(LOC_FT_MES_KILO,"KB");
            }
            if(DecPart>0) Sys_SPrintf(Tmp,".%ld",DecPart);
            Sys_SPrintf(SupInfoText,"%ld%s%s",UnitPart,Tmp,Measure);
        }

        /* Initialisation diverses... */
        Text.BackPen=0;
        Text.DrawMode=JAM1;
        Text.LeftEdge=0;
        Text.TopEdge=0;
        Text.ITextFont=&FileTransferStdTextAttr;
        Text.NextText=NULL;

        /* Affichage du supplément d'information */
        {
            LONG SupInfoLen=Sys_StrLen(SupInfoText);
            LONG PixLen=(LONG)TextLength(rp,SupInfoText,SupInfoLen);

            SupInfoMinX=(LONG)dm->lvdm_Bounds.MaxX-PixLen+1;
            if(SupInfoMinX<(LONG)dm->lvdm_Bounds.MinX) SupInfoMinX=(LONG)dm->lvdm_Bounds.MinX;

            SupInfoLen=(LONG)TextFit(
                rp,SupInfoText,SupInfoLen,&te,NULL,1,
                dm->lvdm_Bounds.MaxX-SupInfoMinX+1,
                dm->lvdm_Bounds.MaxY-dm->lvdm_Bounds.MinY+1);

            SupInfoText[SupInfoLen]=0;
            Text.IText=SupInfoText;
            PrintIText(rp,&Text,SupInfoMinX,dm->lvdm_Bounds.MinY);
        }

        /* Affichage du texte de l'item */
        {
            LONG ItemLen=Sys_StrLen(NodePtr->ln_Name);
            LONG ItemMaxX=SupInfoMinX-8;
            LONG NewLen;

            if(ItemMaxX<(LONG)dm->lvdm_Bounds.MinX) ItemMaxX=(LONG)dm->lvdm_Bounds.MinX;

            NewLen=(LONG)TextFit(
                rp,NodePtr->ln_Name,ItemLen,&te,NULL,1,
                ItemMaxX-dm->lvdm_Bounds.MinX+1,
                dm->lvdm_Bounds.MaxY-dm->lvdm_Bounds.MinY+1);

            if(NewLen>=ItemLen) Text.IText=NodePtr->ln_Name;
            else
            {
                if(NewLen>sizeof(wft->FileItemTextTmp)) NewLen=sizeof(wft->FileItemTextTmp);
                Sys_StrCopy(wft->FileItemTextTmp,NodePtr->ln_Name,NewLen);
                Text.IText=wft->FileItemTextTmp;
                Text.IText[NewLen]=0;
            }

            PrintIText(rp,&Text,dm->lvdm_Bounds.MinX,dm->lvdm_Bounds.MinY);
        }

        Result=LVCB_OK;
    }

    return Result;
}
