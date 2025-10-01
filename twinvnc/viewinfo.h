#ifndef VIEWINFO_H
#define VIEWINFO_H

/* Sert à connaître la façon de gérer le rastport */
#define DISP_TYPE_NONE              0
#define DISP_TYPE_WINDOW            1
#define DISP_TYPE_PUBLIC_SCREEN     2
#define DISP_TYPE_CUSTOM_SCREEN     3


/* Screen Types */
#define SCRTYPE_GRAPHICS            0
#define SCRTYPE_CYBERGRAPHICS       1


/* Pour savoir quelle type de vue on affiche */
#define VIEW_TYPE_NONE      0
#define VIEW_TYPE_NORMAL    1
#define VIEW_TYPE_SCALE     2
#define VIEW_TYPE_SMOOTH    3
#define VIEW_TYPE_OVERLAY   4


struct ViewInfo
{
    struct Window *WindowBase;
    LONG DisplayType;
    LONG ScreenType;
    LONG ViewType;

    BOOL IsScaled;
    BOOL IsSmoothed;
    BOOL IsFullScreen;
    LONG OverlayThreshold;

    LONG BorderLeft;
    LONG BorderTop;
    LONG BorderRight;
    LONG BorderBottom;

    struct FrameBuffer *FrameBufferPtr;
    LONG ViewX;
    LONG ViewY;
    LONG ViewWidth;
    LONG ViewHeight;
    LONG NormalViewX;
    LONG NormalViewY;

    /* Table des couleurs allouées pour l'affichage */
    LONG ColorMapID[256];
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern void Vinf_InitView(struct ViewInfo *, struct FrameBuffer *);
extern void Vinf_ResetViewProperties(struct ViewInfo *, struct Window *, struct FrameBuffer *, LONG, LONG, LONG, BOOL, BOOL, BOOL, LONG);
extern void Vinf_MoveView(struct ViewInfo *, LONG, LONG);
extern void Vinf_RethinkViewType(struct ViewInfo *);
extern BOOL VInf_PrepareColorMap(struct ViewInfo *);
extern void VInf_FlushColorMap(struct ViewInfo *);
extern BOOL VInf_GetRealMouseXY(struct ViewInfo *, LONG, LONG, LONG *, LONG *);

#endif  /* VIEWINFO_H */
