#include "global.h"
#include "system.h"
#include "windowstatus.h"
#include "general.h"
#include "gui.h"
#include "twinvnc_const.h"


/*
    25-02-2007 (Seg)    Remaniements mineurs suite à l'ajout de la sauvegarde de la config
    19-02-2007 (Seg)    Reprototypage de quelques fonctions
    07-03-2006 (Seg)    Gestion des proportions de l'interface en fonction du ratio de l'écran
    17-05-2005 (Seg)    Localisation de la boîte
    11-05-2005 (Seg)    Gestion de la boîte de statut
*/


/***** Prototypes */
BOOL Win_OpenWindowStatus(struct WindowStatus *, struct Window *, struct FontDef *, struct MsgPort *);
void Win_CloseWindowStatus(struct WindowStatus *);
void Win_SetWindowStatusMessage(struct WindowStatus *, const char *, const char *);
LONG Win_ManageWindowStatusMessages(struct WindowStatus *, struct IntuiMessage *);

void InitWindowStatusGadgets(struct FontDef *, ULONG);


/***** Defines locaux */
#define WIN_WIDTH               380
#define WIN_HEIGHT              80
#define BUTTON_TEXT_WIDTH       (WIN_WIDTH-20)
#define BUTTON_TEXT_LEFT        ((WIN_WIDTH-BUTTON_TEXT_WIDTH)/2)
#define BUTTON_TEXT_TOP         20
#define BUTTON_CANCEL_WIDTH     90
#define BUTTON_CANCEL_LEFT      ((WIN_WIDTH-BUTTON_CANCEL_WIDTH)/2)
#define BUTTON_CANCEL_TOP       (WIN_HEIGHT-20)


/***** Variables globales */
struct TagItem StatusNoTag[]=
{
    {TAG_DONE,0UL}
};

struct TagItem StatusTextTag[]=
{
    {GTTX_Clipped,TRUE},
    {TAG_DONE,0UL}
};


/***** Paramètres des gadgets */
struct TextAttr StatusStdTextAttr={NULL,0,0,0};
struct CustGadgetAttr StatusTextGadgetAttr  ={0,NULL,&StatusStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr StatusCancelGadgetAttr={LOC_STATUS_CANCEL,"Cancel",&StatusStdTextAttr,PLACETEXT_IN};


/***** Liste des gadgets */
enum
{
    TAGID_TEXT1,
    TAGID_TEXT2,
    TAGID_CANCEL
};

struct CustomGadget StatusGadgetList[]=
{
    {TEXT_KIND,TAGID_TEXT1,StatusTextTag,BUTTON_TEXT_LEFT,BUTTON_TEXT_TOP,BUTTON_TEXT_WIDTH,12,0,0,0,0,(APTR)&StatusTextGadgetAttr,0,NULL,NULL},
    {TEXT_KIND,TAGID_TEXT2,StatusTextTag,BUTTON_TEXT_LEFT,BUTTON_TEXT_TOP+13,BUTTON_TEXT_WIDTH,12,0,0,0,0,(APTR)&StatusTextGadgetAttr,0,NULL,NULL},
    {BUTTON_KIND,TAGID_CANCEL,StatusNoTag,BUTTON_CANCEL_LEFT,BUTTON_CANCEL_TOP,BUTTON_CANCEL_WIDTH,16,0,0,0,0,(APTR)&StatusCancelGadgetAttr,0,NULL,NULL},
    {KIND_DONE}
};

struct CustomMotif StatusMotifList[]=
{
    {MOTIF_RECTALT_COLOR2,0,0,WIN_WIDTH,WIN_HEIGHT,0,0,0,0,NULL},
    {MOTIF_RECTFILL_COLOR0,6,5,WIN_WIDTH-10,WIN_HEIGHT-30,0,0,0,0,NULL},
    {MOTIF_BEVELBOX_RECESSED,6,5,WIN_WIDTH-10,WIN_HEIGHT-30,0,0,0,0,NULL},
    {MOTIF_DONE}
};


/*****
    Ouverture d'une fenêtre de statut
*****/

BOOL Win_OpenWindowStatus(struct WindowStatus *ws, struct Window *ParentWindow, struct FontDef *fd, struct MsgPort *MPort)
{
    BOOL IsSuccess=FALSE,IsLocked;
    ULONG VerticalRation=0;
    struct Screen *Scr=Gal_GetBestPublicScreen(ParentWindow,&IsLocked,&VerticalRation);

    if(Scr!=NULL)
    {
        LONG WinWidth=WIN_WIDTH,WinHeight=WIN_HEIGHT;

        Gui_GetBestWindowSize(&WinWidth,&WinHeight,VerticalRation);
        InitWindowStatusGadgets(fd,VerticalRation);

        IsSuccess=Gui_OpenCompleteWindow(
                &ws->Window,Scr,MPort,
                __APPNAME__,Sys_GetString(LOC_STATUS_TITLE,"Connection status..."),
                -1,-1,WinWidth,WinHeight,
                StatusGadgetList,StatusMotifList,NULL);

        if(IsLocked) UnlockPubScreen(NULL,Scr);
    }

    return IsSuccess;
}


/*****
    Fermeture de la fenêtre de statut
*****/

void Win_CloseWindowStatus(struct WindowStatus *ws)
{
    Gui_CloseCompleteWindow(&ws->Window);
}


/*****
    Permet de changer le message de statut
*****/

void Win_SetWindowStatusMessage(struct WindowStatus *ws, const char *MsgLine1, const char *MsgLine2)
{
    struct Window *win=ws->Window.WindowBase;

    if(win!=NULL)
    {
        GT_SetGadgetAttrs(Gui_GetGadgetPtr(StatusGadgetList,TAGID_TEXT1),win,NULL,
                GTTX_Text,(ULONG)(MsgLine1!=NULL?MsgLine1:""),
                GTTX_Justification,GTJ_CENTER,
                TAG_DONE);

        GT_SetGadgetAttrs(Gui_GetGadgetPtr(StatusGadgetList,TAGID_TEXT2),win,NULL,
                GTTX_Text,(ULONG)(MsgLine2!=NULL?MsgLine2:""),
                GTTX_Justification,GTJ_CENTER,
                TAG_DONE);
    }
}


/*****
    Traitement des événements de la fenêtre de statut
*****/

LONG Win_ManageWindowStatusMessages(struct WindowStatus *ws, struct IntuiMessage *msg)
{
    LONG Result=WIN_STATUS_RESULT_NONE;
    struct Window *win=ws->Window.WindowBase;

    GT_RefreshWindow(win,NULL);

    switch(msg->Class)
    {
        case IDCMP_GADGETUP:
            {
                ULONG GadID=((struct Gadget *)msg->IAddress)->GadgetID;
                if(GadID==TAGID_CANCEL) Result=WIN_STATUS_RESULT_CANCEL;
            }
            break;

        case IDCMP_GADGETHELP:
            /*
            {
                struct Gadget *GadgetPtr=(struct Gadget *)msg->IAddress;
                char *Title=ws->Window.WindowTitle;

                if(GadgetPtr!=NULL && GadgetPtr->UserData!=NULL) Title=(char *)GadgetPtr->UserData;
                TVNC_SetWindowTitles(win,Title,NULL);
            }
            */
            break;

        case IDCMP_CLOSEWINDOW:
            Result=WIN_STATUS_RESULT_CANCEL;
            break;
    }

    return Result;
}


/*****
    Initialisation des proportions des gadgets en fonctons des ressources
*****/

void InitWindowStatusGadgets(struct FontDef *fd, ULONG VerticalRation)
{
    static const ULONG GadgetIDList[]={TAGID_CANCEL,(ULONG)-1};

    Gui_CopyObjectPosition(StatusGadgetList,StatusMotifList,VerticalRation);
    Gui_InitTextAttr(fd,&StatusStdTextAttr,FS_NORMAL);

    Gui_DisposeGadgetsHorizontaly(0,WIN_WIDTH,StatusGadgetList,GadgetIDList,TRUE);
}
