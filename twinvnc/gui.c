#include "system.h"
#include "gui.h"
#include "twingadgetclass.h"
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <intuition/icclass.h>
#include <graphics/gfxmacros.h>


/*
    22-07-2017 (Segà    Fixe Hit sur gadget image
    07-03-2006 (Seg)    Gestion du ratio de l'interface graphique
    29-03-2004 (Seg)    Gestion de l'interface graphique du projet
*/


#define RATIO_FACTOR_Y_R2   65
#define RATIO_FACTOR_H_R2   80
#define RATIO_FACTOR_H_R4   75
#define RATIO_FACTOR_Y_R4   60


/***** Prototypes */
BOOL Gui_CreateClasses(struct CustomClass *);
void Gui_DeleteClasses(struct CustomClass *);

void Gui_InitCompleteWindow(struct CompleteWindow *);
BOOL Gui_OpenCompleteWindow(struct CompleteWindow *, struct Screen *, struct MsgPort *, const char *, const char *, LONG, LONG, LONG, LONG, struct CustomGadget *, struct CustomMotif *, LONG *);
void Gui_CloseCompleteWindow(struct CompleteWindow *);
void Gui_CloseWindowSafely(struct Window *);
void Gui_GetBestWindowSize(LONG *, LONG *, ULONG);

struct FrameGadgets *Gui_CreateFrameGadgets(struct Window *, struct DrawInfo *, ULONG);
void Gui_FreeFrameGadgets(struct Window *, struct FrameGadgets *);
void Gui_DrawMotifs(struct Window *, APTR, struct CustomMotif *);
struct WindowGadgets *Gui_CreateWindowGadgets(struct Window *, APTR, LONG *, struct CustomGadget *);
void Gui_FreeWindowGadgets(struct WindowGadgets *);

struct CustomMotif *Gui_GetCustomMotifPtr(struct CustomMotif *, ULONG);

BOOL Gui_IsGadgetHit(struct Gadget *, WORD, WORD);
struct Gadget *Gui_GetGadgetPtr(struct CustomGadget *, ULONG);
struct CustomGadget *Gui_GetCustomGadgetPtr(struct CustomGadget *, ULONG);

LONG Gui_GetFinalCycleGadgetWidth(struct CustomGadget *, STRPTR *);
void Gui_ResizeButtonGadgetHorizontaly(struct CustomGadget *, const ULONG *);
void Gui_DisposeGadgetsHorizontaly(LONG, LONG, struct CustomGadget *, const ULONG *, BOOL);
void Gui_SpreadGadgetsHorizontaly(LONG, LONG, struct CustomGadget *, const ULONG *);
LONG Gui_ShiftGadgetsHorizontaly(LONG, LONG, struct CustomGadget *, const ULONG *);
void Gui_CopyObjectPosition(struct CustomGadget *, struct CustomMotif *, ULONG);

void Gui_AttachWindowGadgetsToWindow(struct Window *, struct WindowGadgets *);
void Gui_RemoveWindowGadgetsFromWindow(struct Window *, struct WindowGadgets *);

void Gui_ChangeFontDef(struct FontDef *, char *, LONG);
void Gui_FlushFontDef(struct FontDef *);
void Gui_InitTextAttr(struct FontDef *, struct TextAttr *, ULONG);

LONG *TBar_AllocPalettes(struct ColorMap *, ULONG *, LONG);
void TBar_FreePalettes(struct ColorMap *, LONG *);

#ifndef __amigaos4__ /* This function exists under os4 */
void StripIntuiMessages(struct MsgPort *, struct Window *);
#endif

LONG GetGadgetTextWidth(struct CustomGadget *);
LONG GetFinalGadgetWidth(struct CustomGadget *);
void PrintShadowText(struct RastPort *, LONG, char *, struct TextAttr *, LONG, LONG);


/*****
    Création des classes d'objets graphiques à partir d'une ClassList
*****/

BOOL Gui_CreateClasses(struct CustomClass *ClassList)
{
    if(ClassList!=NULL)
    {
        while(ClassList->MakeClassFunc!=NULL)
        {
            if(!(ClassList->IClass=(ClassList->MakeClassFunc)())) return FALSE;
            ClassList=&ClassList[1];
        }
    }

    return TRUE;
}


/*****
    Libération des classes créées par CreateClasses()
*****/

void Gui_DeleteClasses(struct CustomClass *ClassList)
{
    if(ClassList!=NULL)
    {
        while(ClassList->MakeClassFunc!=NULL)
        {
            if(ClassList->IClass)
            {
                if(ClassList->FreeClassFunc!=NULL) (ClassList->FreeClassFunc)(ClassList->IClass);
                else FreeClass(ClassList->IClass);
            }
            ClassList->IClass=NULL;
            ClassList=&ClassList[1];
        }
    }
}


/*****
    Initialisation de la structure CompleteWindow
*****/

void Gui_InitCompleteWindow(struct CompleteWindow *cw)
{
    cw->WindowBase=NULL;
    cw->VisualInfo=NULL;
    cw->GadgetsBase=NULL;
    cw->WindowTitle=NULL;
    cw->WindowLeft=-1;
    cw->WindowTop=-1;
}


/*****
    Ouverture d'une fenêtre
*****/

BOOL Gui_OpenCompleteWindow(struct CompleteWindow *cw,
    struct Screen *Scr, struct MsgPort *MsgPort,
    const char *ScreenTitle, const char *WindowTitle,
    LONG Left, LONG Top, LONG InnerWidth, LONG InnerHeight,
    struct CustomGadget *GadgetList, struct CustomMotif *MotifList, LONG *ColorMapID)
{
    BOOL IsSuccess=FALSE;

    /* Si les positions sont négatives, on prend les anciennes positions */
    if(Left<0) Left=cw->WindowLeft;
    if(Top<0) Top=cw->WindowTop;

    /* On centre la fenêtre si les positions sont négatives */
    if(Left<0) Left=(Scr->Width-(InnerWidth+Scr->WBorLeft+Scr->WBorRight))/2;
    if(Top<0) Top=(Scr->Height-(InnerHeight+Scr->WBorTop+Scr->WBorBottom+Scr->Font->ta_YSize))/2;

    /* Initialisation du titre de la fenêtre */
    cw->WindowTitle=(char *)WindowTitle;

    if((cw->WindowBase=OpenWindowTags(NULL,
        WA_Left,Left,
        WA_Top,Top,
        WA_InnerWidth,InnerWidth,
        WA_InnerHeight,InnerHeight,
        WA_Flags,WFLG_CLOSEGADGET|WFLG_DEPTHGADGET|WFLG_DRAGBAR|WFLG_SMART_REFRESH|WFLG_ACTIVATE,
        WA_IDCMP,0,
        WA_RMBTrap,TRUE,
        WA_ScreenTitle,(ULONG)ScreenTitle,
        WA_Title,(ULONG)WindowTitle,
        WA_PubScreen,(ULONG)Scr,
        TAG_DONE))!=NULL)
    {
        struct Window *win=cw->WindowBase;

        win->UserPort=MsgPort;
        ModifyIDCMP(win,IDCMP_CLOSEWINDOW|IDCMP_GADGETUP|IDCMP_GADGETDOWN|IDCMP_MOUSEMOVE|IDCMP_VANILLAKEY|IDCMP_GADGETHELP);

        if((cw->VisualInfo=GetVisualInfoA(Scr,NULL))!=NULL)
        {
            if((cw->GadgetsBase=Gui_CreateWindowGadgets(win,cw->VisualInfo,ColorMapID,GadgetList))!=NULL)
            {
                /* Rattachement de la liste des gadgets sur la fenêtre */
                Gui_AttachWindowGadgetsToWindow(win,cw->GadgetsBase);

                /* On dessine les cadres... */
                if(MotifList!=NULL) Gui_DrawMotifs(win,cw->VisualInfo,MotifList);

                /* Rafraîchissement des gadgets sur la fenêtre */
                RefreshGadgets(cw->GadgetsBase->FirstGadget,win,NULL);
                GT_RefreshWindow(win,NULL);

                /* Activation des aides pour les gadgets */
                HelpControl(win,HC_GADGETHELP);

                IsSuccess=TRUE;
            }
        }
    }

    /* En cas d'echec, on ferme les ressources qui sont moitié allouées */
    if(!IsSuccess) Gui_CloseCompleteWindow(cw);

    return IsSuccess;
}


/*****
    Fermeture de la fenêtre
*****/

void Gui_CloseCompleteWindow(struct CompleteWindow *cw)
{
    /* On détache les gadgets... */
    Gui_RemoveWindowGadgetsFromWindow(cw->WindowBase,cw->GadgetsBase);

    /* Puis on libère les ressources des gadgets... */
    Gui_FreeWindowGadgets(cw->GadgetsBase);
    cw->GadgetsBase=NULL;

    /* ...et enfin le visualinfo puisque les gadgets sont fermés */
    FreeVisualInfo(cw->VisualInfo);
    cw->VisualInfo=NULL;

    /* On ferme enfin la fenêtre */
    if(cw->WindowBase!=NULL)
    {
        cw->WindowLeft=(LONG)cw->WindowBase->LeftEdge;
        cw->WindowTop=(LONG)cw->WindowBase->TopEdge;
        Gui_CloseWindowSafely(cw->WindowBase);
    }
    cw->WindowBase=NULL;
}



/*****
    Fermeture d'une fenêtre utilisant un port partagé
*****/

void Gui_CloseWindowSafely(struct Window *win)
{
    /* we forbid here to keep out of race conditions with Intuition */
    Forbid();

    /* send back any messages for this window
     * that have not yet been processed
     */
    StripIntuiMessages(win->UserPort,win);

    /* clear UserPort so Intuition will not free it */
    win->UserPort=NULL;

    /* tell Intuition to stop sending more messages */
    ModifyIDCMP(win,0L);

    /* turn multitasking back on */
    Permit();

    /* and really close the window */
    CloseWindow(win);
}


/*****
    Retourne les dimensions d'une fenêtre en fonction du Ratio
*****/

void Gui_GetBestWindowSize(LONG *Width, LONG *Height, ULONG VerticalRatio)
{
    ULONG Ratio=VerticalRatio>=3?RATIO_FACTOR_Y_R4:(VerticalRatio>=2?RATIO_FACTOR_Y_R2:100);
    *Height=(*Height*Ratio)/100;
}


/*****
    Création des gadgets relatifs à la Frame
*****/

struct FrameGadgets *Gui_CreateFrameGadgets(struct Window *win, struct DrawInfo *di, ULONG IDBase)
{
    struct FrameGadgets *fg=Sys_AllocMem(sizeof(struct FrameGadgets));

    if(fg!=NULL)
    {
        BOOL IsSuccess=TRUE;
        static ULONG ImgID[]={UPIMAGE,DOWNIMAGE,LEFTIMAGE,RIGHTIMAGE};
        ULONG i;

        fg->GadgetCount=0;

        /* Récupération des images des flèches */
        for(i=0; i<4 && IsSuccess; i++)
        {
            if(!(fg->Image[i]=(struct Image *)NewObject(NULL,SYSICLASS,
                SYSIA_Which,ImgID[i],
                SYSIA_DrawInfo,di,
                TAG_DONE))) IsSuccess=FALSE;
        }

        if(IsSuccess)
        {
            LONG BorderV=win->BorderTop+win->BorderBottom;
            LONG BorderH=win->BorderLeft+win->BorderRight;
            LONG HeightU=0,HeightD=0,WidthR=0,WidthL=0;

            /* On recupère les proportions des flèches */
            GetAttr(IA_Height,fg->Image[0],(ULONG *)&HeightU);
            GetAttr(IA_Height,fg->Image[1],(ULONG *)&HeightD);
            GetAttr(IA_Width,fg->Image[2],(ULONG *)&WidthR);
            GetAttr(IA_Width,fg->Image[3],(ULONG *)&WidthL);

            /* Slider vertical */
            if(!(fg->Gadget[0]=NewObject(NULL,PROPGCLASS,
                GA_ID,IDBase,
                GA_Top,win->BorderTop+1,
                GA_RelRight,-win->BorderRight+5,
                GA_Width,win->BorderRight-8,
                GA_RelHeight,-BorderV-2-HeightU-HeightD,
                PGA_Top,0,
                PGA_Total,0,
                PGA_Visible,0,
                PGA_NewLook,TRUE,
                PGA_Borderless,TRUE,
                ICA_TARGET,ICTARGET_IDCMP,
                TAG_DONE))) IsSuccess=FALSE;

            /* Slider Horizontal */
            if(IsSuccess) if(!(fg->Gadget[1]=NewObject(NULL,PROPGCLASS,
                GA_ID,IDBase+1,
                GA_RelBottom,-win->BorderBottom+3,
                GA_Left,win->BorderLeft-1,
                GA_RelWidth,-BorderH-1-WidthR-WidthL,
                GA_Height,win->BorderBottom-4,
                PGA_Top,0,
                PGA_Total,0,
                PGA_Visible,0,
                PGA_Freedom,FREEHORIZ,
                PGA_NewLook,TRUE,
                PGA_Borderless,TRUE,
                ICA_TARGET,ICTARGET_IDCMP,
                GA_Previous,fg->Gadget[0],
                TAG_DONE))) IsSuccess=FALSE;

            /* Flèche verticale "Up" */
            if(IsSuccess) if(!(fg->Gadget[2]=NewObject(NULL,BUTTONGCLASS,
                GA_ID,IDBase+2,
                GA_RelBottom,-win->BorderBottom-HeightU-HeightD+1,
                GA_RelRight,-win->BorderRight+1,
                GA_Image,fg->Image[0],
                ICA_TARGET,ICTARGET_IDCMP,
                GA_Previous,fg->Gadget[1],
                TAG_DONE))) IsSuccess=FALSE;

            /* Flèche verticale "Down" */
            if(IsSuccess) if(!(fg->Gadget[3]=NewObject(NULL,BUTTONGCLASS,
                GA_ID,IDBase+3,
                GA_RelBottom,-win->BorderBottom-HeightU+1,
                GA_RelRight,-win->BorderRight+1,
                GA_Image,fg->Image[1],
                ICA_TARGET,ICTARGET_IDCMP,
                GA_Previous,fg->Gadget[2],
                TAG_DONE))) IsSuccess=FALSE;

            /* Flèche horizontale "right" */
            if(IsSuccess) if(!(fg->Gadget[4]=NewObject(NULL,BUTTONGCLASS,
                GA_ID,IDBase+4,
                GA_RelBottom,-win->BorderBottom+1,
                GA_RelRight,-win->BorderRight-WidthR-WidthL+1,
                GA_Image,fg->Image[2],
                ICA_TARGET,ICTARGET_IDCMP,
                GA_Previous,fg->Gadget[3],
                TAG_DONE))) IsSuccess=FALSE;

            /* Flèche horizontale "left" */
            if(IsSuccess) if(!(fg->Gadget[5]=NewObject(NULL,BUTTONGCLASS,
                GA_ID,IDBase+5,
                GA_RelBottom,-win->BorderBottom+1,
                GA_RelRight,-win->BorderRight-WidthR+1,
                GA_Image,fg->Image[3],
                ICA_TARGET,ICTARGET_IDCMP,
                GA_Previous,fg->Gadget[4],
                TAG_DONE))) IsSuccess=FALSE;

            fg->FirstGadget=fg->Gadget[0];
        }

        if(IsSuccess)
        {
            /* Rattachement de la liste de gadget sur la fenêtre */
            fg->GadgetCount=6;
            AddGList(win,fg->FirstGadget,0,fg->GadgetCount,NULL);
        }
        else
        {
            Gui_FreeFrameGadgets(win,fg);
            fg=NULL;
        }
    }

    return fg;
}


/*****
    Destruction des gadgets alloués par CreateFrameGadgets()
*****/

void Gui_FreeFrameGadgets(struct Window *Win, struct FrameGadgets *fg)
{
    if(Win!=NULL && fg!=NULL)
    {
        ULONG i;

        if(fg->GadgetCount>0) RemoveGList(Win,fg->FirstGadget,fg->GadgetCount);

        for(i=0; i<sizeof(fg->Gadget)/sizeof(struct Gadget *); i++) if(fg->Gadget[i]) DisposeObject((APTR)fg->Gadget[i]);
        for(i=0; i<sizeof(fg->Image)/sizeof(struct Image *); i++) if(fg->Image[i]) DisposeObject((APTR)fg->Image[i]);
        Sys_FreeMem(fg);
    }
}


/*****
    Affichage de motifs graphiques dans la fenêtre
*****/

void Gui_DrawMotifs(struct Window *win, APTR VisualInfo, struct CustomMotif *MotifList)
{
    while(MotifList->MotifID!=MOTIF_DONE)
    {
        LONG Left=(LONG)(win->BorderLeft+MotifList->Left);
        LONG Top=(LONG)(win->BorderTop+MotifList->Top);
        LONG Width=(LONG)MotifList->Width;
        LONG Height=(LONG)MotifList->Height;

        switch(MotifList->MotifID)
        {
            case MOTIF_BEVELBOX_NORMAL:
                DrawBevelBox(win->RPort,Left+1,Top+1,Width-2,Height-2,
                    GT_VisualInfo,(ULONG)VisualInfo,
                    TAG_DONE);
                break;

            case MOTIF_BEVELBOX_RECESSED:
                DrawBevelBox(win->RPort,Left,Top,Width,Height,
                    GTBB_Recessed,TRUE,
                    GT_VisualInfo,(ULONG)VisualInfo,
                    TAG_DONE);
                break;

            case MOTIF_BEVELBOX_DUAL:
                {
                    struct CustMotifAttr *Attr=(struct CustMotifAttr *)MotifList->UserData;

                    DrawBevelBox(win->RPort,Left,Top,Width,Height,
                        GTBB_Recessed,TRUE,
                        GT_VisualInfo,(ULONG)VisualInfo,
                        TAG_DONE);
                    DrawBevelBox(win->RPort,Left+1,Top+1,Width-2,Height-2,
                        GT_VisualInfo,(ULONG)VisualInfo,
                        TAG_DONE);

                    if(Attr!=NULL) PrintShadowText(win->RPort,Attr->TextID,Attr->DefaultText,Attr->TextAttr,Left+10,Top);
                }
                break;

            case MOTIF_BEVELBOX_PLAIN:
                SetAPen(win->RPort,0);
                RectFill(win->RPort,Left+2,Top+1,Left+Width-3,Top+Height-2);
                DrawBevelBox(win->RPort,Left,Top,Width,Height,
                    GT_VisualInfo,(ULONG)VisualInfo,
                    TAG_DONE);
                break;

            case MOTIF_RECTFILL_COLOR0:
                SetAPen(win->RPort,0);
                RectFill(win->RPort,Left,Top,Left+Width-1,Top+Height-1);
                break;

            case MOTIF_RECTALT_COLOR2:
                {
                    static UWORD Pattern[]= /* Tramage */
                    {
                        0xaaaa,
                        0x5555
                    };

                    SetAPen(win->RPort,2);
                    SetBPen(win->RPort,0);
                    SetAfPt(win->RPort,Pattern,sizeof(Pattern)/sizeof(UWORD)-1);
                    RectFill(win->RPort,Left,Top,Left+Width-1,Top+Height-1);
                    SetAfPt(win->RPort,NULL,0);
                }
                break;

        }

        MotifList=&MotifList[1];
    }
}


/*****
    Création des gadgets affichables dans une fenêtre
*****/

struct WindowGadgets *Gui_CreateWindowGadgets(struct Window *win, APTR VisualInfo, LONG *ColorMapID, struct CustomGadget *GadgetList)
{
    struct WindowGadgets *wg=Sys_AllocMem(sizeof(struct WindowGadgets));

    if(wg!=NULL)
    {
        struct Gadget *PrevGadget=CreateContext(&wg->FirstGadget);
        struct NewGadget NewGad;

        wg->GadgetListPtr=GadgetList;

        while(PrevGadget!=NULL && GadgetList->KindID!=KIND_DONE)
        {
            LONG x=win->BorderLeft+GadgetList->Left;
            LONG y=win->BorderTop+GadgetList->Top;

            switch(GadgetList->KindID)
            {
                case GENERIC_KIND:
                    PrevGadget=NewObject(((struct CustomClass *)GadgetList->UserData)->IClass,NULL,
                        GA_ID,GadgetList->GadgetID,
                        GT_VisualInfo,VisualInfo,
                        TIMG_ColorMapID,(LONG)ColorMapID,
                        GA_Left,x,
                        GA_Top,y,
                        GA_Width,GadgetList->Width,
                        GA_Height,GadgetList->Height,
                        GA_Immediate,TRUE,
                        GA_RelVerify,TRUE,
                        GA_GadgetHelp,TRUE,
                        GA_Previous,PrevGadget,
                        TAG_MORE,GadgetList->ti);
                    break;

                default:
                    {
                        struct CustGadgetAttr *cga=(struct CustGadgetAttr *)GadgetList->UserData;

                        NewGad.ng_LeftEdge=x;
                        NewGad.ng_TopEdge=y;
                        NewGad.ng_Width=GadgetList->Width;
                        NewGad.ng_Height=GadgetList->Height;
                        NewGad.ng_GadgetText=(STRPTR)(cga->DefaultText!=NULL?Sys_GetString(cga->TextID,cga->DefaultText):"");
                        NewGad.ng_TextAttr=cga->TextAttr;
                        NewGad.ng_GadgetID=GadgetList->GadgetID;
                        NewGad.ng_Flags=cga->Flags;
                        NewGad.ng_VisualInfo=VisualInfo;
                        NewGad.ng_UserData=NULL;
                        PrevGadget=CreateGadgetA(GadgetList->KindID,PrevGadget,&NewGad,GadgetList->ti);
                    }
                    break;
            }

            if(PrevGadget) PrevGadget->UserData=(APTR)Sys_GetString(GadgetList->HelpID,GadgetList->DefaultHelp);
            GadgetList->GadgetPtr=PrevGadget;
            GadgetList=&GadgetList[1];
        }

        if(!PrevGadget)
        {
            /* Echec! On ferme tout ce qui a pu être alloué */
            Gui_FreeWindowGadgets(wg);
            wg=NULL;
        }
        else
        {
            /* Comptage des gadgets */
            struct Gadget *Ptr=wg->FirstGadget;
            for(wg->GadgetCount=0;Ptr!=NULL;wg->GadgetCount++) Ptr=Ptr->NextGadget;
        }
    }

    return wg;
}


/*****
    Libération des gadgets alloués par CreateAllGadgets()
*****/

void Gui_FreeWindowGadgets(struct WindowGadgets *wg)
{
    if(wg)
    {
        struct CustomGadget *GadgetList=wg->GadgetListPtr;

        FreeGadgets(wg->FirstGadget);

        while(GadgetList->KindID!=KIND_DONE)
        {
            if(GadgetList->KindID==GENERIC_KIND) DisposeObject((APTR)GadgetList->GadgetPtr);
            GadgetList->GadgetPtr=NULL;
            GadgetList=&GadgetList[1];
        }

        Sys_FreeMem(wg);
    }
}


/*****
    Recherche un pointeur sur le motif d'index Idx
*****/

struct CustomMotif *Gui_GetCustomMotifPtr(struct CustomMotif *MotifPtr, ULONG Idx)
{
    ULONG CurrentIdx=0;

    while(MotifPtr->MotifID!=MOTIF_DONE)
    {
        if(CurrentIdx++==Idx) return MotifPtr;
        MotifPtr++;
    }

    return NULL;
}


/*****
    Pour savoir si les coordonnées souris sont dans le gadget
*****/

BOOL Gui_IsGadgetHit(struct Gadget *GadgetPtr, WORD MouseX, WORD MouseY)
{
    if(GadgetPtr!=NULL &&
        MouseX>=GadgetPtr->LeftEdge && MouseY>=GadgetPtr->TopEdge &&
        MouseX<(GadgetPtr->LeftEdge+GadgetPtr->Width) && MouseY<(GadgetPtr->TopEdge+GadgetPtr->Height))
    {
        return TRUE;
    }

    return FALSE;
}


/*****
    Permet d'obtenir le pointeur du gadget en fonction de son ID dans la GadgetList
*****/

struct Gadget *Gui_GetGadgetPtr(struct CustomGadget *GadgetList, ULONG GadgetID)
{
    while(GadgetList->KindID!=KIND_DONE)
    {
        if(GadgetList->GadgetID==GadgetID) return GadgetList->GadgetPtr;
        GadgetList++;
    }

    return NULL;
}


/*****
    Permet d'obtenir le pointeur du customgadget en fonction de son ID dans la GadgetList
*****/

struct CustomGadget *Gui_GetCustomGadgetPtr(struct CustomGadget *GadgetList, ULONG GadgetID)
{
    while(GadgetList->KindID!=KIND_DONE)
    {
        if(GadgetList->GadgetID==GadgetID)
        {
            return GadgetList;
        }
        GadgetList++;
    }

    return NULL;
}


/*****
    Calcule la largeur d'un cycle gadget en fonction de ses ressources
*****/

LONG Gui_GetFinalCycleGadgetWidth(struct CustomGadget *Gadget, STRPTR *LabelList)
{
    LONG MaxWidth=0;
    struct IntuiText text;

    text.FrontPen=0;
    text.BackPen=0;
    text.DrawMode=0;
    text.LeftEdge=0;
    text.TopEdge=0;
    text.ITextFont=((struct CustGadgetAttr *)Gadget->UserData)->TextAttr;
    text.NextText=NULL;

    for(;*LabelList!=NULL;LabelList++)
    {
        LONG Len;

        text.IText=*LabelList;
        Len=IntuiTextLength(&text);
        if(Len>MaxWidth) MaxWidth=Len;
    }
    MaxWidth+=30;
    if(Gadget->Width>MaxWidth) MaxWidth=Gadget->Width;

    return MaxWidth;
}


/*****
    Retaille un bouton en fonction de son texte de ressource
*****/

void Gui_ResizeButtonGadgetHorizontaly(struct CustomGadget *GadgetList, const ULONG *GadgetID)
{
    const ULONG *Ptr;

    for(Ptr=GadgetID; *Ptr!=(ULONG)-1; Ptr++)
    {
        struct CustomGadget *CurGadget=Gui_GetCustomGadgetPtr(GadgetList,*Ptr);
        CurGadget->Width=GetFinalGadgetWidth(CurGadget);
    }
}


/*****
    Dispose une liste de gadgets horizontalement en tenant compte des ressources textes
    utilisées par les gadgets, ainsi qu'en tenant compte de leurs espacements.
*****/

void Gui_DisposeGadgetsHorizontaly(LONG LeftBase, LONG WidthBase, struct CustomGadget *GadgetList, const ULONG *GadgetID, BOOL IsCentered)
{
    const ULONG *Ptr;
    LONG TotalWidth=0,PrevRight=0;

    /* Calcule de la largeur globale des gadgets en tenant compte des espaces
       entre les gadgets.
    */
    for(Ptr=GadgetID; *Ptr!=(ULONG)-1; Ptr++)
    {
        struct CustomGadget *CurGadget=Gui_GetCustomGadgetPtr(GadgetList,*Ptr);

        if(Ptr==GadgetID) PrevRight=CurGadget->Left;
        TotalWidth+=GetFinalGadgetWidth(CurGadget)+(CurGadget->Left-PrevRight);
        PrevRight=CurGadget->Left+CurGadget->Width;
    }

    /* Repositionnement des gadgets en tenant compte des espacements originaux
       des gadgets de la liste.
     */
    if(IsCentered) LeftBase=LeftBase+(WidthBase-TotalWidth)/2;
    for(Ptr=GadgetID; *Ptr!=(ULONG)-1; Ptr++)
    {
        struct CustomGadget *CurGadget=Gui_GetCustomGadgetPtr(GadgetList,*Ptr);
        LONG Width=GetFinalGadgetWidth(CurGadget);

        if(Ptr==GadgetID) PrevRight=CurGadget->Left;
        LeftBase+=CurGadget->Left-PrevRight;
        PrevRight=CurGadget->Left+CurGadget->Width;
        CurGadget->Left=LeftBase;
        CurGadget->Width=Width;
        LeftBase+=Width;
    }

}


/*****
    Dispose une liste de gadgets horizontalement, en tenant compte des ressources textes
    utilisées par ces gadgets.
*****/

void Gui_SpreadGadgetsHorizontaly(LONG LeftBase, LONG WidthBase, struct CustomGadget *GadgetList, const ULONG *GadgetID)
{
    const ULONG *Ptr;
    LONG Space=0,TotalWidth=0,Count=0;

    /* Calcule de la largeur globale des gadgets */
    for(Ptr=GadgetID; *Ptr!=(ULONG)-1; Ptr++)
    {
        struct CustomGadget *CurGadget=Gui_GetCustomGadgetPtr(GadgetList,*Ptr);
        TotalWidth+=GetFinalGadgetWidth(CurGadget);
        Count++;
    }

    if(Count>1) Space=(WidthBase-TotalWidth)/(Count-1);
    for(Ptr=GadgetID; *Ptr!=(ULONG)-1; Ptr++)
    {
        struct CustomGadget *CurGadget=Gui_GetCustomGadgetPtr(GadgetList,*Ptr);
        LONG Width=GetFinalGadgetWidth(CurGadget);

        CurGadget->Left=LeftBase;
        CurGadget->Width=Width;
        LeftBase+=Width+Space;
    }
}


/*****
    Décalage des gadgets en fonction des ressources texte.
    Toute la liste de gadgets est décalée si GadgetID est NULL.
*****/

LONG Gui_ShiftGadgetsHorizontaly(LONG LeftBase, LONG WidthBase, struct CustomGadget *GadgetList, const ULONG *GadgetID)
{
    LONG MaxShift=0;
    struct CustomGadget *CurGadget;
    const ULONG *Ptr;

    /* On calcule le plus grand décalage de gadget par rapport au LeftBase */
    for(Ptr=GadgetID,CurGadget=GadgetList; CurGadget!=NULL; Ptr++)
    {
        if(GadgetID!=NULL) CurGadget=(*Ptr!=(ULONG)-1?Gui_GetCustomGadgetPtr(GadgetList,*Ptr):NULL);
        else if(CurGadget->KindID==KIND_DONE) CurGadget=NULL;

        if(CurGadget!=NULL)
        {
            struct CustGadgetAttr *attr=(struct CustGadgetAttr *)CurGadget->UserData;
            if(attr->Flags==PLACETEXT_LEFT)
            {
                LONG Width=GetGadgetTextWidth(CurGadget)+10;
                LONG CurShift=CurGadget->Left-Width-LeftBase;
                if(CurShift<0 && -CurShift>MaxShift) MaxShift=-CurShift;
            }
            CurGadget++;
        }
    }

    /* On effectue le décalage */
    for(Ptr=GadgetID,CurGadget=GadgetList; CurGadget!=NULL; Ptr++)
    {
        if(GadgetID!=NULL) CurGadget=(*Ptr!=(ULONG)-1?Gui_GetCustomGadgetPtr(GadgetList,*Ptr):NULL);
        else if(CurGadget->KindID==KIND_DONE) CurGadget=NULL;

        if(CurGadget!=NULL)
        {
            CurGadget->Left+=MaxShift;
            if(CurGadget->Left+CurGadget->Width>LeftBase+WidthBase)
            {
                CurGadget->Width=LeftBase+WidthBase-CurGadget->Left;
            }
            CurGadget++;
        }
    }

    return MaxShift;
}


/*****
    Recopie des coordonnées originales de chaque gadget
*****/

void Gui_CopyObjectPosition(struct CustomGadget *GadgetList, struct CustomMotif *MotifList, ULONG VerticalRatio)
{
    ULONG RatioY=VerticalRatio>=3?RATIO_FACTOR_Y_R4:(VerticalRatio>=2?RATIO_FACTOR_Y_R2:100);
    ULONG RatioH=VerticalRatio>=3?RATIO_FACTOR_H_R4:(VerticalRatio>=2?RATIO_FACTOR_H_R2:100);

    RatioY=(RatioY<<16)/100;
    RatioH=(RatioH<<16)/100;
    if(GadgetList!=NULL)
    {

        for(;GadgetList->KindID!=KIND_DONE;GadgetList++)
        {
            GadgetList->Left=GadgetList->LeftBase;
            GadgetList->Top=(GadgetList->TopBase*RatioY)>>16;
            GadgetList->Width=GadgetList->WidthBase;
            GadgetList->Height=(GadgetList->HeightBase*(GadgetList->KindID==LISTVIEW_KIND?RatioY:RatioH))>>16;
        }
    }

    if(MotifList!=NULL)
    {
        for(;MotifList->MotifID!=MOTIF_DONE;MotifList++)
        {
            MotifList->Left=MotifList->LeftBase;
            MotifList->Top=(MotifList->TopBase*RatioY)>>16;
            MotifList->Width=MotifList->WidthBase;
            MotifList->Height=(MotifList->HeightBase*RatioY)>>16;
        }
    }
}


/*****
    Permet d'attacher une liste de WindowGadgets à une Window
*****/

void Gui_AttachWindowGadgetsToWindow(struct Window *Win, struct WindowGadgets *Gad)
{
    if(Win!=NULL && Gad!=NULL)
    {
        AddGList(Win,Gad->FirstGadget,0,Gad->GadgetCount,NULL);
    }
}


/*****
    Permet de supprimer une liste de WindowGadgets d'une CompleteWindow
*****/

void Gui_RemoveWindowGadgetsFromWindow(struct Window *Win, struct WindowGadgets *Gad)
{
    if(Win!=NULL && Gad!=NULL)
    {
        struct Gadget *Ptr=Gad->FirstGadget;
        LONG Count=Gad->GadgetCount;

        /* On détache la liste des gadgets de la fenêtre */
        RemoveGList(Win,Ptr,Count);

        /* On ferme la list qui a été mal fermée par RemoveGList!
           Contrairement à ce que semble dire les autodocs, RemoveGList ne
           met pas à NULL le NextGadget du dernier gadget de la liste!
        */
        while(--Count>0) Ptr=Ptr->NextGadget;
        Ptr->NextGadget=NULL;
    }
}


/*****
    Définition de la font courante
*****/

void Gui_ChangeFontDef(struct FontDef *fd, char *FontName, LONG FontSize)
{
    Sys_StrCopy(fd->CurrentFontName,"topaz.font",sizeof(fd->CurrentFontName)-1);
    fd->CurrentFontSize=8;

    Gui_FlushFontDef(fd);

    if(FontName!=NULL)
    {
        struct TextAttr FontAttr={NULL,0,FS_NORMAL,0};

        FontAttr.ta_Name=FontName;
        FontAttr.ta_YSize=FontSize;
        fd->CurrentTextFont=OpenDiskFont(&FontAttr);

        if(fd->CurrentTextFont!=NULL)
        {
            Sys_StrCopy(fd->CurrentFontName,FontName,sizeof(fd->CurrentFontName)-1);
            fd->CurrentFontSize=FontSize;
        }
    }
}


/*****
    Nettoyage de la font courante
*****/

void Gui_FlushFontDef(struct FontDef *fd)
{
    if(fd->CurrentTextFont!=NULL) CloseFont(fd->CurrentTextFont);
    fd->CurrentTextFont=NULL;
}


/*****
    Permet d'initialiser un text attribut à l'aide de la définition d'une font
*****/

void Gui_InitTextAttr(struct FontDef *fd, struct TextAttr *ta, ULONG Style)
{
    ta->ta_Name=fd->CurrentFontName;
    ta->ta_Style=(UBYTE)Style;

    if(fd->CurrentTextFont!=NULL)
    {
        ta->ta_YSize=fd->CurrentTextFont->tf_YSize;
        ta->ta_Flags=fd->CurrentTextFont->tf_Flags;
    }
    else
    {
        ta->ta_YSize=(UWORD)fd->CurrentFontSize;
        ta->ta_Flags=0;
    }
}


/*****
    Allocation d'une palette graphique en fonction d'une table
    de palettes RGB.
*****/

LONG *Gui_AllocPalettes(struct ColorMap *cm, ULONG *PalRGB, LONG NPal)
{
    LONG *NewPalID=Sys_AllocMem(NPal*sizeof(ULONG)+sizeof(LONG));

    if(NewPalID!=NULL)
    {
        LONG Error=0,i;

        *(NewPalID++)=NPal;

        for(i=0;i<NPal;i++)
        {
            ULONG RGB=PalRGB[i];
            ULONG r=((RGB>>16)&0xff)*0x01010101;
            ULONG v=((RGB>>8)&0xff)*0x01010101;
            ULONG b=(RGB&0xff)*0x01010101;

            NewPalID[i]=ObtainBestPen(cm,r,v,b,OBP_Precision,PRECISION_ICON,TAG_DONE);
            if(NewPalID[i]<0) Error++;
        }

        if(Error>0)
        {
            Gui_FreePalettes(cm,NewPalID);
            NewPalID=NULL;
        }
    }

    return NewPalID;
}


/*****
    Destruction de la palette allouée par TBar_AllocPalettes()
*****/

void Gui_FreePalettes(struct ColorMap *cm, LONG *PalID)
{
    LONG i,NPal=PalID[-1];

    for(i=0;i<NPal;i++)
    {
        if(PalID[i]>=0) ReleasePen(cm,PalID[i]);
    }

    Sys_FreeMem((void *)&PalID[-1]);
}


/*****
    Libération des messages d'une fenêtre sur un port partagé
*****/
#ifndef __amigaos4__ /* This function exists under os4 */
void StripIntuiMessages(struct MsgPort *mp, struct Window *win)
{
    /* remove and reply all IntuiMessages on a port that
     * have been sent to a particular window
     * (note that we don't rely on the ln_Succ pointer
     *  of a message after we have replied it)
     */
    struct Node *succ;
    struct IntuiMessage *msg=(struct IntuiMessage *)mp->mp_MsgList.lh_Head;

    while((succ=msg->ExecMessage.mn_Node.ln_Succ)!=NULL)
    {
        if(msg->IDCMPWindow==win)
        {
            /* Intuition is about to free this message.
             * Make sure that we have politely sent it back.
             */
            Remove((struct Node *)msg);
            ReplyMsg((struct Message *)msg);
        }

        msg=(struct IntuiMessage *)succ;
    }
}
#endif


/*****
    Retourne la longueur en pixel du texte d'un gadget
*****/

LONG GetGadgetTextWidth(struct CustomGadget *cg)
{
    if(cg->KindID!=GENERIC_KIND)
    {
        struct CustGadgetAttr *attr=(struct CustGadgetAttr *)cg->UserData;
        struct IntuiText text;

        text.FrontPen=0;
        text.BackPen=0;
        text.DrawMode=0;
        text.LeftEdge=0;
        text.TopEdge=0;
        text.ITextFont=attr->TextAttr;
        text.IText=(STRPTR)Sys_GetString(attr->TextID,attr->DefaultText);
        text.NextText=NULL;
        return IntuiTextLength(&text);
    }

    return cg->Width;
}


/*****
    Calcule la largeur potentielle d'un gadget en fonction de son texte de ressource
*****/

LONG GetFinalGadgetWidth(struct CustomGadget *Gadget)
{
    LONG Width=GetGadgetTextWidth(Gadget);
    if(Gadget->KindID!=GENERIC_KIND) Width+=10;
    if(Width<Gadget->Width) Width=Gadget->Width;
    return Width;
}


/*****
    Affichage d'un texte ombré
*****/

void PrintShadowText(struct RastPort *rp, LONG TextID, char *DefaultText, struct TextAttr *TextAttr, LONG Left, LONG Top)
{
    struct IntuiText Text;

    Top=Top-(TextAttr->ta_YSize)/2+1;

    Text.FrontPen=0;
    Text.BackPen=0;
    Text.DrawMode=JAM2;
    Text.LeftEdge=0;
    Text.TopEdge=0;
    Text.ITextFont=TextAttr;
    Text.IText=(STRPTR)Sys_GetString(TextID,DefaultText);
    Text.NextText=NULL;

    /* Petite opération pour que ce soit plus design */
    PrintIText(rp,&Text,Left+5,Top+1);
    PrintIText(rp,&Text,Left,Top);

    /* On affiche d'abord l'ombre */
    Text.FrontPen=1;
    PrintIText(rp,&Text,Left+3,Top+1);

    /* On affiche ensuite le texte */
    Text.FrontPen=2;
    Text.DrawMode=JAM1;
    PrintIText(rp,&Text,Left+1,Top);
}
