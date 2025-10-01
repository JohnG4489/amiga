#include "system.h"
#include "toolbar.h"
#include "twin.h"
#include "general.h"
#include "gui.h"
#include "icons.h"
#include "twingadgetclass.h"
#include "twinvnc_const.h"


/*
    06-01-2016 (Seg)    Renommage des fonctions
    16-11-2005 (Seg)    Restructurations en rapport avec l'affichage
    16-12-2004 (Seg)    Gestion de la toolbar
*/


/***** Prototypes */
BOOL TBar_CreateToolBar(struct Display *);
void TBar_FreeToolBar(struct Display *);
void TBar_ShowToolBar(struct Display *);
void TBar_HideToolBar(struct Display *);
void TBar_RefreshToolBar(struct Display *);
LONG TBar_GetToolBarWidth(void);
LONG TBar_GetToolBarHeight(void);
LONG TBar_ManageToolBarMessages(struct IntuiMessage *);


/***** Données */
enum TWIN_GAD_ID
{
    TGID_OPTIONS,
    TGID_INFO,
    TGID_FULLSCREEN,
    TGID_REFRESH,
    TGID_RECONNECT,
    TGID_TRANSFER,
    TGID_EXIT,
    TGID_ICONIFY,
    TGID_ABOUT,
};


struct CustomClass ToolBarClassList[]=
{
    {Tgc_MakeTwinGadgetClass,Tgc_FreeTwinGadgetClass,NULL},
    {NULL}
};


/* Liste des gadgets */
struct TagItem OptionsTag[]=
{
    {TIMG_Image,(ULONG)IconOptions},
    {TIMG_ColorMapRGB,(ULONG)PaletteOfIcons},
    {TAG_DONE,0}
};

struct TagItem InfoTag[]=
{
    {TIMG_Image,(ULONG)IconInfo},
    {TIMG_ColorMapRGB,(ULONG)PaletteOfIcons},
    {TAG_DONE,0}
};

struct TagItem FullScreenTag[]=
{
    {TIMG_Image,(ULONG)IconFullScreen},
    {TIMG_ColorMapRGB,(ULONG)PaletteOfIcons},
    {TAG_DONE,0}
};

struct TagItem RefreshTag[]=
{
    {TIMG_Image,(ULONG)IconRefresh},
    {TIMG_ColorMapRGB,(ULONG)PaletteOfIcons},
    {TAG_DONE,0}
};

struct TagItem ReconnectTag[]=
{
    {TIMG_Image,(ULONG)IconReconnect},
    {TIMG_ColorMapRGB,(ULONG)PaletteOfIcons},
    {TAG_DONE,0}
};

struct TagItem TransferTag[]=
{
    {TIMG_Image,(ULONG)IconTransfer},
    {TIMG_ColorMapRGB,(ULONG)PaletteOfIcons},
    {TAG_DONE,0}
};

struct TagItem ExitTag[]=
{
    {TIMG_Image,(ULONG)IconExit},
    {TIMG_ColorMapRGB,(ULONG)PaletteOfIcons},
    {TAG_DONE,0}
};

struct TagItem IconifyTag[]=
{
    {TIMG_Image,(ULONG)IconIconify},
    {TIMG_ColorMapRGB,(ULONG)PaletteOfIcons},
    {TAG_DONE,0}
};

struct TagItem TwinTag[]=
{
    {TIMG_Image,(ULONG)IconTwin},
    {TIMG_ColorMapRGB,(ULONG)PaletteOfIcons},
    {TAG_DONE,0}
};


struct CustomGadget ToolBarGadgetList[]=
{
    {GENERIC_KIND,TGID_OPTIONS,     OptionsTag,     4,2,28,28,0,0,0,0,   &ToolBarClassList[0], LOC_TOOLBAR_OPTIONS,     "Options", NULL},
    {GENERIC_KIND,TGID_INFO,        InfoTag,        33,2,28,28,0,0,0,0,  &ToolBarClassList[0], LOC_TOOLBAR_INFO,        "Information", NULL},
    {GENERIC_KIND,TGID_FULLSCREEN,  FullScreenTag,  64,2,28,28,0,0,0,0,  &ToolBarClassList[0], LOC_TOOLBAR_FULLSCREEN,  "Switch to full screen or window", NULL},
    {GENERIC_KIND,TGID_REFRESH,     RefreshTag,     93,2,28,28,0,0,0,0,  &ToolBarClassList[0], LOC_TOOLBAR_REFRESH,     "Refresh display", NULL},
    {GENERIC_KIND,TGID_TRANSFER,    TransferTag,    124,2,28,28,0,0,0,0, &ToolBarClassList[0], LOC_TOOLBAR_TRANSFER,    "File transfer", NULL},
    {GENERIC_KIND,TGID_RECONNECT,   ReconnectTag,   153,2,28,28,0,0,0,0, &ToolBarClassList[0], LOC_TOOLBAR_RECONNECT,   "Reconnect", NULL},
    {GENERIC_KIND,TGID_ICONIFY,     IconifyTag,     184,2,28,28,0,0,0,0, &ToolBarClassList[0], LOC_TOOLBAR_ICONIFY,     "Iconify", NULL},
    {GENERIC_KIND,TGID_EXIT,        ExitTag,        220,2,28,28,0,0,0,0, &ToolBarClassList[0], LOC_TOOLBAR_QUIT,        "Exit", NULL},
    {GENERIC_KIND,TGID_ABOUT,       TwinTag,        251,2,28,28,0,0,0,0, &ToolBarClassList[0], LOC_TOOLBAR_ABOUT,       "About", NULL},
    {KIND_DONE}
};


/*****
    Création de la toolbar
*****/

BOOL TBar_CreateToolBar(struct Display *disp)
{
    BOOL IsSuccess=TRUE;
    struct ViewInfo *vi=&disp->ViewInfo;
    struct Window *win=vi->WindowBase;

    if(vi->ScreenType==SCRTYPE_GRAPHICS)
    {
        /* On alloue des couleurs sur un écran 8 bits ou moins */
        disp->ToolBarColorID=Gui_AllocPalettes(win->WScreen->ViewPort.ColorMap,
                            (ULONG *)PaletteOfIcons,sizeof(PaletteOfIcons)/sizeof(ULONG));
        if(disp->ToolBarColorID==NULL) IsSuccess=FALSE;
    }

    if(IsSuccess)
    {
        Gui_CopyObjectPosition(ToolBarGadgetList,NULL,TRUE);

        disp->ToolBarG=Gui_CreateWindowGadgets(win,disp->VisualI,disp->ToolBarColorID,ToolBarGadgetList);
        if(disp->ToolBarG==NULL) IsSuccess=FALSE;
    }

    if(!IsSuccess) TBar_FreeToolBar(disp);

    return IsSuccess;
}


/*****
    Destruction de la toolbar
*****/

void TBar_FreeToolBar(struct Display *disp)
{
    TBar_HideToolBar(disp);

    if(disp->ToolBarG!=NULL)
    {
        Gui_FreeWindowGadgets(disp->ToolBarG);
        disp->ToolBarG=NULL;
    }

    if(disp->ToolBarColorID)
    {
        Gui_FreePalettes(disp->ViewInfo.WindowBase->WScreen->ViewPort.ColorMap,disp->ToolBarColorID);
        disp->ToolBarColorID=NULL;
    }
}


/*****
    Affichage de la toolbar
*****/

void TBar_ShowToolBar(struct Display *disp)
{
    if(disp->ToolBarG!=NULL)
    {
        struct Window *win=disp->ViewInfo.WindowBase;

        /* On rattache les gadgets de la toolbar à la fenêtre */
        Gui_AttachWindowGadgetsToWindow(win,disp->ToolBarG);
    }
}


/*****
    Disparition de la toolbar
*****/

void TBar_HideToolBar(struct Display *disp)
{
    if(disp->ToolBarG!=NULL)
    {
        Gui_RemoveWindowGadgetsFromWindow(disp->ViewInfo.WindowBase,disp->ToolBarG);
    }
}


/*****
    Pour afficher visuellement la toolbar
*****/

void TBar_RefreshToolBar(struct Display *disp)
{
    if(disp->IsToolBar && disp->ToolBarG!=NULL)
    {
        struct Window *win=disp->ViewInfo.WindowBase;
        LONG Height=TBar_GetToolBarHeight();

        SetAPen(win->RPort,0);
        RectFill(win->RPort,
            (LONG)win->BorderLeft,(LONG)win->BorderTop,
            (LONG)(win->Width-win->BorderRight-1),(LONG)win->BorderTop+Height-1);

        DrawBevelBox(win->RPort,
            (LONG)win->BorderLeft,(LONG)win->BorderTop,
            (LONG)(win->Width-win->BorderLeft-win->BorderRight),Height,
            GT_VisualInfo,disp->VisualI,
            GTBB_FrameType,BBFT_BUTTON,
            TAG_DONE);

        RefreshGList(disp->ToolBarG->FirstGadget,win,NULL,disp->ToolBarG->GadgetCount);
    }
}


/*****
    Pour obtenir la largeur de la ToolBar
*****/

LONG TBar_GetToolBarWidth(void)
{
    struct CustomGadget *Ptr=ToolBarGadgetList;
    LONG MinLeft=0x7fffffff,MaxRight=0;

    while(Ptr->KindID!=KIND_DONE)
    {
        if((LONG)Ptr->LeftBase<MinLeft) MinLeft=(LONG)Ptr->LeftBase;
        if((LONG)Ptr->LeftBase+(LONG)Ptr->WidthBase>MaxRight) MaxRight=(LONG)Ptr->LeftBase+(LONG)Ptr->WidthBase;
        Ptr++;
    }

    return (MaxRight-MinLeft+8);
}


/*****
    Pour obtenir la hauteur de la ToolBar
*****/

LONG TBar_GetToolBarHeight(void)
{
    struct CustomGadget *Ptr=ToolBarGadgetList;
    LONG MinTop=0x7fffffff,MaxBottom=0;

    while(Ptr->KindID!=KIND_DONE)
    {
        if((LONG)Ptr->TopBase<MinTop) MinTop=(LONG)Ptr->TopBase;
        if((LONG)Ptr->TopBase+(LONG)Ptr->HeightBase>MaxBottom) MaxBottom=(LONG)Ptr->TopBase+(LONG)Ptr->HeightBase;
        Ptr++;
    }

    return (MaxBottom-MinTop+4);
}


/*****
    Gestion des messages de la toolbar
*****/

LONG TBar_ManageToolBarMessages(struct IntuiMessage *msg)
{
    LONG ControlCode=GAL_CTRL_NONE;

    switch(msg->Class)
    {
        case IDCMP_GADGETUP:
            switch(((struct Gadget *)msg->IAddress)->GadgetID)
            {
                case TGID_OPTIONS:
                    ControlCode=GAL_CTRL_OPTIONS;
                    break;

                case TGID_INFO:
                    ControlCode=GAL_CTRL_INFORMATION;
                    break;

                case TGID_TRANSFER:
                    ControlCode=GAL_CTRL_FILE_TRANSFER;
                    break;

                case TGID_RECONNECT:
                    ControlCode=GAL_CTRL_RECONNECT;
                    break;

                case TGID_ABOUT:
                    ControlCode=GAL_CTRL_ABOUT;
                    break;

                case TGID_FULLSCREEN:
                    ControlCode=GAL_CTRL_SWITCH_VIEW;
                    break;

                case TGID_REFRESH:
                    ControlCode=GAL_CTRL_REFRESH;
                    break;

                case TGID_ICONIFY:
                    ControlCode=GAL_CTRL_ICONIFY;
                    break;

                case TGID_EXIT:
                    ControlCode=GAL_CTRL_EXIT;
                    break;
            }

    }

    return ControlCode;
}
