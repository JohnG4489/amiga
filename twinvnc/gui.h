#ifndef GUI_H
#define GUI_H

/* Liste des classes */
struct CustomClass
{
    Class           *(*MakeClassFunc)(void);
    void            (*FreeClassFunc)(Class *);
    Class           *IClass;
};


/* Liste des motifs */
#define MOTIF_DONE              -1
#define MOTIF_BEVELBOX_NORMAL   0
#define MOTIF_BEVELBOX_RECESSED 1
#define MOTIF_BEVELBOX_DUAL     2
#define MOTIF_BEVELBOX_PLAIN    3
#define MOTIF_RECTFILL_COLOR0   4
#define MOTIF_RECTALT_COLOR2    5

struct CustomMotif
{
    ULONG           MotifID;
    WORD            LeftBase,TopBase,WidthBase,HeightBase;
    WORD            Left,Top,Width,Height;
    APTR            UserData;
};


struct CustMotifAttr
{
    ULONG           TextID;
    char            *DefaultText;
    struct TextAttr *TextAttr;
};


struct FontDef
{
    struct TextFont *CurrentTextFont;
    char CurrentFontName[MAXFONTNAME];
    LONG CurrentFontSize;
};


/* Liste des gadgets */
#define KIND_DONE           -1

struct CustGadgetAttr
{
    ULONG           TextID;
    char            *DefaultText;
    struct TextAttr *TextAttr;
    ULONG           Flags;
};

struct CustomGadget
{
    ULONG           KindID;
    UWORD           GadgetID;
    struct TagItem  *ti;
    WORD            LeftBase,TopBase,WidthBase,HeightBase;
    WORD            Left,Top,Width,Height;
    APTR            UserData;
    ULONG           HelpID;
    char            *DefaultHelp;
    struct Gadget   *GadgetPtr; /* Adresse du gadget des qu'il est alloue */
};


struct FrameGadgets
{
    struct Gadget *FirstGadget;
    struct Image *Image[4];
    struct Gadget *Gadget[6];
    LONG GadgetCount;
};


struct WindowGadgets
{
    struct Gadget *FirstGadget;
    LONG GadgetCount;
    struct CustomGadget *GadgetListPtr;
};


struct CompleteWindow
{
    struct Window *WindowBase;
    APTR VisualInfo;
    struct WindowGadgets *GadgetsBase;
    char *WindowTitle;
    LONG WindowLeft;
    LONG WindowTop;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL Gui_CreateClasses(struct CustomClass *);
extern void Gui_DeleteClasses(struct CustomClass *);
extern void Gui_InitCompleteWindow(struct CompleteWindow *);
extern BOOL Gui_OpenCompleteWindow(struct CompleteWindow *, struct Screen *, struct MsgPort *, const char *, const char *, LONG, LONG, LONG, LONG, struct CustomGadget *, struct CustomMotif *, LONG *);
extern void Gui_CloseCompleteWindow(struct CompleteWindow *);
extern void Gui_CloseWindowSafely(struct Window *);
extern void Gui_GetBestWindowSize(LONG *, LONG *, ULONG);

extern struct FrameGadgets *Gui_CreateFrameGadgets(struct Window *, struct DrawInfo *, ULONG);
extern void Gui_FreeFrameGadgets(struct Window *, struct FrameGadgets *);
extern void Gui_DrawMotifs(struct Window *, APTR, struct CustomMotif *);
extern struct WindowGadgets *Gui_CreateWindowGadgets(struct Window *, APTR, LONG *, struct CustomGadget *);
extern void Gui_FreeWindowGadgets(struct WindowGadgets *);

extern struct CustomMotif *Gui_GetCustomMotifPtr(struct CustomMotif *, ULONG);

extern BOOL Gui_IsGadgetHit(struct Gadget *, WORD, WORD);
extern struct Gadget *Gui_GetGadgetPtr(struct CustomGadget *, ULONG);
extern struct CustomGadget *Gui_GetCustomGadgetPtr(struct CustomGadget *, ULONG);

extern LONG Gui_GetFinalCycleGadgetWidth(struct CustomGadget *, STRPTR *);
extern void Gui_ResizeButtonGadgetHorizontaly(struct CustomGadget *, const ULONG *);
extern void Gui_DisposeGadgetsHorizontaly(LONG, LONG, struct CustomGadget *, const ULONG *, BOOL);
extern void Gui_SpreadGadgetsHorizontaly(LONG, LONG, struct CustomGadget *, const ULONG *);
extern LONG Gui_ShiftGadgetsHorizontaly(LONG, LONG, struct CustomGadget *, const ULONG *);
extern void Gui_CopyObjectPosition(struct CustomGadget *, struct CustomMotif *, ULONG);

extern void Gui_AttachWindowGadgetsToWindow(struct Window *, struct WindowGadgets *);
extern void Gui_RemoveWindowGadgetsFromWindow(struct Window *, struct WindowGadgets *);

extern void Gui_ChangeFontDef(struct FontDef *, char *, LONG);
extern void Gui_FlushFontDef(struct FontDef *);
extern void Gui_InitTextAttr(struct FontDef *, struct TextAttr *, ULONG);

extern LONG *Gui_AllocPalettes(struct ColorMap *, ULONG *, LONG);
extern void Gui_FreePalettes(struct ColorMap *, LONG *);

#endif  /* GUI_H */
