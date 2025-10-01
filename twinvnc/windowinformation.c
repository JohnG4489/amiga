#include "global.h"
#include "system.h"
#include "windowinformation.h"
#include "general.h"
#include "gui.h"
#include "connect.h"
#include "twinvnc_const.h"


/*
    25-02-2007 (Seg)    Remaniements mineurs suite à l'ajout de la sauvegarde de la config
    16-04-2006 (Seg)    Gestion de la boîte d'information
*/


/***** Prototypes */
BOOL Win_OpenWindowInformation(struct WindowInformation *, struct Window *, struct Connect *, char *, struct FontDef *, struct MsgPort *);
void Win_CloseWindowInformation(struct WindowInformation *);
LONG Win_ManageWindowInformationMessages(struct WindowInformation *, struct IntuiMessage *);

void InitWindowInformationGadgets(struct FontDef *, ULONG);
void UpdateInformationGadgetsAttr(struct WindowInformation *);
const char *GetBooleanString(BOOL);


/***** Defines locaux */
#define WIN_WIDTH               400
#define WIN_HEIGHT              290
#define BUTTON_IDENTITY_LEFT    140
#define BUTTON_IDENTITY_TOP     25
#define BUTTON_IDENTITY_WIDTH   (WIN_WIDTH-BUTTON_IDENTITY_LEFT-30)
#define BUTTON_PIXF_LEFT        (BUTTON_IDENTITY_LEFT)
#define BUTTON_PIXF_TOP         130
#define BUTTON_PIXF_WIDTH       (BUTTON_IDENTITY_WIDTH)
#define BUTTON_CLOSE_WIDTH      90
#define BUTTON_CLOSE_LEFT       ((WIN_WIDTH-BUTTON_CLOSE_WIDTH)/2)
#define BUTTON_CLOSE_TOP        (WIN_HEIGHT-20)


/***** Variables globales */
struct TagItem InformationNoTag[]=
{
    {TAG_DONE,0UL}
};

struct TagItem InformationTextTag[]=
{
    {GTTX_Clipped,TRUE},
    {GTTX_Border,TRUE},
    {TAG_DONE,0UL}
};


/***** Paramètres des gadgets */
struct TextAttr InformationStdTextAttr={NULL,0,0,0};

enum
{
    GID_HOST,
    GID_IP,
    GID_PROTOCOL,
    GID_NAME,
    GID_BITPERPIXEL,
    GID_DEPTH,
    GID_BIGENDIAN,
    GID_TRUECOLOR,
    GID_RGBMAX,
    GID_RGBSHIFT,
    GID_CLOSE
};


/***** Définitions des gadgets */
struct CustGadgetAttr InformationEmptyGadgetAttr        ={-1,"",&InformationStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr InformationHostGadgetAttr         ={LOC_INFO_HOST,"Host",&InformationStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr InformationIPGadgetAttr           ={LOC_INFO_IP,"IP/Port",&InformationStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr InformationProtocolGadgetAttr     ={LOC_INFO_PROTOCOL,"Protocol",&InformationStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr InformationNameGadgetAttr         ={LOC_INFO_NAME,"Name",&InformationStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr InformationBitPerPixelGadgetAttr  ={LOC_INFO_BITPERPIXEL,"Bit per pixel",&InformationStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr InformationDepthGadgetAttr        ={LOC_INFO_DEPTH,"Depth",&InformationStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr InformationBigEndianGadgetAttr    ={LOC_INFO_BIGENDIAN,"Big endian",&InformationStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr InformationTrueColorGadgetAttr    ={LOC_INFO_TRUECOLOR,"True color",&InformationStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr InformationRGBMaxGadgetAttr       ={LOC_INFO_RGBMAX,"RGB max",&InformationStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr InformationRGBShiftGadgetAttr     ={LOC_INFO_RGBSHIFT,"RGB shift",&InformationStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr InformationCloseGadgetAttr        ={LOC_INFO_CLOSE,"Close",&InformationStdTextAttr,PLACETEXT_IN};

struct CustomGadget InformationGadgetList[]=
{
    {TEXT_KIND,GID_HOST,        InformationTextTag,BUTTON_IDENTITY_LEFT,BUTTON_IDENTITY_TOP+0*20,BUTTON_IDENTITY_WIDTH,16,0,0,0,0,(APTR)&InformationHostGadgetAttr,0,NULL,NULL},
    {TEXT_KIND,GID_IP,          InformationTextTag,BUTTON_IDENTITY_LEFT,BUTTON_IDENTITY_TOP+1*20,BUTTON_IDENTITY_WIDTH,16,0,0,0,0,(APTR)&InformationIPGadgetAttr,0,NULL,NULL},
    {TEXT_KIND,GID_NAME,        InformationTextTag,BUTTON_IDENTITY_LEFT,BUTTON_IDENTITY_TOP+2*20,BUTTON_IDENTITY_WIDTH,16,0,0,0,0,(APTR)&InformationNameGadgetAttr,0,NULL,NULL},
    {TEXT_KIND,GID_PROTOCOL,    InformationTextTag,BUTTON_IDENTITY_LEFT,BUTTON_IDENTITY_TOP+3*20,BUTTON_IDENTITY_WIDTH,16,0,0,0,0,(APTR)&InformationProtocolGadgetAttr,0,NULL,NULL},

    {TEXT_KIND,GID_BITPERPIXEL, InformationTextTag,BUTTON_PIXF_LEFT,BUTTON_PIXF_TOP+0*20,BUTTON_PIXF_WIDTH,16,0,0,0,0,(APTR)&InformationBitPerPixelGadgetAttr,0,NULL,NULL},
    {TEXT_KIND,GID_DEPTH,       InformationTextTag,BUTTON_PIXF_LEFT,BUTTON_PIXF_TOP+1*20,BUTTON_PIXF_WIDTH,16,0,0,0,0,(APTR)&InformationDepthGadgetAttr,0,NULL,NULL},
    {TEXT_KIND,GID_BIGENDIAN,   InformationTextTag,BUTTON_PIXF_LEFT,BUTTON_PIXF_TOP+2*20,BUTTON_PIXF_WIDTH,16,0,0,0,0,(APTR)&InformationBigEndianGadgetAttr,0,NULL,NULL},
    {TEXT_KIND,GID_TRUECOLOR,   InformationTextTag,BUTTON_PIXF_LEFT,BUTTON_PIXF_TOP+3*20,BUTTON_PIXF_WIDTH,16,0,0,0,0,(APTR)&InformationTrueColorGadgetAttr,0,NULL,NULL},
    {TEXT_KIND,GID_RGBMAX,      InformationTextTag,BUTTON_PIXF_LEFT,BUTTON_PIXF_TOP+4*20,BUTTON_PIXF_WIDTH,16,0,0,0,0,(APTR)&InformationRGBMaxGadgetAttr,0,NULL,NULL},
    {TEXT_KIND,GID_RGBSHIFT,    InformationTextTag,BUTTON_PIXF_LEFT,BUTTON_PIXF_TOP+5*20,BUTTON_PIXF_WIDTH,16,0,0,0,0,(APTR)&InformationRGBShiftGadgetAttr,0,NULL,NULL},

    {BUTTON_KIND,GID_CLOSE,     InformationNoTag,BUTTON_CLOSE_LEFT,BUTTON_CLOSE_TOP,BUTTON_CLOSE_WIDTH,16,0,0,0,0,(APTR)&InformationCloseGadgetAttr,0,NULL,NULL},
    {KIND_DONE}
};


/***** Définitions des motifs */
struct CustMotifAttr GroupIdentity={LOC_INFO_IDENTITY,"Server identity",&InformationStdTextAttr};
struct CustMotifAttr GroupPixelFormat={LOC_INFO_PIXELFORMAT,"Server pixel format",&InformationStdTextAttr};

struct CustomMotif InformationMotifList[]=
{
    {MOTIF_RECTALT_COLOR2,0,0,WIN_WIDTH,WIN_HEIGHT,0,0,0,0,NULL},
    {MOTIF_RECTFILL_COLOR0,6,5,WIN_WIDTH-10,WIN_HEIGHT-30,0,0,0,0,NULL},
    {MOTIF_BEVELBOX_RECESSED,6,5,WIN_WIDTH-10,WIN_HEIGHT-30,0,0,0,0,NULL},
    {MOTIF_BEVELBOX_DUAL,15,15,WIN_WIDTH-30,95,0,0,0,0,(APTR)&GroupIdentity},
    {MOTIF_BEVELBOX_DUAL,15,120,WIN_WIDTH-30,135,0,0,0,0,(APTR)&GroupPixelFormat},
    {MOTIF_DONE}
};


/*****
    Ouverture d'une fenêtre d'information
*****/

BOOL Win_OpenWindowInformation(struct WindowInformation *wi, struct Window *ParentWindow, struct Connect *co, char *Host, struct FontDef *fd, struct MsgPort *MPort)
{
    BOOL IsSuccess=FALSE,IsLocked;
    ULONG VerticalRatio=0;
    struct Screen *Scr=Gal_GetBestPublicScreen(ParentWindow,&IsLocked,&VerticalRatio);

    if(Scr!=NULL)
    {
        LONG WinWidth=WIN_WIDTH,WinHeight=WIN_HEIGHT;

        wi->Connect=co;
        wi->Host=Host;

        Gui_GetBestWindowSize(&WinWidth,&WinHeight,VerticalRatio);
        InitWindowInformationGadgets(fd,VerticalRatio);

        IsSuccess=Gui_OpenCompleteWindow(
                &wi->Window,Scr,MPort,
                __APPNAME__,Sys_GetString(LOC_INFO_TITLE,"Information"),
                -1,-1,WinWidth,WinHeight,
                InformationGadgetList,InformationMotifList,NULL);

        if(IsSuccess)
        {
            UpdateInformationGadgetsAttr(wi);
        }

        if(IsLocked) UnlockPubScreen(NULL,Scr);
    }

    return IsSuccess;
}


/*****
    Fermeture de la fenêtre d'information
*****/

void Win_CloseWindowInformation(struct WindowInformation *wi)
{
    Gui_CloseCompleteWindow(&wi->Window);
}


/*****
    Traitement des événements de la fenêtre d'information
*****/

LONG Win_ManageWindowInformationMessages(struct WindowInformation *wi, struct IntuiMessage *msg)
{
    LONG Result=WIN_INFORMATION_RESULT_NONE;
    struct Window *win=wi->Window.WindowBase;

    GT_RefreshWindow(win,NULL);

    switch(msg->Class)
    {
        case IDCMP_VANILLAKEY:
            switch(msg->Code)
            {
                case 13: /* Touche Entree */
                case 27: /* Touche Escape */
                    Result=WIN_INFORMATION_RESULT_CLOSE;
                    break;
            }
            break;

        case IDCMP_GADGETUP:
            {
                ULONG GadID=((struct Gadget *)msg->IAddress)->GadgetID;
                if(GadID==GID_CLOSE) Result=WIN_INFORMATION_RESULT_CLOSE;
            }
            break;

        case IDCMP_GADGETHELP:
            /*
            {
                struct Gadget *GadgetPtr=(struct Gadget *)msg->IAddress;
                char *Title=wi->Window.WindowTitle;

                if(GadgetPtr!=NULL && GadgetPtr->UserData!=NULL) Title=(char *)GadgetPtr->UserData;
                TVNC_SetWindowTitles(win,Title,NULL);
            }
            */
            break;

        case IDCMP_CLOSEWINDOW:
            Result=WIN_INFORMATION_RESULT_CLOSE;
            break;
    }

    return Result;
}


/*****
    Initialisation des proportions des gadgets en fonctons des ressources
*****/

void InitWindowInformationGadgets(struct FontDef *fd, ULONG VerticalRatio)
{
    static const ULONG GadgetIDList[]={GID_CLOSE,(ULONG)-1};

    Gui_CopyObjectPosition(InformationGadgetList,InformationMotifList,VerticalRatio);
    Gui_InitTextAttr(fd,&InformationStdTextAttr,FS_NORMAL);

    /* Décalage des gadgets en fonction de la longueur des textes à gauche
       des gadgets, par rapport à la bordure gauche
    */
    Gui_ShiftGadgetsHorizontaly(20,WIN_WIDTH-45,InformationGadgetList,NULL);

    /* Disposition du bouton Close */
    Gui_DisposeGadgetsHorizontaly(0,WIN_WIDTH,InformationGadgetList,GadgetIDList,TRUE);
}


/*****
    Mise à jour des champs
*****/

void UpdateInformationGadgetsAttr(struct WindowInformation *wi)
{
    struct Window *win=wi->Window.WindowBase;
    struct Connect *co=wi->Connect;
    struct PixelFormat *pf=&co->ServerPixelFormat;
    struct Gadget *Ptr;

    /* Groupe Identité */
    {
        long ip=(long)co->Socket->s_SockAddrIn.sin_addr.s_addr;
        long port=(long)co->Socket->s_SockAddrIn.sin_port;

        Ptr=Gui_GetGadgetPtr(InformationGadgetList,GID_HOST);
        GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,wi->Host,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(InformationGadgetList,GID_IP);
        Sys_SPrintf(wi->IP,"%ld.%ld.%ld.%ld:%ld",
                (long)(UBYTE)(ip>>24),
                (long)(UBYTE)(ip>>16),
                (long)(UBYTE)(ip>>8),
                (long)(UBYTE)ip,
                port);

        GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,wi->IP,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(InformationGadgetList,GID_NAME);
        GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,co->ServerName,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(InformationGadgetList,GID_PROTOCOL);
        Sys_SPrintf(wi->Protocol,"RFB %ld.%ld",(co->ServerVersion&0x3ff),(co->ServerRevision&0x3ff));
        GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,wi->Protocol,TAG_DONE);
    }

    /* Groupe Information sur le format du pixel */
    {
        Ptr=Gui_GetGadgetPtr(InformationGadgetList,GID_BITPERPIXEL);
        Sys_SPrintf(wi->BitPerPixel,"%ld",(long)pf->BitPerPixel);
        GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,wi->BitPerPixel,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(InformationGadgetList,GID_DEPTH);
        Sys_SPrintf(wi->Depth,"%ld",(long)pf->Depth);
        GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,wi->Depth,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(InformationGadgetList,GID_BIGENDIAN);
        Sys_StrCopy(wi->BigEndian,GetBooleanString((BOOL)pf->BigEndian),sizeof(wi->BigEndian));
        GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,wi->BigEndian,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(InformationGadgetList,GID_TRUECOLOR);
        Sys_StrCopy(wi->TrueColor,GetBooleanString((BOOL)pf->TrueColor),sizeof(wi->TrueColor));
        GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,wi->TrueColor,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(InformationGadgetList,GID_RGBMAX);
        Sys_SPrintf(wi->RGBMax,"%03ld/%03ld/%03ld",(long)pf->RedMax,(long)pf->GreenMax,(long)pf->BlueMax);
        GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,wi->RGBMax,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(InformationGadgetList,GID_RGBSHIFT);
        Sys_SPrintf(wi->RGBShift,"%03ld/%03ld/%03ld",(long)pf->RedShift,(long)pf->GreenShift,(long)pf->BlueShift);
        GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,wi->RGBShift,TAG_DONE);
    }
}


/*****
    Retourne la chaîne associée à la valeur du booléen, en fonction du catalog utilisé
*****/

const char *GetBooleanString(BOOL Flag)
{
    if(Flag)
    {
        return Sys_GetString(LOC_BOOLEAN_YES,"Yes");
    }

    return Sys_GetString(LOC_BOOLEAN_NO,"No");
}
