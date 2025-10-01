#ifndef DISPLAY_H
#define DISPLAY_H

#ifndef FRAMEBUFFER_H
#include "framebuffer.h"
#endif

#ifndef VIEWINFO_H
#include "viewinfo.h"
#endif

#ifndef GFXNONE_H
#include "gfxnone.h"
#endif

#ifndef GFXNORMAL_H
#include "gfxnormal.h"
#endif

#ifndef GFXOVERLAY_H
#include "gfxoverlay.h"
#endif

#ifndef GFXSCALE_H
#include "gfxscale.h"
#endif

#ifndef GFXSMOOTH_H
#include "gfxsmooth.h"
#endif


/* Type de curseur à afficher */
#define VIEWCURSOR_NORMAL           0
#define VIEWCURSOR_HIDDEN           1
#define VIEWCURSOR_VIEWONLY         2


struct CursorData
{
    void *CursorPtr;
    UBYTE *MaskPtr;
    void *TmpPtr;
    LONG PosX;
    LONG PosY;
    LONG Width;
    LONG Height;
    LONG HotX;
    LONG HotY;
    LONG PrevX;
    LONG PrevY;
    LONG PrevWidth;
    LONG PrevHeight;
};


struct Display
{
    struct CursorData Cursor;
    struct FrameBuffer FrameBuffer;
    struct ViewInfo ViewInfo;
    struct GfxInfo GfxNone;
    struct GfxNormal GfxNormal;
    struct GfxOverlay GfxOverlay;
    struct GfxScale GfxScale;
    struct GfxSmooth GfxSmooth;

    /* Paramètres de l'affichage */
    BOOL (*OpenFuncHookPtr)(void *);
    void (*CloseFuncHookPtr)(void *);
    void *FuncHookUserData;
    struct MsgPort *MPort;
    char AppTitle[128];
    char ViewTitle[128];
    char PubScrName[MAXPUBSCREENNAME+1];
    struct GfxInfo *CurrentGfxPtr;

    ULONG ModeID;
    ULONG ScrDepth;
    BOOL IsToolBar;
    BOOL IsScreenBar;
    ULONG PrevDisplayOption;
    WORD AltPos[4];

    /* Propriétés des ressources d'affichage */
    struct Screen *ScreenBase;
    struct DrawInfo *DrawI;
    APTR VisualI;
    struct FrameGadgets *FrameG;

    /* Ressources de la ToolBar */
    struct WindowGadgets *ToolBarG;
    LONG *ToolBarColorID;

    /* Table des couleurs allouées pour l'affichage */
    BOOL IsColorMapInitialized;

    /* Autres */
    LONG InitialWinTop;
    LONG InitialWinLeft;
    LONG InitialWinViewWidth;
    LONG InitialWinViewHeight;
    LONG IsInitialFullScreenProportional;
    ULONG CurrentPointerType;
    BOOL IsOnResize;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern void Disp_Init(struct Display *, BOOL (*)(void *), void (*)(void *), void *, struct MsgPort *, LONG, LONG, LONG, LONG);
extern BOOL Disp_OpenDisplay(struct Display *, char *, BOOL, BOOL, BOOL, LONG, BOOL, BOOL, ULONG, ULONG, ULONG *);
extern void Disp_CloseDisplay(struct Display *);
extern BOOL Disp_ChangeDisplay(struct Display *, BOOL, BOOL, BOOL, LONG, BOOL, BOOL, ULONG, ULONG, ULONG *);

extern BOOL Disp_RethinkViewType(struct Display *, BOOL, BOOL, BOOL);
extern void Disp_MoveCursor(struct Display *, LONG, LONG);
extern void Disp_RefreshCursor(struct Display *);
extern void Disp_ClearCursor(struct Display *);

extern BOOL Disp_ResizeView(struct Display *, LONG, LONG, BOOL);
extern void Disp_GetDisplayDimensions(struct Display *, LONG *, LONG *, LONG *, LONG *);
extern void Disp_SetViewTitle(struct Display *, const char *);
extern void Disp_SetAmigaCursor(struct Display *, ULONG, LONG, LONG, BOOL);

extern LONG Disp_GetScreenType(struct Screen *);

extern BOOL Disp_ManageWindowDisplayMessages(struct Display *, struct IntuiMessage *);

#endif  /* DISPLAY_H */
