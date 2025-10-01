#include "global.h"
#include "system.h"
#include "windowoptions.h"
#include "twin.h"
#include "general.h"
#include "gui.h"
#include "config.h"
#include "twinvnc_const.h"
#include <intuition/sghooks.h>
#include <intuition/gadgetclass.h>


/*
    26-06-2016 (Seg)    Nouvelle gestion du CTRLKEY
    29-01-2016 (Seg)    Ajout de l'option de redéfinition de la touche de contrôle
    25-02-2007 (Seg)    Remaniements majeurs pour l'ajout de la sauvegarde de la config
    19-02-2007 (Seg)    Reprototypage de quelques fonctions
    04-12-2006 (Seg)    Ajout de la fonction RefreshOptionsPage() qui permet de rafraîchir
                        les gadgets de la fenêtre après un changement dynamique des options.
    19-11-2006 (Seg)    L'option NODISPLAY est maintenant gérée dans les paramètres d'affichage.
                        L'option NOREPORTMOUSE est maintenant activée.
    07-03-2006 (Seg)    Gestion des proportions de l'interface en fonction du ratio de l'écran
    06-03-2006 (Seg)    Ajout de "à propos de" et modification des options pour l'overlay
    25-05-2005 (Seg)    Gestion de la boîte d'options
*/


/***** Prototypes */
BOOL Win_OpenWindowOptions(
    struct WindowOptions *, struct Window *,
    struct ProtocolPrefs *, struct GlobalPrefs *,
    struct ScreenModeRequester *, struct FontRequester *,
    struct FontDef *, struct MsgPort *);
void Win_CloseWindowOptions(struct WindowOptions *);
void Win_SetOptionsPage(struct WindowOptions *, LONG);
void Win_RefreshOptionsPage(struct WindowOptions *);
LONG Win_ManageWindowOptionsMessages(struct WindowOptions *, struct IntuiMessage *);

BOOL SelectScreenMode(struct WindowOptions *);
void SetScreenModeZone(struct WindowOptions *, BOOL);
BOOL SelectFont(struct WindowOptions *);
void SetFontZone(struct WindowOptions *, BOOL);
void InitWindowOptionsGadgets(struct FontDef *, ULONG);
void UpdateOptionsGadgetsAttr(struct WindowOptions *);

M_HOOK_PROTO(StringControlKeyHook, struct Hook *, struct SGWork *, ULONG *);


/***** Defines locaux */
#define WIN_WIDTH               530
#define WIN_HEIGHT              325
#define BUTTON_PAGE_LEFT        5
#define BUTTON_PAGE_TOP         5
#define BUTTON_PAGE_WIDTH       80
#define BUTTON_PAGE_HEIGHT      16


/**********************
 ***** Données globales
 **********************/

enum
{
    GID_PAGE_1,
    GID_PAGE_2,
    GID_PAGE_3,
    GID_PAGE_4,
    GID_PAGE_5,
    GID_SAVEAS,
    GID_TEST,
    GID_USE,
    GID_CANCEL,
    GID_NONE,

    GID_P1_DEPTH,
    GID_P1_SCALE,
    GID_P1_SCALE_DOWN,
    GID_P1_SCALE_UP,
    GID_P1_ENCODER,
    GID_P1_ISCOPYRECT,
    GID_P1_ISLOCALCURSOR,
    GID_P1_ISJPEG,
    GID_P1_JPEG,
    GID_P1_ISZLIB,
    GID_P1_ZLIB,

    GID_P2_ISSHARED,
    GID_P2_ISSTARTICONIFY,
    GID_P2_ISVIEWONLY,
    GID_P2_ISNOCLIPBOARD,
    GID_P2_ISKBEMUL,
    GID_P2_ISNOLAMIGAKEY,
    GID_P2_ISREPORTMOUSE,
    GID_P2_ISFLAG,

    GID_P3_DISPLAYTYPE,
    GID_P3_ISTOOLBAR,
    GID_P3_ISCURSOR,
    GID_P3_VIEWTYPE,
    GID_P3_ISOVERLAY,
    GID_P3_OVERLAYTHRESHOLD,
    GID_P3_ISSMOOTH,
    GID_P3_SCREENTYPE,
    GID_P3_SCREENMODE,
    GID_P3_SELECT,
    GID_P3_ISSCREENBAR,

    GID_P4_STYLE,
    GID_P4_FONT,
    GID_P4_SELECT,
    GID_P4_ICON,
    GID_P4_SEARCH,
    GID_P4_CONTROLKEY
};

struct TextAttr OptionsStdTextAttr={NULL,0,0,0};
struct TextAttr OptionsStdTextBoldAttr={NULL,0,0,0};
struct TextAttr OptionsStdTextBoldUnderlinedAttr={NULL,0,0,0};

struct TagItem OptionsNoTag[]=
{
    {TAG_DONE,0UL}
};

struct TagItem OptionsCheckTag[]=
{
    {GTCB_Scaled,TRUE},
    {TAG_DONE,0UL}
};

struct TagItem OptionsTextBorderTag[]=
{
    {GTTX_Border,TRUE},
    {TAG_DONE,0UL}
};

struct TagItem OptionsTextNoBorderTag[]=
{
    {GTTX_Border,FALSE},
    {TAG_DONE,0UL}
};

struct TagItem OptionsNumberTag[]=
{
    {GTNM_Border,TRUE},
    {GTNM_Justification,GTJ_CENTER},
    {TAG_DONE,0UL}
};


/*************
 ***** FENETRE
 *************/

/***** Définitions des gadgets */
struct CustGadgetAttr ButtonPageCodecsTextGadgetAttr    ={LOC_OPTIONS_CODECS,"Codecs",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr ButtonPageMiscTextGadgetAttr      ={LOC_OPTIONS_MISC,"Misc",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr ButtonPageDisplayTextGadgetAttr   ={LOC_OPTIONS_DISPLAY,"Display",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr ButtonPageCustomizeTextGadgetAttr ={LOC_OPTIONS_CUSTOMIZE,"Customize",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr ButtonPageAboutTextGadgetAttr     ={LOC_OPTIONS_ABOUT,"About",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr ButtonPageSaveTextGadgetAttr      ={LOC_OPTIONS_SAVEAS,"Save as",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr ButtonPageTestTextGadgetAttr      ={LOC_OPTIONS_TEST,"Test",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr ButtonPageUseTextGadgetAttr       ={LOC_OPTIONS_USE,"Use",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr ButtonPageCancelTextGadgetAttr    ={LOC_OPTIONS_CANCEL,"Cancel",&OptionsStdTextAttr,PLACETEXT_IN};

struct CustomGadget OptionsGadgetList[]=
{
    {BUTTON_KIND,GID_PAGE_1,OptionsNoTag,BUTTON_PAGE_LEFT,BUTTON_PAGE_TOP,BUTTON_PAGE_WIDTH,BUTTON_PAGE_HEIGHT,0,0,0,0,(APTR)&ButtonPageCodecsTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,GID_PAGE_2,OptionsNoTag,BUTTON_PAGE_LEFT+1*(BUTTON_PAGE_WIDTH+5),BUTTON_PAGE_TOP,BUTTON_PAGE_WIDTH,BUTTON_PAGE_HEIGHT,0,0,0,0,(APTR)&ButtonPageMiscTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,GID_PAGE_3,OptionsNoTag,BUTTON_PAGE_LEFT+2*(BUTTON_PAGE_WIDTH+5),BUTTON_PAGE_TOP,BUTTON_PAGE_WIDTH,BUTTON_PAGE_HEIGHT,0,0,0,0,(APTR)&ButtonPageDisplayTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,GID_PAGE_4,OptionsNoTag,BUTTON_PAGE_LEFT+3*(BUTTON_PAGE_WIDTH+5),BUTTON_PAGE_TOP,BUTTON_PAGE_WIDTH,BUTTON_PAGE_HEIGHT,0,0,0,0,(APTR)&ButtonPageCustomizeTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,GID_PAGE_5,OptionsNoTag,BUTTON_PAGE_LEFT+4*(BUTTON_PAGE_WIDTH+5),BUTTON_PAGE_TOP,BUTTON_PAGE_WIDTH,BUTTON_PAGE_HEIGHT,0,0,0,0,(APTR)&ButtonPageAboutTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,GID_SAVEAS,OptionsNoTag, 5,WIN_HEIGHT-20,100,BUTTON_PAGE_HEIGHT,0,0,0,0,(APTR)&ButtonPageSaveTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,GID_USE,   OptionsNoTag,10,WIN_HEIGHT-20,100,BUTTON_PAGE_HEIGHT,0,0,0,0,(APTR)&ButtonPageUseTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,GID_TEST,  OptionsNoTag,15,WIN_HEIGHT-20,100,BUTTON_PAGE_HEIGHT,0,0,0,0,(APTR)&ButtonPageTestTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,GID_CANCEL,OptionsNoTag,20,WIN_HEIGHT-20,100,BUTTON_PAGE_HEIGHT,0,0,0,0,(APTR)&ButtonPageCancelTextGadgetAttr,-1,NULL,NULL},
    {KIND_DONE}
};

/***** Définitions des motifs */
struct CustomMotif OptionsMotifList[]=
{
    {MOTIF_BEVELBOX_PLAIN,5,25,WIN_WIDTH-10,WIN_HEIGHT-50,0,0,0,0,NULL},
    {MOTIF_DONE}
};


/***********************
 ***** ONGLET 1 - CODECS
 ***********************/

/***** Définitions des gadgets */
struct CustGadgetAttr CycleDepthTextGadgetAttr          ={LOC_OPTIONS_P1_COLORS,"Colors",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr NumberScaleTextGadgetAttr         ={LOC_OPTIONS_P1_SCALE,"Scale",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr ButtonScaleDownTextGadgetAttr     ={-1,"<",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr ButtonScaleUpTextGadgetAttr       ={-1,">",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr CycleEncoderTextGadgetAttr        ={LOC_OPTIONS_P1_ENCODER,"Encoder",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr CheckJpegTextGadgetAttr           ={LOC_OPTIONS_P1_ISJPEG,"Jpeg",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr CheckZLibTextGadgetAttr           ={LOC_OPTIONS_P1_ISZLIB,"ZLib",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr SliderJpegTextGadgetAttr          ={LOC_OPTIONS_P1_QUALITY,"Quality",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr SliderZLibTextGadgetAttr          ={LOC_OPTIONS_P1_COMPRESSION,"Compression",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr CheckCopyRectTextGadgetAttr       ={LOC_OPTIONS_P1_ISCOPYRECT,"Use CopyRect",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CheckLocalMouseCursorTextGadgetAttr={LOC_OPTIONS_P1_ISLOCALCURSOR,"Use local mouse cursor",&OptionsStdTextAttr,PLACETEXT_RIGHT};

STRPTR DepthLabels[7]; /* Initialisé dans le code... */

STRPTR EncodersLabels[]=
{
    "Tight",
    "ZRle",
    "ZLibRaw",
    "ZLibHex",
    "Hextile",
    "CoRRE",
    "RRE",
    "Raw",
    NULL
};

struct TagItem OptionsDepthTag[]=
{
    {GTCY_Labels,(ULONG)DepthLabels},
    {TAG_DONE,0UL}
};

struct TagItem OptionsEncodersTag[]=
{
    {GTCY_Labels,(ULONG)EncodersLabels},
    {TAG_DONE,0UL}
};

struct TagItem OptionsSliderJpegAndZLibTag[]=
{
    {GTSL_Min,0},
    {GTSL_Max,9},
    {GTSL_MaxLevelLen,2},
    {GTSL_LevelFormat,(ULONG)"%ld"},
    {GTSL_LevelPlace,PLACETEXT_RIGHT},
    {TAG_DONE,0UL}
};

struct CustomGadget OptionsPage1GadgetList[]=
{
    {CYCLE_KIND,   GID_P1_DEPTH,        OptionsDepthTag,            100,50,160,16,0,0,0,0,  (APTR)&CycleDepthTextGadgetAttr,-1,NULL,NULL},
    {NUMBER_KIND,  GID_P1_SCALE,        OptionsNumberTag,           100,70,30,16,0,0,0,0,   (APTR)&NumberScaleTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,  GID_P1_SCALE_DOWN,   OptionsNoTag,               131,70,16,16,0,0,0,0,   (APTR)&ButtonScaleDownTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,  GID_P1_SCALE_UP,     OptionsNoTag,               147,70,16,16,0,0,0,0,   (APTR)&ButtonScaleUpTextGadgetAttr,-1,NULL,NULL},

    {CYCLE_KIND,   GID_P1_ENCODER,      OptionsEncodersTag,         100,125,160,16,0,0,0,0, (APTR)&CycleEncoderTextGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND,GID_P1_ISCOPYRECT,   OptionsCheckTag,            100,145,20,16,0,0,0,0,  (APTR)&CheckCopyRectTextGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND,GID_P1_ISLOCALCURSOR,OptionsCheckTag,            100,165,20,16,0,0,0,0,  (APTR)&CheckLocalMouseCursorTextGadgetAttr,-1,NULL,NULL},

    {CHECKBOX_KIND,GID_P1_ISJPEG,       OptionsCheckTag,            100,220,20,16,0,0,0,0,  (APTR)&CheckJpegTextGadgetAttr,-1,NULL,NULL},
    {SLIDER_KIND,  GID_P1_JPEG,         OptionsSliderJpegAndZLibTag,240,221,80,13,0,0,0,0,  (APTR)&SliderJpegTextGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND,GID_P1_ISZLIB,       OptionsCheckTag,            100,240,20,16,0,0,0,0,  (APTR)&CheckZLibTextGadgetAttr,-1,NULL,NULL},
    {SLIDER_KIND,  GID_P1_ZLIB,         OptionsSliderJpegAndZLibTag,240,241,80,13,0,0,0,0,  (APTR)&SliderZLibTextGadgetAttr,-1,NULL,NULL},
    {KIND_DONE}
};


/***** Définitions des motifs */
struct CustMotifAttr GroupRemoteDisplay={LOC_OPTIONS_P1_REMOTEDISPLAY,"Remote display",&OptionsStdTextAttr};
struct CustMotifAttr GroupEncoders={LOC_OPTIONS_P1_ENCODERS,"Encoders",&OptionsStdTextAttr};
struct CustMotifAttr GroupEncodersParameters={LOC_OPTIONS_P1_ENCODERSPARAMETERS,"Encoder parameters",&OptionsStdTextAttr};

struct CustomMotif OptionsPage1MotifList[]=
{
    {MOTIF_BEVELBOX_PLAIN,5,25,WIN_WIDTH-10,WIN_HEIGHT-50,0,0,0,0,NULL},
    {MOTIF_BEVELBOX_DUAL,15,35,WIN_WIDTH-30,65,0,0,0,0,(APTR)&GroupRemoteDisplay},
    {MOTIF_BEVELBOX_DUAL,15,110,WIN_WIDTH-30,85,0,0,0,0,(APTR)&GroupEncoders},
    {MOTIF_BEVELBOX_DUAL,15,205,WIN_WIDTH-30,85,0,0,0,0,(APTR)&GroupEncodersParameters},
    {MOTIF_DONE}
};


/*********************
 ***** ONGLET 2 - MISC
 *********************/

/***** Définitions des gadgets */
struct CustGadgetAttr CheckSharedGadgetAttr             ={LOC_OPTIONS_P2_ISSHARED,"Shared remote control",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CheckStartIconifyGadgetAttr       ={LOC_OPTIONS_P2_ISICONIFY,"Iconify on startup",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CheckViewOnlyGadgetAttr           ={LOC_OPTIONS_P2_ISVIEWONLY,"Inputs locked (View Only)",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CheckNoClipBoardGadgetAttr        ={LOC_OPTIONS_P2_ISNOCLIPBOARD,"No Clipboard",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CheckKBEmulGadgetAttr             ={LOC_OPTIONS_P2_ISKBEMUL,"Emulate some keys (ie: Home/End/Page Up)",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CheckReportMouseGadgetAttr        ={LOC_OPTIONS_P2_ISREPORTMOUSE,"Report mouse move from server",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CheckLAmigaKeyGadgetAttr          ={LOC_OPTIONS_P2_ISNOLAMIGAKEY,"Disable Left Amiga key",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CheckFlagGadgetAttr               ={LOC_OPTIONS_P2_ISFLAG,"No wait Flag (experimental)",&OptionsStdTextAttr,PLACETEXT_RIGHT};

struct CustomGadget OptionsPage2GadgetList[]=
{
    {CHECKBOX_KIND, GID_P2_ISSHARED,    OptionsCheckTag,            40,50,20,16,0,0,0,0,    (APTR)&CheckSharedGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND, GID_P2_ISSTARTICONIFY,OptionsCheckTag,          40,70,20,16,0,0,0,0,    (APTR)&CheckStartIconifyGadgetAttr,-1,NULL,NULL},

    {CHECKBOX_KIND, GID_P2_ISVIEWONLY,  OptionsCheckTag,            40,125,20,16,0,0,0,0,   (APTR)&CheckViewOnlyGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND, GID_P2_ISNOCLIPBOARD,OptionsCheckTag,           40,145,20,16,0,0,0,0,   (APTR)&CheckNoClipBoardGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND, GID_P2_ISKBEMUL,    OptionsCheckTag,            40,165,20,16,0,0,0,0,   (APTR)&CheckKBEmulGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND, GID_P2_ISNOLAMIGAKEY,OptionsCheckTag,           40,185,20,16,0,0,0,0,   (APTR)&CheckLAmigaKeyGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND, GID_P2_ISREPORTMOUSE,OptionsCheckTag,           40,205,20,16,0,0,0,0,   (APTR)&CheckReportMouseGadgetAttr,-1,NULL,NULL},

    {CHECKBOX_KIND, GID_P2_ISFLAG,      OptionsCheckTag,            40,260,20,16,0,0,0,0,   (APTR)&CheckFlagGadgetAttr,-1,NULL,NULL},
    {KIND_DONE}
};

/***** Définitions des motifs */
struct CustMotifAttr GroupStartup={LOC_OPTIONS_P2_STARTUP,"Startup",&OptionsStdTextAttr};
struct CustMotifAttr GroupInputs={LOC_OPTIONS_P2_MOUSEKEYBOARD,"Mouse/Keyboard",&OptionsStdTextAttr};
struct CustMotifAttr GroupSpecial={LOC_OPTIONS_P2_SPECIAL,"Special",&OptionsStdTextAttr};

struct CustomMotif OptionsPage2MotifList[]=
{
    {MOTIF_BEVELBOX_PLAIN,5,25,WIN_WIDTH-10,WIN_HEIGHT-50,0,0,0,0,NULL},
    {MOTIF_BEVELBOX_DUAL,15,35,WIN_WIDTH-30,65,0,0,0,0,(APTR)&GroupStartup},
    {MOTIF_BEVELBOX_DUAL,15,110,WIN_WIDTH-30,125,0,0,0,0,(APTR)&GroupInputs},
    {MOTIF_BEVELBOX_DUAL,15,245,WIN_WIDTH-30,45,0,0,0,0,(APTR)&GroupSpecial},
    {MOTIF_DONE}
};


/************************
 ***** ONGLET 3 - DISPLAY
 ************************/

/***** Définitions des gadgets */
struct CustGadgetAttr CycleDisplayTypeTextGadgetAttr    ={LOC_OPTIONS_P3_DISPLAY,"Display",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr CheckShowToolBarTextGadgetAttr    ={LOC_OPTIONS_P3_ISSHOWTOOLBAR,"Show Toolbar",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CheckShowAmigaCursorTextGadgetAttr={LOC_OPTIONS_P3_ISSHOWCURSOR,"Show Amiga mouse cursor",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CycleViewTypeTextGadgetAttr       ={LOC_OPTIONS_P3_VIEW,"View",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr CheckOverlayTextGadgetAttr        ={LOC_OPTIONS_P3_ISOVERLAY,"Use overlay",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr IntegerOverlayThresholdTextGadgetAttr={LOC_OPTIONS_P3_OVERLAYTHRESHOLD,"% (Threshold)",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CheckSmoothTextGadgetAttr         ={LOC_OPTIONS_P3_ISSMOOTH,"Smooth",&OptionsStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr CycleScreenTypeTextGadgetAttr     ={LOC_OPTIONS_P3_TYPE,"Type",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr TextScreenModeTextGadgetAttr      ={LOC_OPTIONS_P3_SCREEN,"Screen",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr ButtonSelectScreenTextGadgetAttr  ={LOC_OPTIONS_P3_SELECT,"Select",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr CheckScreenTitleTextGadgetAttr    ={LOC_OPTIONS_P3_ISSHOWSCREENBAR,"Show screen title",&OptionsStdTextAttr,PLACETEXT_RIGHT};


STRPTR DisplayTypeLabels[4]; /* Initialisé dans le code... */

struct TagItem OptionsDisplayTypeTag[]=
{
    {GTCY_Labels,(ULONG)DisplayTypeLabels},
    {TAG_DONE,0UL}
};


STRPTR ViewTypeLabels[3]; /* Initialisé dans le code... */

struct TagItem OptionsViewTypeTag[]=
{
    {GTCY_Labels,(ULONG)ViewTypeLabels},
    {TAG_DONE,0UL}
};


STRPTR ScreenTypeLabels[4]; /* Initialisé dans le code... */

struct TagItem OptionsScreenTypeTag[]=
{
    {GTCY_Labels,(ULONG)ScreenTypeLabels},
    {TAG_DONE,0UL}
};


struct CustomGadget OptionsPage3GadgetList[]=
{
    {CYCLE_KIND,    GID_P3_DISPLAYTYPE, OptionsDisplayTypeTag,      100,50,160,16,0,0,0,0,  (APTR)&CycleDisplayTypeTextGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND, GID_P3_ISTOOLBAR,   OptionsCheckTag,            100,70,20,16,0,0,0,0,   (APTR)&CheckShowToolBarTextGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND, GID_P3_ISCURSOR,    OptionsCheckTag,            100,90,20,16,0,0,0,0,   (APTR)&CheckShowAmigaCursorTextGadgetAttr,-1,NULL,NULL},

    {CYCLE_KIND,    GID_P3_VIEWTYPE,    OptionsViewTypeTag,         100,145,160,16,0,0,0,0, (APTR)&CycleViewTypeTextGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND, GID_P3_ISOVERLAY,   OptionsCheckTag,            330,145,20,16,0,0,0,0,  (APTR)&CheckOverlayTextGadgetAttr,-1,NULL,NULL},
    {INTEGER_KIND,  GID_P3_OVERLAYTHRESHOLD,OptionsNoTag,           330,165,50,16,0,0,0,0,  (APTR)&IntegerOverlayThresholdTextGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND, GID_P3_ISSMOOTH,    OptionsCheckTag,            100,165,20,16,0,0,0,0,  (APTR)&CheckSmoothTextGadgetAttr,-1,NULL,NULL},

    {CYCLE_KIND,    GID_P3_SCREENTYPE,  OptionsScreenTypeTag,       100,220,230,16,0,0,0,0, (APTR)&CycleScreenTypeTextGadgetAttr,-1,NULL,NULL},
    {TEXT_KIND,     GID_P3_SCREENMODE,  OptionsTextBorderTag,       100,240,300,16,0,0,0,0, (APTR)&TextScreenModeTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,   GID_P3_SELECT,      OptionsNoTag,               402,240,60,16,0,0,0,0,  (APTR)&ButtonSelectScreenTextGadgetAttr,-1,NULL,NULL},
    {CHECKBOX_KIND, GID_P3_ISSCREENBAR, OptionsCheckTag,            100,260,20,16,0,0,0,0,  (APTR)&CheckScreenTitleTextGadgetAttr,-1,NULL,NULL},

    {KIND_DONE}
};

/***** Définitions des motifs */
struct CustMotifAttr GroupLocalDisplay={LOC_OPTIONS_P3_LOCALDISPLAY,"Local display",&OptionsStdTextAttr};
struct CustMotifAttr GroupViewParameters={LOC_OPTIONS_P3_VIEWPARAMETERS,"View parameters",&OptionsStdTextAttr};
struct CustMotifAttr GroupScreenParameters={LOC_OPTIONS_P3_SCREENPARAMETERS,"Screen parameters",&OptionsStdTextAttr};

struct CustomMotif OptionsPage3MotifList[]=
{
    {MOTIF_BEVELBOX_PLAIN,5,25,WIN_WIDTH-10,WIN_HEIGHT-50,0,0,0,0,NULL},
    {MOTIF_BEVELBOX_DUAL,15,35,WIN_WIDTH-30,85,0,0,0,0,(APTR)&GroupLocalDisplay},
    {MOTIF_BEVELBOX_DUAL,15,130,WIN_WIDTH-30,65,0,0,0,0,(APTR)&GroupViewParameters},
    {MOTIF_BEVELBOX_DUAL,15,205,WIN_WIDTH-30,85,0,0,0,0,(APTR)&GroupScreenParameters},
    {MOTIF_DONE}
};


/**************************
 ***** ONGLET 4 - CUSTOMIZE
 **************************/

/***** Définitions des gadgets */
struct CustGadgetAttr CycleStyleTextGadgetAttr          ={LOC_OPTIONS_P4_STYLE,"Style",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr TextFontTextGadgetAttr            ={LOC_OPTIONS_P4_FONT,"Font",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr ButtonSelectFontTextGadgetAttr    ={LOC_OPTIONS_P4_SELECT,"Select",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr StringIconPathTextGadgetAttr      ={LOC_OPTIONS_P4_ICON,"Icon",&OptionsStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr ButtonSearchTextGadgetAttr        ={LOC_OPTIONS_P4_SEARCH,"Search",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr StringControlKeyGadgetAttr        ={LOC_OPTIONS_P4_CONTROLKEY,"Control Key",&OptionsStdTextAttr,PLACETEXT_LEFT};

STRPTR StyleLabels[4]; /* Initialisé dans le code... */

struct TagItem OptionsStyleTag[]=
{
    {GTCY_Labels,(ULONG)StyleLabels},
    {TAG_DONE,0UL}
};

struct TagItem OptionControlKeyTags[]=
{
    {GTST_EditHook,(ULONG)NULL},
    {GTST_MaxChars,256},
    {TAG_DONE,0UL}
};


struct CustomGadget OptionsPage4GadgetList[]=
{
    {CYCLE_KIND,    GID_P4_STYLE,       OptionsStyleTag,            100,50,250,16,0,0,0,0,  (APTR)&CycleStyleTextGadgetAttr,-1,NULL,NULL},
    {TEXT_KIND,     GID_P4_FONT,        OptionsTextBorderTag,       100,80,300,16,0,0,0,0,  (APTR)&TextFontTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,   GID_P4_SELECT,      OptionsNoTag,               402,80,60,16,0,0,0,0,   (APTR)&ButtonSelectFontTextGadgetAttr,-1,NULL,NULL},

    {STRING_KIND,   GID_P4_ICON,        OptionsNoTag,               100,145,300,16,0,0,0,0, (APTR)&StringIconPathTextGadgetAttr,-1,NULL,NULL},
    {BUTTON_KIND,   GID_P4_SEARCH,      OptionsNoTag,               402,145,60,16,0,0,0,0,  (APTR)&ButtonSearchTextGadgetAttr,-1,NULL,NULL},

    {STRING_KIND,   GID_P4_CONTROLKEY,  OptionControlKeyTags,       100,175,250,16,0,0,0,0, (APTR)&StringControlKeyGadgetAttr,-1,NULL,NULL},
    {KIND_DONE}
};

/***** Définitions des motifs */
struct CustMotifAttr GroupGUI={LOC_OPTIONS_P4_GUI,"Graphic user interface",&OptionsStdTextAttr};
struct CustMotifAttr GroupMisc={LOC_OPTIONS_P4_MISC,"Miscellaneous",&OptionsStdTextAttr};

struct CustomMotif OptionsPage4MotifList[]=
{
    {MOTIF_BEVELBOX_PLAIN,5,25,WIN_WIDTH-10,WIN_HEIGHT-50,0,0,0,0,NULL},
    {MOTIF_BEVELBOX_DUAL,15,35,WIN_WIDTH-30,75,0,0,0,0,(APTR)&GroupGUI},
    {MOTIF_BEVELBOX_DUAL,15,120,WIN_WIDTH-30,170,0,0,0,0,(APTR)&GroupMisc},
    {MOTIF_DONE}
};


/**********************
 ***** ONGLET 5 - ABOUT
 **********************/

/***** Définitions des gadgets */
struct CustGadgetAttr TextFontAboutLine1GadgetAttr      ={-1,__APPNAME__" "__CURRENTVERSION__" "__COPYRIGHT__,&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr TextFontAboutLine2GadgetAttr      ={-1,"Compiled for "__OSNAME__" ("__CURRENTDATE__")",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr TextFontAboutLine3GadgetAttr      ={-1,"This program is Giftware",&OptionsStdTextBoldAttr,PLACETEXT_IN};
struct CustGadgetAttr TextFontAboutLine4GadgetAttr      ={-1,"Web: http://twinvnc.free.fr",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr TextFontAboutLine5GadgetAttr      ={-1,"Mail: twinvnc@free.fr",&OptionsStdTextAttr,PLACETEXT_IN};

struct CustGadgetAttr TextFontAboutLine6GadgetAttr      ={LOC_OPTIONS_P5_AUTHORS,"Authors",&OptionsStdTextBoldUnderlinedAttr,PLACETEXT_IN};
struct CustGadgetAttr TextFontAboutLine7GadgetAttr      ={-1,"Code by Sébastien Gréau (Seg)",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr TextFontAboutLine8GadgetAttr      ={-1,"Design by Christophe Delorme (Highlander)",&OptionsStdTextAttr,PLACETEXT_IN};

struct CustGadgetAttr TextFontAboutLine9GadgetAttr      ={LOC_OPTIONS_P5_COMPILATION,"Compilation",&OptionsStdTextBoldUnderlinedAttr,PLACETEXT_IN};
struct CustGadgetAttr TextFontAboutLine10GadgetAttr     ={-1,"AmigaOS3.x: Sébastien Gréau (Seg)",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr TextFontAboutLine11GadgetAttr     ={-1,"AmigaOS4: Sébastien Gréau (Seg)",&OptionsStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr TextFontAboutLine12GadgetAttr     ={-1,"MorphOS: Nicolas Gressard (Niffo)",&OptionsStdTextAttr,PLACETEXT_IN};

struct CustGadgetAttr TextFontAboutLine13GadgetAttr     ={LOC_OPTIONS_P5_THANKS,"And thanks to...",&OptionsStdTextBoldUnderlinedAttr,PLACETEXT_IN};
struct CustGadgetAttr TextFontAboutLine14GadgetAttr     ={-1,"Corto, Crisot, Fab, FaraPeg, Leo, Stan and sreg",&OptionsStdTextAttr,PLACETEXT_IN};

struct CustomGadget OptionsPage5GadgetList[]=
{
    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,35,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine1GadgetAttr,-1,NULL,NULL},
    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,51,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine2GadgetAttr,-1,NULL,NULL},
    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,67,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine3GadgetAttr,-1,NULL,NULL},

    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,90,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine4GadgetAttr,-1,NULL,NULL},
    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,106,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine5GadgetAttr,-1,NULL,NULL},

    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,130,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine6GadgetAttr,-1,NULL,NULL},
    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,146,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine7GadgetAttr,-1,NULL,NULL},
    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,162,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine8GadgetAttr,-1,NULL,NULL},

    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,185,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine9GadgetAttr,-1,NULL,NULL},
    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,201,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine10GadgetAttr,-1,NULL,NULL},
    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,217,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine11GadgetAttr,-1,NULL,NULL},
    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,233,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine12GadgetAttr,-1,NULL,NULL},

    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,255,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine13GadgetAttr,-1,NULL,NULL},
    {TEXT_KIND,     0,                  OptionsTextNoBorderTag,     15,271,WIN_WIDTH-30,16,0,0,0,0,(APTR)&TextFontAboutLine14GadgetAttr,-1,NULL,NULL},

    {KIND_DONE}
};

/***** Définitions des motifs */
struct CustomMotif OptionsPage5MotifList[]=
{
    {MOTIF_BEVELBOX_PLAIN,5,25,WIN_WIDTH-10,WIN_HEIGHT-50,0,0,0,0,NULL},
    {MOTIF_DONE}
};


/*****
    Ouverture d'une fenêtre de statut
*****/

BOOL Win_OpenWindowOptions(
    struct WindowOptions *wo, struct Window *ParentWindow,
    struct ProtocolPrefs *PPrefs, struct GlobalPrefs *GPrefs,
    struct ScreenModeRequester *SMReq, struct FontRequester *FontReq,
    struct FontDef *fd, struct MsgPort *MPort)
{
    BOOL IsSuccess=FALSE,IsLocked;
    ULONG VerticalRatio=0;
    struct Screen *Scr=Gal_GetBestPublicScreen(ParentWindow,&IsLocked,&VerticalRatio);

    if(Scr!=NULL)
    {
        LONG WinWidth=WIN_WIDTH,WinHeight=WIN_HEIGHT;

        wo->ParentWindow=ParentWindow;
        wo->PPrefs=PPrefs;
        wo->GPrefs=GPrefs;
        wo->SMReq=SMReq;
        wo->FontReq=FontReq;

        /* On initialise le hook du Control Key */
        wo->ControlKeyHook.h_Entry=(HOOKFUNC)&StringControlKeyHook;
        wo->ControlKeyHook.h_SubEntry=NULL;
        wo->ControlKeyHook.h_Data=(APTR)&GPrefs->ControlKey;
        OptionControlKeyTags[0].ti_Data=(ULONG)&wo->ControlKeyHook;

        Gui_GetBestWindowSize(&WinWidth,&WinHeight,VerticalRatio);
        InitWindowOptionsGadgets(fd,VerticalRatio);

        IsSuccess=Gui_OpenCompleteWindow(
                &wo->Window,Scr,MPort,
                __APPNAME__,Sys_GetString(LOC_OPTIONS_TITLE,"Options"),
                -1,-1,WinWidth,WinHeight,
                OptionsGadgetList,OptionsMotifList,NULL);

        /* Déverrouillage de l'écran si celui-ci est un écran public */
        if(IsLocked) UnlockPubScreen(NULL,Scr);

        /* Initialisation des gadgets de la fenêtre */
        if(IsSuccess)
        {
            struct VisualInfo *vi=wo->Window.VisualInfo;
            struct Window *win=wo->Window.WindowBase;

            wo->GadgetsPage[0]=Gui_CreateWindowGadgets(win,vi,NULL,OptionsPage1GadgetList);
            wo->GadgetsPage[1]=Gui_CreateWindowGadgets(win,vi,NULL,OptionsPage2GadgetList);
            wo->GadgetsPage[2]=Gui_CreateWindowGadgets(win,vi,NULL,OptionsPage3GadgetList);
            wo->GadgetsPage[3]=Gui_CreateWindowGadgets(win,vi,NULL,OptionsPage4GadgetList);
            wo->GadgetsPage[4]=Gui_CreateWindowGadgets(win,vi,NULL,OptionsPage5GadgetList);
            wo->MotifsPage[0]=OptionsPage1MotifList;
            wo->MotifsPage[1]=OptionsPage2MotifList;
            wo->MotifsPage[2]=OptionsPage3MotifList;
            wo->MotifsPage[3]=OptionsPage4MotifList;
            wo->MotifsPage[4]=OptionsPage5MotifList;

            if(!wo->GadgetsPage[0] || !wo->GadgetsPage[1] || !wo->GadgetsPage[2] || !wo->GadgetsPage[3] || !wo->GadgetsPage[4])
            {
                Win_CloseWindowOptions(wo);
                IsSuccess=FALSE;
            }
            else
            {
                UpdateOptionsGadgetsAttr(wo);
                SetScreenModeZone(wo,FALSE);
                SetFontZone(wo,FALSE);
                Win_SetOptionsPage(wo,wo->Page);
            }
        }
    }

    return IsSuccess;
}


/*****
    Fermeture de la fenêtre de statut
*****/

void Win_CloseWindowOptions(struct WindowOptions *wo)
{
    struct Window *win=wo->Window.WindowBase;

    if(win!=NULL)
    {
        /* On supprime les pages de gadgets de la fenêtre */
        Gui_RemoveWindowGadgetsFromWindow(win,wo->GadgetsPage[0]);
        Gui_RemoveWindowGadgetsFromWindow(win,wo->GadgetsPage[1]);
        Gui_RemoveWindowGadgetsFromWindow(win,wo->GadgetsPage[2]);
        Gui_RemoveWindowGadgetsFromWindow(win,wo->GadgetsPage[3]);
        Gui_RemoveWindowGadgetsFromWindow(win,wo->GadgetsPage[4]);
    }

    /* On libère les ressources allouées par les pages de gadgets */
    Gui_FreeWindowGadgets(wo->GadgetsPage[0]);
    Gui_FreeWindowGadgets(wo->GadgetsPage[1]);
    Gui_FreeWindowGadgets(wo->GadgetsPage[2]);
    Gui_FreeWindowGadgets(wo->GadgetsPage[3]);
    Gui_FreeWindowGadgets(wo->GadgetsPage[4]);
    wo->GadgetsPage[0]=NULL;
    wo->GadgetsPage[1]=NULL;
    wo->GadgetsPage[2]=NULL;
    wo->GadgetsPage[3]=NULL;
    wo->GadgetsPage[4]=NULL;

    /* On ferme la fenêtre et ses ressources */
    Gui_CloseCompleteWindow(&wo->Window);
}


/*****
    Cette routine se charge de sélectionner les gadgets à afficher sur la page courante
*****/

void Win_SetOptionsPage(struct WindowOptions *wo, LONG Page)
{
    struct Window *win=wo->Window.WindowBase;

    if(win!=NULL)
    {
        /* On supprime l'ancienne page de gadgets */
        if(wo->Page>=0)
        {
            struct Gadget *GadgetPtr=Gui_GetGadgetPtr(OptionsGadgetList,(LONG)GID_PAGE_1+wo->Page);

            Gui_RemoveWindowGadgetsFromWindow(win,wo->GadgetsPage[wo->Page]);
            GT_SetGadgetAttrs(GadgetPtr,NULL,NULL,GA_Disabled,FALSE,TAG_DONE);
        }

        /* On colle les gadgets de la nouvelle page */
        if(Page>=0)
        {
            struct WindowGadgets *Ptr=wo->GadgetsPage[Page];
            struct Gadget *GadgetPtr=Gui_GetGadgetPtr(OptionsGadgetList,(LONG)GID_PAGE_1+Page);

            GT_SetGadgetAttrs(GadgetPtr,NULL,NULL,GA_Disabled,TRUE,TAG_DONE);
            Gui_AttachWindowGadgetsToWindow(win,Ptr);

            if(wo->MotifsPage[Page]!=NULL) Gui_DrawMotifs(win,wo->Window.VisualInfo,wo->MotifsPage[Page]);
            if(Ptr!=NULL) RefreshGadgets(Ptr->FirstGadget,win,NULL);
            else RefreshGadgets(wo->Window.GadgetsBase->FirstGadget,win,NULL);

            /* Rafraîchissement du contenu de certains gadgets gadtools */
            GT_RefreshWindow(win,NULL);
        }
    }

    wo->Page=Page;
}


/*****
    Rafraîchissement des options de la page courante
*****/

void Win_RefreshOptionsPage(struct WindowOptions *wo)
{
    struct Window *win=wo->Window.WindowBase;

    if(win!=NULL)
    {
        struct WindowGadgets *Ptr=wo->GadgetsPage[wo->Page];

        /* Mise à jour des états des gadgets */
        UpdateOptionsGadgetsAttr(wo);

        if(Ptr!=NULL)
        {

            /* Rafraîchissement des gadgets de la page courante */
            RefreshGadgets(Ptr->FirstGadget,win,NULL);
            GT_RefreshWindow(win,NULL);
        }
    }
}


/*****
    Traitement des événements de la fenêtre d'option
*****/

LONG Win_ManageWindowOptionsMessages(struct WindowOptions *wo, struct IntuiMessage *msg)
{
    LONG Result=WIN_OPTIONS_RESULT_NONE;
    struct ProtocolPrefs *pp=wo->PPrefs;
    struct GlobalPrefs *gp=wo->GPrefs;
    struct Window *win=wo->Window.WindowBase;

    GT_RefreshWindow(win,NULL);
    switch(msg->Class)
    {
        case IDCMP_GADGETUP:
            {
                struct Gadget *GadgetPtr=(struct Gadget *)msg->IAddress;
                ULONG GadID=GadgetPtr->GadgetID,Value=0;

                switch(GadID)
                {
                    /**********************************************/
                    /* Gestion des gadgets généraux de la fenêtre */
                    /**********************************************/
                    case GID_PAGE_1:
                    case GID_PAGE_2:
                    case GID_PAGE_3:
                    case GID_PAGE_4:
                    case GID_PAGE_5:
                        Win_SetOptionsPage(wo,GadID-(LONG)GID_PAGE_1);
                        break;

                    case GID_SAVEAS:
                        Result=WIN_OPTIONS_RESULT_SAVEAS;
                        break;

                    case GID_TEST:
                        Result=WIN_OPTIONS_RESULT_TEST;
                        break;

                    case GID_USE:
                        Result=WIN_OPTIONS_RESULT_USE;
                        break;

                    case GID_CANCEL:
                        Result=WIN_OPTIONS_RESULT_CANCEL;
                        break;

                    /************************************/
                    /* Gestion des gadgets de la page 1 */
                    /************************************/
                    case GID_P1_DEPTH:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCY_Active,&Value,TAG_DONE);
                        if(Value==0) pp->Depth=0;
                        else if(Value==1) pp->Depth=3;
                        else if(Value==2) pp->Depth=6;
                        else if(Value==3) pp->Depth=8;
                        else if(Value==4) pp->Depth=16;
                        else if(Value==5) pp->Depth=24;
                        break;

                    case GID_P1_SCALE_UP:
                        pp->ServerScale++;
                        if(pp->ServerScale>99) pp->ServerScale=99;
                        GadgetPtr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_SCALE);
                        GT_SetGadgetAttrs(GadgetPtr,win,NULL,GTNM_Number,(ULONG)pp->ServerScale,TAG_DONE);
                        break;

                    case GID_P1_SCALE_DOWN:
                        pp->ServerScale--;
                        if(pp->ServerScale<=1) pp->ServerScale=1;
                        GadgetPtr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_SCALE);
                        GT_SetGadgetAttrs(GadgetPtr,win,NULL,GTNM_Number,(ULONG)pp->ServerScale,TAG_DONE);
                        break;

                    case GID_P1_ENCODER:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCY_Active,&Value,TAG_DONE);
                        pp->EncoderId=Value;
                        break;

                    case GID_P1_ISCOPYRECT:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        pp->IsNoCopyRect=Value?FALSE:TRUE;
                        break;

                    case GID_P1_ISLOCALCURSOR:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        pp->IsNoLocalCursor=Value?FALSE:TRUE;
                        break;

                    case GID_P1_ISJPEG:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        GadgetPtr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_JPEG);
                        GT_SetGadgetAttrs(GadgetPtr,win,NULL,GA_Disabled,!Value,TAG_DONE);
                        pp->IsUseJpeg=Value?TRUE:FALSE;
                        break;

                    case GID_P1_ISZLIB:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        GadgetPtr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_ZLIB);
                        GT_SetGadgetAttrs(GadgetPtr,win,NULL,GA_Disabled,!Value,TAG_DONE);
                        pp->IsChangeZLib=Value?TRUE:FALSE;
                        break;

                    /************************************/
                    /* Gestion des gadgets de la page 2 */
                    /************************************/
                    case GID_P2_ISSHARED:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        pp->IsShared=Value?TRUE:FALSE;
                        break;

                    case GID_P2_ISSTARTICONIFY:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        gp->IsStartIconify=Value?TRUE:FALSE;
                        break;

                    case GID_P2_ISVIEWONLY:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        gp->IsViewOnly=Value?TRUE:FALSE;
                        break;

                    case GID_P2_ISNOCLIPBOARD:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        gp->IsNoClipboard=Value?TRUE:FALSE;
                        break;

                    case GID_P2_ISKBEMUL:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        gp->IsKeyboardEmul=Value?TRUE:FALSE;
                        break;

                    case GID_P2_ISNOLAMIGAKEY:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        gp->IsNoLAmigaKey=Value?TRUE:FALSE;
                        break;

                    case GID_P2_ISREPORTMOUSE:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        pp->IsReportMouse=Value?TRUE:FALSE;
                        break;

                    case GID_P2_ISFLAG:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        gp->Flag=Value?TRUE:FALSE;
                        break;

                    /************************************/
                    /* Gestion des gadgets de la page 3 */
                    /************************************/
                    case GID_P3_DISPLAYTYPE:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCY_Active,&Value,TAG_DONE);
                        gp->DisplayOption=Value;
                        break;

                    case GID_P3_ISTOOLBAR:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        gp->IsShowToolBar=Value?TRUE:FALSE;
                        break;

                    case GID_P3_ISCURSOR:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        gp->IsCursor=Value?TRUE:FALSE;
                        break;

                    case GID_P3_VIEWTYPE:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCY_Active,&Value,TAG_DONE);
                        gp->ViewOption=Value;
                        break;

                    case GID_P3_ISOVERLAY:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        gp->IsOverlay=Value?TRUE:FALSE;
                        GadgetPtr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_OVERLAYTHRESHOLD);
                        GT_SetGadgetAttrs(GadgetPtr,win,NULL,
                                GA_Disabled,!Value,
                                TAG_DONE);
                        break;

                    case GID_P3_ISSMOOTH:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        gp->IsSmoothed=Value?TRUE:FALSE;
                        break;

                    case GID_P3_OVERLAYTHRESHOLD:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTIN_Number,&Value,TAG_DONE);
                        gp->OverlayThreshold=Value;
                        break;

                    case GID_P3_SCREENTYPE:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCY_Active,&Value,TAG_DONE);
                        if(Value==0) gp->ScrModeSelectType=SCRMODE_SELECT_AUTO;
                        else if(Value==1) gp->ScrModeSelectType=SCRMODE_SELECT_ASK;
                        else gp->ScrModeSelectType=SCRMODE_SELECT_CURRENT;

                        GadgetPtr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_SCREENMODE);
                        GT_SetGadgetAttrs(GadgetPtr,win,NULL,GA_Disabled,(Value==0?TRUE:FALSE),TAG_DONE);
                        GadgetPtr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_SELECT);
                        GT_SetGadgetAttrs(GadgetPtr,win,NULL,GA_Disabled,(Value==0?TRUE:FALSE),TAG_DONE);
                        break;

                    case GID_P3_SELECT:
                        SelectScreenMode(wo);
                        break;

                    case GID_P3_ISSCREENBAR:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                        gp->IsScreenBar=Value?TRUE:FALSE;
                        break;

                    /************************************/
                    /* Gestion des gadgets de la page 4 */
                    /************************************/
                    case GID_P4_SELECT:
                        SelectFont(wo);
                        break;

                    case GID_P4_CONTROLKEY:
                        break;
                }
            }
            break;

        case IDCMP_MOUSEMOVE:
            {
                struct Gadget *GadgetPtr=(struct Gadget *)msg->IAddress;
                ULONG GadID=GadgetPtr->GadgetID,Value=0;

                switch(GadID)
                {
                    case GID_P1_JPEG:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTSL_Level,&Value,TAG_DONE);
                        pp->JpegQuality=Value;
                        break;

                    case GID_P1_ZLIB:
                        GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTSL_Level,&Value,TAG_DONE);
                        pp->ZLibLevel=Value;
                        break;
                }
            }
            break;

        case IDCMP_GADGETHELP:
            /*
            {
                struct Gadget *GadgetPtr=(struct Gadget *)msg->IAddress;
                char *Title=wo->Window.WindowTitle;

                if(GadgetPtr!=NULL && GadgetPtr->UserData!=NULL) Title=(char *)GadgetPtr->UserData;
                TVNC_SetWindowTitles(win,Title,NULL);
            }
            */
            break;

        case IDCMP_CLOSEWINDOW:
            Result=WIN_OPTIONS_RESULT_CANCEL;
            break;
    }

    return Result;
}


/*****
    Sélection du mode d'écran via l'ASL.library
*****/

BOOL SelectScreenMode(struct WindowOptions *wo)
{
    BOOL IsSuccess;
    struct GlobalPrefs *gp=wo->GPrefs;
    struct Window *pwin=wo->ParentWindow;
    WORD MinW=0,MinH=0;
    UWORD MaxW=0,MaxH=0;

    if(pwin!=NULL)
    {
        Forbid();
        MinW=pwin->MinWidth;
        MinH=pwin->MinHeight;
        MaxW=pwin->MaxWidth;
        MaxH=pwin->MaxHeight;
        WindowLimits(pwin,pwin->Width,pwin->Height,(UWORD)pwin->Width,(UWORD)pwin->Height);
        Permit();
    }

    IsSuccess=AslRequestTags((APTR)wo->SMReq,
        ASLSM_Window,(ULONG)wo->Window.WindowBase,
        ASLSM_SleepWindow,TRUE,
        ASLSM_InitialDisplayID,gp->ScrModeID,
        ASLSM_InitialDisplayDepth,gp->ScrDepth,
        ASLSM_DoDepth,TRUE,
        TAG_DONE);

    if(IsSuccess)
    {
        gp->ScrModeID=(wo->SMReq)->sm_DisplayID;
        gp->ScrDepth=(wo->SMReq)->sm_DisplayDepth;
        SetScreenModeZone(wo,TRUE);
    }
    else
    {
        if(!IoErr()) IsSuccess=TRUE;
    }

    if(pwin!=NULL) WindowLimits(pwin,MinW,MinH,MaxW,MaxH);

    return IsSuccess;
}


/*****
    Rafraîchissement du gadget qui indique le mode d'écran à utiliser
*****/

void SetScreenModeZone(struct WindowOptions *wo, BOOL IsRefreshGadget)
{
    struct Gadget *Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_SCREENMODE);
    struct Window *win=NULL;
    struct NameInfo ni;

    wo->DisplayedScreenMode[0]=0;
    if(GetDisplayInfoData(NULL,(UBYTE *)&ni,sizeof(ni),DTAG_NAME,wo->GPrefs->ScrModeID)>0)
    {
        Sys_StrCopy(wo->DisplayedScreenMode,ni.Name,sizeof(wo->DisplayedScreenMode));
    }

    if(IsRefreshGadget) win=wo->Window.WindowBase;
    GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,(ULONG)wo->DisplayedScreenMode,TAG_DONE);
}


/*****
    Sélection de la font via l'ASL.library
*****/

BOOL SelectFont(struct WindowOptions *wo)
{
    BOOL IsSuccess;
    struct Window *pwin=wo->ParentWindow;
    WORD MinW=0,MinH=0;
    UWORD MaxW=0,MaxH=0;

    if(pwin!=NULL)
    {
        Forbid();
        MinW=pwin->MinWidth;
        MinH=pwin->MinHeight;
        MaxW=pwin->MaxWidth;
        MaxH=pwin->MaxHeight;
        WindowLimits(pwin,pwin->Width,pwin->Height,(UWORD)pwin->Width,(UWORD)pwin->Height);
        Permit();
    }

    IsSuccess=AslRequestTags((APTR)wo->FontReq,
        ASLFO_Window,(ULONG)wo->Window.WindowBase,
        ASLFO_SleepWindow,TRUE,
        ASLFO_MaxHeight,15,
        TAG_DONE);

    if(IsSuccess)
    {
        struct GlobalPrefs *gp=wo->GPrefs;
        struct TextAttr *Ptr=&(wo->FontReq)->fo_Attr;

        Sys_StrCopy(gp->FontName,Ptr->ta_Name,sizeof(gp->FontName));
        gp->FontSize=(LONG)Ptr->ta_YSize;

        SetFontZone(wo,TRUE);
    } else if(!IoErr()) IsSuccess=TRUE;

    if(pwin!=NULL) WindowLimits(pwin,MinW,MinH,MaxW,MaxH);

    return IsSuccess;
}


/*****
    Rafraîchissement du gadget qui indique la font à utiliser
*****/

void SetFontZone(struct WindowOptions *wo, BOOL IsRefreshGadget)
{
    struct Gadget *Ptr=Gui_GetGadgetPtr(OptionsPage4GadgetList,GID_P4_FONT);
    struct Window *win=NULL;

    Sys_SPrintf(wo->DisplayedFont,"%s/%ld",wo->GPrefs->FontName,wo->GPrefs->FontSize);
    if(IsRefreshGadget) win=wo->Window.WindowBase;
    GT_SetGadgetAttrs(Ptr,win,NULL,GTTX_Text,(ULONG)wo->DisplayedFont,TAG_DONE);
}


/*****
    Initialisation des proportions des gadgets en fonctons des ressources
*****/

void InitWindowOptionsGadgets(struct FontDef *fd, ULONG VerticalRatio)
{
    Gui_CopyObjectPosition(OptionsGadgetList,OptionsMotifList,VerticalRatio);
    Gui_CopyObjectPosition(OptionsPage1GadgetList,OptionsPage1MotifList,VerticalRatio);
    Gui_CopyObjectPosition(OptionsPage2GadgetList,OptionsPage2MotifList,VerticalRatio);
    Gui_CopyObjectPosition(OptionsPage3GadgetList,OptionsPage3MotifList,VerticalRatio);
    Gui_CopyObjectPosition(OptionsPage4GadgetList,OptionsPage4MotifList,VerticalRatio);
    Gui_CopyObjectPosition(OptionsPage5GadgetList,OptionsPage5MotifList,VerticalRatio);

    Gui_InitTextAttr(fd,&OptionsStdTextAttr,FS_NORMAL);
    Gui_InitTextAttr(fd,&OptionsStdTextBoldAttr,FSF_BOLD);
    Gui_InitTextAttr(fd,&OptionsStdTextBoldUnderlinedAttr,FSF_BOLD|FSF_UNDERLINED);

    /* Disposition des gadgets principaux de la fenêtre */
    {
        static const ULONG GadgetIDHead[]={GID_PAGE_1,GID_PAGE_2,GID_PAGE_3,GID_PAGE_4,GID_PAGE_5,(ULONG)-1};
        static const ULONG GadgetIDFoot[]={GID_SAVEAS,GID_TEST,GID_USE,GID_CANCEL,(ULONG)-1};

        Gui_DisposeGadgetsHorizontaly(5,WIN_WIDTH-10,OptionsGadgetList,GadgetIDHead,FALSE);
        Gui_SpreadGadgetsHorizontaly(5,WIN_WIDTH-10,OptionsGadgetList,GadgetIDFoot);
    }

    /* Disposition des gadgets de la page 1 */
    {
        static const ULONG GadgetIDSlider[]={GID_P1_JPEG,GID_P1_ZLIB,(ULONG)-1};
        struct CustomGadget *GadgetPtr;
        LONG LeftBase;

        /* Localisation du gadget cycle Colors */
        DepthLabels[0]=(STRPTR)Sys_GetString(LOC_OPTIONS_COLORS_SERVER,"Like server");
        DepthLabels[1]=(STRPTR)Sys_GetString(LOC_OPTIONS_COLORS_8,"8 colors");
        DepthLabels[2]=(STRPTR)Sys_GetString(LOC_OPTIONS_COLORS_64,"64 colors");
        DepthLabels[3]=(STRPTR)Sys_GetString(LOC_OPTIONS_COLORS_256,"256 colors");
        DepthLabels[4]=(STRPTR)Sys_GetString(LOC_OPTIONS_COLORS_65K,"65536 colors");
        DepthLabels[5]=(STRPTR)Sys_GetString(LOC_OPTIONS_COLORS_16M,"16M colors");
        DepthLabels[6]=NULL;

        /* Décalage des gadgets en fonction de la longueur des textes à gauche
           des gadgets, par rapport à la bordure gauche
        */
        Gui_ShiftGadgetsHorizontaly(20,WIN_WIDTH-40,OptionsPage1GadgetList,NULL);

        /* Décalage des sliders par rapport aux boutons check situes à
           leur gauche.
        */
        GadgetPtr=Gui_GetCustomGadgetPtr(OptionsPage1GadgetList,GID_P1_ISJPEG);
        LeftBase=GadgetPtr->Left+GadgetPtr->Width+10;
        Gui_ShiftGadgetsHorizontaly(LeftBase,WIN_WIDTH-LeftBase-40,OptionsPage1GadgetList,GadgetIDSlider);

        /* Agrandissement des cycles gadgets */
        GadgetPtr=Gui_GetCustomGadgetPtr(OptionsPage1GadgetList,GID_P1_DEPTH);
        GadgetPtr->Width=Gui_GetFinalCycleGadgetWidth(GadgetPtr,DepthLabels);

        GadgetPtr=Gui_GetCustomGadgetPtr(OptionsPage1GadgetList,GID_P1_ENCODER);
        GadgetPtr->Width=Gui_GetFinalCycleGadgetWidth(GadgetPtr,EncodersLabels);
    }

    /* Disposition des gadgets de la page 3 */
    {
        struct CustomGadget *GadgetPtr,*GadViewTypePtr;
        static const ULONG GadgetIDButton[]={GID_P3_SELECT,(ULONG)-1};

        /* Localisation du gadget cycle Display */
        DisplayTypeLabels[0]=(STRPTR)Sys_GetString(LOC_OPTIONS_DISPLAY_WINDOW,"Window");
        DisplayTypeLabels[1]=(STRPTR)Sys_GetString(LOC_OPTIONS_DISPLAY_FULLSCREEN,"Full screen");
        DisplayTypeLabels[2]=(STRPTR)Sys_GetString(LOC_OPTIONS_DISPLAY_NODISPLAY,"No display");
        DisplayTypeLabels[3]=NULL;

        /* Localisation du gadget cycle View */
        ViewTypeLabels[0]=(STRPTR)Sys_GetString(LOC_OPTIONS_VIEW_NORMAL,"Normal");
        ViewTypeLabels[1]=(STRPTR)Sys_GetString(LOC_OPTIONS_VIEW_SCALED,"Scaled");
        ViewTypeLabels[2]=NULL;

        /* Localisation du gadget cycle Type */
        ScreenTypeLabels[0]=(STRPTR)Sys_GetString(LOC_OPTIONS_TYPE_AUTODETECT,"Auto-detect screen mode");
        ScreenTypeLabels[1]=(STRPTR)Sys_GetString(LOC_OPTIONS_TYPE_ASK,"Ask screen mode");
        ScreenTypeLabels[2]=(STRPTR)Sys_GetString(LOC_OPTIONS_TYPE_USEMODEID,"Use specified screen mode");
        ScreenTypeLabels[3]=NULL;

        /* Agrandissement des boutons */
        Gui_ResizeButtonGadgetHorizontaly(OptionsPage3GadgetList,GadgetIDButton);

        /* Agrandissement des cycles gadgets */
        GadgetPtr=Gui_GetCustomGadgetPtr(OptionsPage3GadgetList,GID_P3_DISPLAYTYPE);
        GadgetPtr->Width=Gui_GetFinalCycleGadgetWidth(GadgetPtr,DisplayTypeLabels);

        GadViewTypePtr=Gui_GetCustomGadgetPtr(OptionsPage3GadgetList,GID_P3_VIEWTYPE);
        GadViewTypePtr->Width=Gui_GetFinalCycleGadgetWidth(GadViewTypePtr,ViewTypeLabels);

        GadgetPtr=Gui_GetCustomGadgetPtr(OptionsPage3GadgetList,GID_P3_SCREENTYPE);
        GadgetPtr->Width=Gui_GetFinalCycleGadgetWidth(GadgetPtr,ScreenTypeLabels);

        /* Décalage des gadgets en fonction de la longueur des textes à gauche
           des gadgets, par rapport à la bordure gauche
        */
        Gui_ShiftGadgetsHorizontaly(20,WIN_WIDTH-40,OptionsPage3GadgetList,NULL);

        /* Positionnement du gadget overlay par rapport au cycle gadget à côté */
        GadgetPtr=Gui_GetCustomGadgetPtr(OptionsPage3GadgetList,GID_P3_ISOVERLAY);
        GadgetPtr->Left=GadViewTypePtr->Left+GadViewTypePtr->Width+20;

        GadgetPtr=Gui_GetCustomGadgetPtr(OptionsPage3GadgetList,GID_P3_OVERLAYTHRESHOLD);
        GadgetPtr->Left=GadViewTypePtr->Left+GadViewTypePtr->Width+20;
    }

    /* Disposition des gadgets de la page 4 */
    {
        struct CustomGadget *GadgetPtr;
        static const ULONG GadgetIDButton[]={GID_P4_SELECT,GID_P4_SEARCH,(ULONG)-1};

        /* Localisation du gadget cycle Style */
        StyleLabels[0]=(STRPTR)Sys_GetString(LOC_OPTIONS_STYLE_GADTOOLS,"Gadtools (Like OS3.1)");
        StyleLabels[1]=(STRPTR)Sys_GetString(LOC_OPTIONS_STYLE_REACTION,"Reaction (Like OS3.5+)");
        StyleLabels[2]=(STRPTR)Sys_GetString(LOC_OPTIONS_STYLE_MUI,"Magic User Interface");
        StyleLabels[3]=NULL;

        /* Agrandissement des boutons */
        Gui_ResizeButtonGadgetHorizontaly(OptionsPage4GadgetList,GadgetIDButton);

        /* Agrandissement des cycles gadgets */
        GadgetPtr=Gui_GetCustomGadgetPtr(OptionsPage4GadgetList,GID_P4_STYLE);
        GadgetPtr->Width=Gui_GetFinalCycleGadgetWidth(GadgetPtr,StyleLabels);

        /* Décalage des gadgets en fonction de la longueur des textes à gauche
           des gadgets, par rapport à la bordure gauche
        */
        Gui_ShiftGadgetsHorizontaly(20,WIN_WIDTH-40,OptionsPage4GadgetList,NULL);
    }
}


/*****
    Mise à jour des gadgets en fonction des paramètres
*****/

void UpdateOptionsGadgetsAttr(struct WindowOptions *wo)
{
    struct ProtocolPrefs *pp=wo->PPrefs;
    struct GlobalPrefs *gp=wo->GPrefs;
    struct Gadget *Ptr;

    /* Initialisations globales */
    /*{
        Ptr=Gui_GetGadgetPtr(OptionsGadgetList,GID_SAVEAS);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,
                GA_Disabled,TRUE,
                TAG_DONE);
    }*/

    /* Initialisation des gadgets de la page 1 */
    {
        ULONG DepthNumber=0;
        if(pp->Depth>=24) DepthNumber=5;
        else if(pp->Depth>=16) DepthNumber=4;
        else if(pp->Depth>=8) DepthNumber=3;
        else if(pp->Depth>=6) DepthNumber=2;
        else if(pp->Depth>=3) DepthNumber=1;

        Ptr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_DEPTH);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCY_Active,(ULONG)DepthNumber,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_SCALE);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTNM_Number,(ULONG)pp->ServerScale,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_ENCODER);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCY_Active,(ULONG)pp->EncoderId,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_ISCOPYRECT);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)!pp->IsNoCopyRect,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_ISLOCALCURSOR);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)!pp->IsNoLocalCursor,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_ISJPEG);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)pp->IsUseJpeg,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_ISZLIB);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)pp->IsChangeZLib,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_JPEG);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,
                GTSL_Level,(ULONG)pp->JpegQuality,
                GA_Disabled,!pp->IsUseJpeg,
                TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage1GadgetList,GID_P1_ZLIB);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,
                GTSL_Level,(ULONG)pp->ZLibLevel,
                GA_Disabled,!pp->IsChangeZLib,
                TAG_DONE);
    }

    /* Initialisation des gadgets de la page 2 */
    {
        Ptr=Gui_GetGadgetPtr(OptionsPage2GadgetList,GID_P2_ISSHARED);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,
                GTCB_Checked,(ULONG)pp->IsShared,
                TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage2GadgetList,GID_P2_ISSTARTICONIFY);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)gp->IsStartIconify,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage2GadgetList,GID_P2_ISVIEWONLY);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,
                GTCB_Checked,(ULONG)gp->IsViewOnly,
                TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage2GadgetList,GID_P2_ISNOCLIPBOARD);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)gp->IsNoClipboard,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage2GadgetList,GID_P2_ISKBEMUL);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)gp->IsKeyboardEmul,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage2GadgetList,GID_P2_ISNOLAMIGAKEY);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)gp->IsNoLAmigaKey,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage2GadgetList,GID_P2_ISREPORTMOUSE);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,
                GTCB_Checked,(ULONG)pp->IsReportMouse,
                TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage2GadgetList,GID_P2_ISFLAG);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)gp->Flag,TAG_DONE);
    }

    /* Initialisation des gadgets de la page 3 */
    {
        ULONG Cycle=gp->DisplayOption;
        Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_DISPLAYTYPE);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCY_Active,Cycle,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_ISTOOLBAR);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)gp->IsShowToolBar,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_ISCURSOR);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)gp->IsCursor,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_VIEWTYPE);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCY_Active,gp->ViewOption,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_ISOVERLAY);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)gp->IsOverlay,TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_OVERLAYTHRESHOLD);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,
                GTIN_Number,(ULONG)gp->OverlayThreshold,
                GA_Disabled,!gp->IsOverlay,
                TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_ISSMOOTH);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,
                GTCB_Checked,(ULONG)gp->IsSmoothed,
                TAG_DONE);

        if(gp->ScrModeSelectType==SCRMODE_SELECT_AUTO) Cycle=0;
        else if(gp->ScrModeSelectType==SCRMODE_SELECT_ASK) Cycle=1;
        else Cycle=2;
        Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_SCREENTYPE);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCY_Active,Cycle,TAG_DONE);
        Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_SCREENMODE);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GA_Disabled,(Cycle==0?TRUE:FALSE),TAG_DONE);
        Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_SELECT);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GA_Disabled,(Cycle==0?TRUE:FALSE),TAG_DONE);

        Ptr=Gui_GetGadgetPtr(OptionsPage3GadgetList,GID_P3_ISSCREENBAR);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTCB_Checked,(ULONG)gp->IsScreenBar,TAG_DONE);
    }

    /* Initialisation des gadgets de la page 4 */
    {
        char Tmp[128];

        Ptr=Gui_GetGadgetPtr(OptionsPage4GadgetList,GID_P4_STYLE);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GA_Disabled,TRUE,TAG_DONE);
        /*
        Ptr=Gui_GetGadgetPtr(OptionsPage4GadgetList,GID_P4_FONT);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GA_Disabled,TRUE,TAG_DONE);
        Ptr=Gui_GetGadgetPtr(OptionsPage4GadgetList,GID_P4_SELECT);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GA_Disabled,TRUE,TAG_DONE);
        */
        Ptr=Gui_GetGadgetPtr(OptionsPage4GadgetList,GID_P4_ICON);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GA_Disabled,TRUE,TAG_DONE);
        Ptr=Gui_GetGadgetPtr(OptionsPage4GadgetList,GID_P4_SEARCH);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GA_Disabled,TRUE,TAG_DONE);

        Cfg_GetControlKeyOptionText(gp->ControlKey,Tmp);
        Ptr=Gui_GetGadgetPtr(OptionsPage4GadgetList,GID_P4_CONTROLKEY);
        GT_SetGadgetAttrs(Ptr,NULL,NULL,GTST_String,(ULONG)Tmp,TAG_DONE);
    }
}


/*****
    Hook pour la gestion de la chaîne Control Key
*****/

M_HOOK_FUNC(StringControlKeyHook, struct Hook *hook, struct SGWork *sgw, ULONG *msg)
{
    ULONG Result=~0;

    if(*msg==SGH_KEY)
    {
        switch(sgw->EditOp)
        {
            case EO_NOOP:
                {
                    ULONG Qual=(ULONG)sgw->IEvent->ie_Qualifier;
                    if(Qual!=0)
                    {
                        ULONG FinalQual=Cfg_GetControlKeyOptionText(Qual,sgw->WorkBuffer);
                        *((ULONG *)hook->h_Data)=FinalQual;
                        sgw->BufferPos=Sys_StrLen(sgw->WorkBuffer);
                        sgw->NumChars=sgw->BufferPos;
                        sgw->Actions|=SGA_REDISPLAY;
                    }
                }
                break;

            case EO_DELBACKWARD:
            case EO_DELFORWARD:
                *((ULONG *)hook->h_Data)=DEFAULT_CONTROLKEY;
                sgw->WorkBuffer[0]=0;
                sgw->Actions|=SGA_REDISPLAY;
                break;

            default:
                sgw->Actions&=~SGA_USE;
                break;
        }

        Result=0;
    }

    return Result;
}
