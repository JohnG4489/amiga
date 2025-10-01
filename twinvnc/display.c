#include "system.h"
#include "display.h"
#include "twin.h"
#include "gui.h"
#include "toolbar.h"
#include "input.h"
#include <intuition/gadgetclass.h>


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    16-01-2016 (Seg)    Refonte du code et utilisation de la lib ViewInfo
    24-12-2006 (Seg)    Modification de RethinkViewType(). La fonction s'occupe de
                        l'allocation des couleurs maintenant.
    20-02-2006 (Seg)    Remaniement de la gestion des événements
    13-02-2006 (Seg)    Remaniement total et gestion de l'overlay avec un seuil
    03-07-2005 (Seg)    Ajout de la gestion des événements de la vue
    21-05-2005 (Seg)    Gestion du centrage de la fenêtre d'affichage
    25-02-2005 (Seg)    Beaucoup d'améliorations!!!
    23-03-2004 (Seg)    Gestion de l'affichage du projet
*/



/***** Prototypes */
void Disp_Init(struct Display *, BOOL (*)(void *), void (*)(void *), void *, struct MsgPort *, LONG, LONG, LONG, LONG);

BOOL Disp_OpenDisplay(struct Display *, char *, BOOL, BOOL, BOOL, LONG, BOOL, BOOL, ULONG, ULONG, ULONG *);
void Disp_CloseDisplay(struct Display *);
BOOL Disp_ChangeDisplay(struct Display *, BOOL, BOOL, BOOL, LONG, BOOL, BOOL, ULONG, ULONG, ULONG *);

BOOL Disp_RethinkViewType(struct Display *, BOOL, BOOL, BOOL);
void Disp_RefreshViewDisplay(struct Display *);
void Disp_MoveViewPosition(struct Display *, LONG, LONG);
void Disp_MoveCursor(struct Display *, LONG, LONG);
void Disp_RefreshCursor(struct Display *);
void Disp_ClearCursor(struct Display *);

void Disp_SetNewDisplayColorMap(struct Display *);

void Disp_CheckWindowPositionOver(struct Display *);
void Disp_GetDisplayDimensions(struct Display *, LONG *, LONG *, LONG *, LONG *);
void Disp_GetViewMinWH(struct Display *, LONG *, LONG *);
void Disp_ConvertInnerSizeToProportionalSize(struct Display *, struct Screen *, LONG *, LONG *);
BOOL Disp_ResizeWindow(struct Display *, LONG, LONG, BOOL);
BOOL Disp_ResizeView(struct Display *, LONG, LONG, BOOL);
void Disp_RefreshWindowObjects(struct Display *);
void Disp_SetViewTitle(struct Display *, const char *);
void Disp_GetInitialWindowPosition(struct Display *, LONG *, LONG *, LONG *, LONG *);
void Disp_SetDisplayWindowLimits(struct Display *);
void Disp_UpdateWindowFrame(struct Display *, BOOL, BOOL);
void Disp_SetAmigaCursor(struct Display *, ULONG, LONG, LONG, BOOL);

BOOL Disp_GetDimensionFromModeID(ULONG, LONG *, LONG *);
LONG Disp_GetScreenType(struct Screen *);
void Disp_InitPubScreenName(struct Display *);
void Disp_SetIDCMPFlags(struct Window *, ULONG);
BOOL Disp_OpenDisplayCustomScreen(struct Display *, BOOL, BOOL, LONG, ULONG *);
BOOL Disp_OpenDisplayPublicScreen(struct Display *, BOOL, BOOL, LONG, BOOL, BOOL, ULONG *);
BOOL Disp_OpenDisplayWindow(struct Display *, BOOL, BOOL, LONG, BOOL);
void Disp_GetScreenParam(LONG *, LONG *, LONG *, LONG *, LONG *, LONG *, LONG *);
BOOL Disp_ManageWindowDisplayMessages(struct Display *, struct IntuiMessage *);


/***** Données */
#if !defined(__amigaos4__) && !defined(__MORPHOS__)
/* No support in gcc nor in ELF for chip qualifier under os4 */
UWORD __chip PointerGfx[4]={0,0,0,0};
#else
UWORD PointerGfx[4]={0,0,0,0};
#endif

#if !defined(__amigaos4__) && !defined(__MORPHOS__)
UWORD __chip PointerViewOnlyGfx[]=
#else
UWORD PointerViewOnlyGfx[]=
#endif
{
    0x0000,0x0000, /* reserved, must be NULL */
    0x4f20,0x7fe0,0x8010,0xd9b0,
    0x30e0,0x8010,0x0900,0x76e0,
    0x76e0,0xfff0,0x56a0,0xfff0,
    0x74e0,0xfff0,0x8d10,0xfff0,
    0xf9f0,0xfff0,0x1f80,0x7fe0,
    0x2040,0x7fe0,0x1f80,0x3fc0,
    0x0000,0x1f80,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x0000,
    0x0000,0x0000  /* reserved, must be NULL */
};


/*****************************************************************************/
/*****************************************************************************/
/*****                                                                   *****/
/*****  GESTION DE LA ZONE D'AFFICHAGE                                   *****/
/*****                                                                   *****/
/*****************************************************************************/
/*****************************************************************************/

/*****
    Initialisation de la structure Display
*****/

void Disp_Init(
    struct Display *disp,
    BOOL (*OpenFuncHookPtr)(void *), void (*CloseFuncHookPtr)(void *), void *FuncHookUserData,
    struct MsgPort *MPort,
    LONG Left, LONG Top, LONG Width, LONG Height)
{
    Vinf_InitView(&disp->ViewInfo,&disp->FrameBuffer);

    disp->PrevDisplayOption=DISPLAY_OPTION_FULLSCREEN;
    disp->OpenFuncHookPtr=OpenFuncHookPtr;
    disp->CloseFuncHookPtr=CloseFuncHookPtr;
    disp->FuncHookUserData=FuncHookUserData;
    disp->MPort=MPort;

    disp->InitialWinLeft=Left;
    disp->InitialWinTop=Top;
    disp->InitialWinViewWidth=Width;
    disp->InitialWinViewHeight=Height;
    disp->IsInitialFullScreenProportional=FALSE;

    disp->CurrentGfxPtr=&disp->GfxNone;
    Gfxno_Reset(disp->CurrentGfxPtr);

    /* Initialisation des propriétés d'affichage */
    Sys_StrCopy(disp->AppTitle,"",sizeof(disp->AppTitle));
    Sys_StrCopy(disp->ViewTitle,"",sizeof(disp->ViewTitle));
    Sys_StrCopy(disp->PubScrName,"",sizeof(disp->PubScrName));
}


/*****
    Ouverture d'une zone d'affichage écran ou fenêtre.
*****/

BOOL Disp_OpenDisplay(struct Display *disp,
    char *AppTitle,
    BOOL IsFullScreen,
    BOOL IsScaled, BOOL IsSmoothed, LONG OverlayThreshold,
    BOOL IsToolBar, BOOL IsScreenBar,
    ULONG ModeID, ULONG ScrDepth,
    ULONG *ErrorCode)
{
    BOOL IsSuccess;
    BOOL IsRefreshDisplay=TRUE;

    *ErrorCode=0;

    /* Changement des paramètres */
    Sys_StrCopy(disp->AppTitle,AppTitle,sizeof(disp->AppTitle));
    disp->ModeID=ModeID;
    disp->ScrDepth=ScrDepth;
    disp->IsScreenBar=IsScreenBar;
    disp->IsToolBar=IsToolBar;

    /* Ouverture de l'affichage */
    if(IsFullScreen)
    {
        IsSuccess=Disp_OpenDisplayPublicScreen(disp,IsScaled,IsSmoothed,OverlayThreshold,IsToolBar,IsScreenBar,ErrorCode);
    }
    else
    {
        IsSuccess=Disp_OpenDisplayWindow(disp,IsScaled,IsSmoothed,OverlayThreshold,IsToolBar);
        if(disp->ViewInfo.ViewType!=VIEW_TYPE_OVERLAY) IsRefreshDisplay=FALSE;
    }

    /* Appel du Hook d'ouverture utilisateur après l'ouverture de l'affichage */
    if(IsSuccess && disp->OpenFuncHookPtr!=NULL)
    {
        IsSuccess=disp->OpenFuncHookPtr(disp->FuncHookUserData);
    }

    if(!IsSuccess)
    {
        Disp_CloseDisplay(disp);
    }
    else
    {
        /* Activation des aides pour les gadgets */
        HelpControl(disp->ViewInfo.WindowBase,HC_GADGETHELP);

        /* Formalités d'affichage */
        TVNC_SetWindowTitles(disp->ViewInfo.WindowBase,disp->ViewTitle,disp->ViewTitle);
        Disp_SetAmigaCursor(disp,VIEWCURSOR_NORMAL,0,0,TRUE);
        Disp_RefreshWindowObjects(disp);
        if(IsRefreshDisplay) Disp_RefreshViewDisplay(disp);
    }

    return IsSuccess;
}


/*****
    Fermeture de l'écran et libération de la structure Display
*****/

void Disp_CloseDisplay(struct Display *disp)
{
    struct ViewInfo *vi=&disp->ViewInfo;
    struct Window *win=vi->WindowBase;

    /* Appel du hook de fermeture utilisateur avant la fermeture de l'affichage */
    if(disp->CloseFuncHookPtr!=NULL) disp->CloseFuncHookPtr(disp->FuncHookUserData);

    /* Sauvegarde des proportions de la fenêtre uniquement si on était en mode
       Window. Les coordonnées doivent être sauvées avant ce qui suit!
    */
    if(vi->DisplayType==DISP_TYPE_WINDOW)
    {
        disp->InitialWinLeft=(LONG)win->LeftEdge;
        disp->InitialWinTop=(LONG)win->TopEdge;
        disp->InitialWinViewWidth=vi->ViewWidth;
        disp->InitialWinViewHeight=vi->ViewHeight;
    }

    /* Fermeture des ressources allouées pour les affichage Overlay, Scale, etc. */
    Vinf_ResetViewProperties(vi,win,&disp->FrameBuffer,vi->ScreenType,DISP_TYPE_NONE,0,vi->IsScaled,vi->IsSmoothed,vi->IsFullScreen,vi->OverlayThreshold);
    Disp_RethinkViewType(disp,FALSE,FALSE,FALSE);

    /* Fermeture de la toolbar et de ses ressources */
    TBar_FreeToolBar(disp);

    /* Fermeture d'autres ressources */
    FreeVisualInfo(disp->VisualI);
    disp->VisualI=NULL;

    /* Fermeture de la fenêtre */
    if(win!=NULL)
    {
        /* Fermeture des gadgets de la fenêtre s'ils existent */
        Gui_FreeFrameGadgets(win,disp->FrameG);
        disp->FrameG=NULL;

        /* Fermeture des ressources reliées à l'écran */
        if(disp->DrawI!=NULL)
        {
            FreeScreenDrawInfo(win->WScreen,disp->DrawI);
            disp->DrawI=NULL;
        }

        /* Libération de la palette */
        VInf_FlushColorMap(vi);

        Gui_CloseWindowSafely(win);
        vi->WindowBase=NULL;
    }

    /* Fermeture de l'écran */
    if(disp->ScreenBase!=NULL)
    {
        while(!CloseScreen(disp->ScreenBase))
        {
            Delay(50);
        }

        disp->ScreenBase=NULL;
    }
}


/*****
    Changement des propriétés d'affichage
*****/

BOOL Disp_ChangeDisplay(
    struct Display *disp,
    BOOL IsFullScreen,
    BOOL IsScaled, BOOL IsSmoothed, LONG OverlayThreshold,
    BOOL IsToolBar, BOOL IsScreenBar,
    ULONG ModeID, ULONG ScrDepth,
    ULONG *ErrorCode)
{
    BOOL IsSuccess=TRUE;
    struct ViewInfo *vi=&disp->ViewInfo;
    struct FrameBuffer *fb=&disp->FrameBuffer;
    struct Window *win=vi->WindowBase;
    BOOL IsShowToolBar=FALSE,IsHideToolBar=FALSE;

    if(IsToolBar && !disp->IsToolBar) IsShowToolBar=TRUE;
    if(!IsToolBar && disp->IsToolBar) IsHideToolBar=TRUE;

    /* Paramètres de l'écran */
    disp->ModeID=ModeID;
    disp->ScrDepth=ScrDepth;
    disp->IsScreenBar=IsScreenBar;
    disp->IsToolBar=IsToolBar;

    *ErrorCode=0;
    switch(vi->DisplayType)
    {
        case DISP_TYPE_NONE:
            /* Recalcul des paramètres de la vue */
            Vinf_ResetViewProperties(vi,win,fb,vi->ScreenType,DISP_TYPE_NONE,0,IsScaled,IsSmoothed,IsFullScreen,OverlayThreshold);
            IsSuccess=Disp_RethinkViewType(disp,FALSE,FALSE,FALSE);
            break;

        case DISP_TYPE_CUSTOM_SCREEN:
        case DISP_TYPE_PUBLIC_SCREEN:
            /* Fermeture de l'écran courant */
            Disp_CloseDisplay(disp);

            /* Réouverture de l'écran avec les nouveaux paramètres */
            IsSuccess=Disp_OpenDisplay(disp,disp->ViewTitle,IsFullScreen,IsScaled,IsSmoothed,OverlayThreshold,IsToolBar,IsScreenBar,ModeID,ScrDepth,ErrorCode);
            break;

        case DISP_TYPE_WINDOW:
            if(IsFullScreen)
            {
                /* Fermeture de l'écran courant */
                Disp_CloseDisplay(disp);

                /* Réouverture de l'écran avec les nouveaux paramètres */
                IsSuccess=Disp_OpenDisplay(disp,disp->ViewTitle,IsFullScreen,IsScaled,IsSmoothed,OverlayThreshold,IsToolBar,IsScreenBar,ModeID,ScrDepth,ErrorCode);
            }
            else
            {
                LONG MargeTop=IsToolBar?TBar_GetToolBarHeight():0;

                /* Attachement ou détachement de la toolbar */
                if(IsShowToolBar) TBar_ShowToolBar(disp);
                if(IsHideToolBar) TBar_HideToolBar(disp);

                /* Recalcul des paramètres de la vue */
                Vinf_ResetViewProperties(vi,win,fb,vi->ScreenType,vi->DisplayType,MargeTop,IsScaled,IsSmoothed,IsFullScreen,OverlayThreshold);
                /* NOTE: Disp_ResizeView() lance le RethinkView! */

                /* On retaille la taille fenêtre */
                IsSuccess=Disp_ResizeView(disp,vi->ViewWidth,vi->ViewHeight,FALSE);
            }
            break;
    }

    if(!IsSuccess) Disp_CloseDisplay(disp);

    return IsSuccess;
}


/*****************************************************************************/
/*****************************************************************************/
/*****                                                                   *****/
/*****  GESTION DE LA VUE                                                *****/
/*****                                                                   *****/
/*****************************************************************************/
/*****************************************************************************/

/*****
    Recalcule les buffers d'affichage et définit le type d'affichage de la vue.
    Quatre types sont possibles: Normal, Scale, Smooth et Overlay

    NOTE: Cette fonction s'utilise APRES un Vinf_ResetViewProperties()!!!
    Il faut bien comprendre que cette fonction permet à la fois d'allouer
    des ressources pour l'affichage ET de désallouer les ressources qu'elle
    a elle-même demandé.
*****/

BOOL Disp_RethinkViewType(struct Display *disp, BOOL IsNewFrameBuffer, BOOL IsForceRefreshDisplay, BOOL IsForceRefreshColors)
{
    BOOL IsSuccess=TRUE;
    struct ViewInfo *vi=&disp->ViewInfo;

    Vinf_RethinkViewType(vi);
    disp->CurrentGfxPtr=&disp->GfxNone;

    /* Initialisation de la vue */
    Gfxn_Free(&disp->GfxNormal);
    Gfxs_Free(&disp->GfxScale);
    Gfxsm_Free(&disp->GfxSmooth);

    if(IsForceRefreshColors || !disp->IsColorMapInitialized)
    {
        /* Mise à jour des couleurs de l'écran si 8 bits */
        Disp_SetNewDisplayColorMap(disp);
    }

    switch(vi->ViewType)
    {
        case VIEW_TYPE_NONE:
            Gfxo_Free(&disp->GfxOverlay);
            IsForceRefreshDisplay=FALSE;
            break;

        case VIEW_TYPE_NORMAL:
            Gfxo_Free(&disp->GfxOverlay);
            disp->CurrentGfxPtr=(struct GfxInfo *)&disp->GfxNormal;
            IsSuccess=Gfxn_Create(&disp->GfxNormal,vi);
            break;

        case VIEW_TYPE_SCALE:
            Gfxo_Free(&disp->GfxOverlay);
            disp->CurrentGfxPtr=(struct GfxInfo *)&disp->GfxScale;
            IsSuccess=Gfxs_Create(&disp->GfxScale,vi);
            break;

        case VIEW_TYPE_SMOOTH:
            Gfxo_Free(&disp->GfxOverlay);
            disp->CurrentGfxPtr=(struct GfxInfo *)&disp->GfxSmooth;
            IsSuccess=Gfxsm_Create(&disp->GfxSmooth,vi);
            break;

        case VIEW_TYPE_OVERLAY:
            disp->CurrentGfxPtr=(struct GfxInfo *)&disp->GfxOverlay;
            if(IsNewFrameBuffer || disp->GfxOverlay.VLHandle==NULL)
            {
                Gfxo_Free(&disp->GfxOverlay);
                if(Gfxo_Create(&disp->GfxOverlay,&disp->ViewInfo)==OVERLAY_SUCCESS)
                {
                    IsSuccess=TRUE;
                }
                /* Si l'overlay n'est pas possible, on passe en scale soft */
                else if(vi->IsSmoothed)
                {
                    vi->ViewType=VIEW_TYPE_SMOOTH;
                    disp->CurrentGfxPtr=(struct GfxInfo *)&disp->GfxSmooth;
                    IsSuccess=Gfxsm_Create(&disp->GfxSmooth,vi);
                }
                else
                {
                    vi->ViewType=VIEW_TYPE_SCALE;
                    disp->CurrentGfxPtr=(struct GfxInfo *)&disp->GfxScale;
                    IsSuccess=Gfxs_Create(&disp->GfxScale,vi);
                }
            }
            else
            {
                /* Réutilisation de l'overlay déjà ouvert */
                IsForceRefreshDisplay=FALSE;
            }
            break;
    }

    if(IsSuccess)
    {
        if(disp->CurrentGfxPtr->UpdateParamFuncPtr!=NULL) disp->CurrentGfxPtr->UpdateParamFuncPtr(disp->CurrentGfxPtr);
        if(IsForceRefreshDisplay) Disp_RefreshViewDisplay(disp);
    }

    return IsSuccess;
}


/*****
    Rafraîchissement de la vue
*****/

void Disp_RefreshViewDisplay(struct Display *disp)
{
    disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,0,0,disp->FrameBuffer.Width,disp->FrameBuffer.Height);
    Disp_RefreshCursor(disp);
}


/*****
    Déplacement de la vue sur de nouvelles coordonnées
*****/

void Disp_MoveViewPosition(struct Display *disp, LONG TopX, LONG TopY)
{
    struct ViewInfo *vi=&disp->ViewInfo;
    struct GfxInfo *gfx=disp->CurrentGfxPtr;
    LONG DeltaX=TopX-vi->ViewX;
    LONG DeltaY=TopY-vi->ViewY;
    LONG AbsDeltaX=DeltaX;
    LONG AbsDeltaY=DeltaY;

    if(AbsDeltaX<0) AbsDeltaX=-AbsDeltaX;
    if(AbsDeltaY<0) AbsDeltaY=-AbsDeltaY;

    ClipBlit(vi->WindowBase->RPort,
        vi->BorderLeft+(DeltaX>=0?DeltaX:0),vi->BorderTop+(DeltaY>=0?DeltaY:0),
        vi->WindowBase->RPort,
        vi->BorderLeft+(DeltaX>=0?0:-DeltaX),vi->BorderTop+(DeltaY>=0?0:-DeltaY),
        vi->ViewWidth-AbsDeltaX,vi->ViewHeight-AbsDeltaY,0xc0);

    Vinf_MoveView(vi,TopX,TopY);
    if(gfx->UpdateParamFuncPtr!=NULL) gfx->UpdateParamFuncPtr(gfx);

    if(DeltaY<0) gfx->RefreshRectFuncPtr(gfx,TopX,TopY,vi->ViewWidth,AbsDeltaY);
    else if(DeltaY>0) gfx->RefreshRectFuncPtr(gfx,TopX,TopY+vi->ViewHeight-AbsDeltaY,vi->ViewWidth,AbsDeltaY);

    if(DeltaX<0) gfx->RefreshRectFuncPtr(gfx,TopX,TopY,AbsDeltaX,vi->ViewHeight);
    else if(DeltaX>0) gfx->RefreshRectFuncPtr(gfx,TopX+vi->ViewWidth-AbsDeltaX,TopY,AbsDeltaX,vi->ViewHeight);
}


/*****
    Affichage du curseur souris distant à la position (x,y)
*****/

void Disp_MoveCursor(struct Display *disp, LONG x, LONG y)
{
    struct CursorData *cd=&disp->Cursor;

    cd->PosX=x;
    cd->PosY=y;
    x-=cd->HotX;
    y-=cd->HotY;
    if(cd->CursorPtr!=NULL)
    {
        struct FrameBuffer *fb=&disp->FrameBuffer;
        LONG x1=x,y1=y;
        LONG w=cd->Width,h=cd->Height;

        /* Clipping */
        if(Frb_ClipToFrameBuffer(fb,&x1,&y1,&w,&h))
        {
            ULONG Offset=(x1-x)+cd->Width*(y1-y);
            void *NewCursor=(void *)&((UBYTE *)cd->CursorPtr)[Offset*fb->PixelBufferSize];

            /* Sauvegarde du framebuffer */
            Frb_ReadFromFrameBuffer(fb,x1,y1,w,h,cd->TmpPtr);

            /* Application du motif dans le framebuffer */
            Frb_CopyRectMasked(fb,NewCursor,&cd->MaskPtr[Offset],cd->Width,x1,y1,w,h);

            /* On efface l'ancien curseur */
            Disp_ClearCursor(disp);
            cd->PrevX=x1;
            cd->PrevY=y1;
            cd->PrevWidth=w;
            cd->PrevHeight=h;

            /* On affiche le nouveau curseur */
            disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,x1,y1,w,h);

            /* On efface le curseur du framebuffer */
            Frb_CopyRectFromBuffer(fb,cd->TmpPtr,fb->PixelFormatSize,x1,y1,w,h);
        }
    }
}


/*****
    Rafraîchissement du curseur
*****/

void Disp_RefreshCursor(struct Display *disp)
{
    Disp_MoveCursor(disp,disp->Cursor.PosX,disp->Cursor.PosY);
}


/*****
    Effacement du curseur
*****/

void Disp_ClearCursor(struct Display *disp)
{
    struct CursorData *cd=&disp->Cursor;
    LONG PrevX=cd->PrevX;
    LONG PrevY=cd->PrevY;

    if(PrevX>=0 && PrevX<disp->FrameBuffer.Width &&
       PrevY>=0 && PrevY<disp->FrameBuffer.Height)
    {
        LONG PrevW=cd->PrevWidth;
        LONG PrevH=cd->PrevHeight;

        if(PrevW>0 && PrevH>0)
        {
            disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,PrevX,PrevY,PrevW,PrevH);
            cd->PrevX=0;
            cd->PrevY=0;
            cd->PrevWidth=0;
            cd->PrevHeight=0;
        }
    }
}


/*****************************************************************************/
/*****************************************************************************/
/*****                                                                   *****/
/*****  GESTION DES COULEURS                                             *****/
/*****                                                                   *****/
/*****************************************************************************/
/*****************************************************************************/

/*****
    Permet de lancer l'allocation et le changement de palette de couleurs
    quel que soit le type d'écran utilisé.
*****/

void Disp_SetNewDisplayColorMap(struct Display *disp)
{
    struct ViewInfo *vi=&disp->ViewInfo;
    struct Window *win=vi->WindowBase;

    if(win!=NULL)
    {
        disp->IsColorMapInitialized=FALSE;
        VInf_FlushColorMap(vi);

        if(vi->ScreenType==SCRTYPE_GRAPHICS && vi->ViewType!=VIEW_TYPE_OVERLAY)
        {
            disp->IsColorMapInitialized=TRUE;

            switch(vi->DisplayType)
            {
                case DISP_TYPE_CUSTOM_SCREEN:
                    {
                        struct FrameBuffer *fb=&disp->FrameBuffer;
                        static ULONG Pal[1+3*256+1];
                        ULONG *PalDst=Pal;
                        LONG i;

                        *PalDst++=fb->ColorCount<<16;

                        for(i=0; i<fb->ColorCount; i++)
                        {
                            LONG RGB=fb->ColorMapRGB32[i];
                            *PalDst++=(ULONG)((RGB>>16)&0xff)*0x01010101;
                            *PalDst++=(ULONG)((RGB>>8)&0xff)*0x01010101;
                            *PalDst++=(ULONG)(RGB&0xff)*0x01010101;
                        }

                        *PalDst=0;
                        LoadRGB32(&win->WScreen->ViewPort,Pal);
                    }
                    break;

                case DISP_TYPE_PUBLIC_SCREEN:
                case DISP_TYPE_WINDOW:
                    VInf_PrepareColorMap(vi);
                    break;
            }
        }
    }
}


/*****************************************************************************/
/*****************************************************************************/
/*****                                                                   *****/
/*****  GESTION DE L'INTERFACE GRAPHIQUE                                 *****/
/*****                                                                   *****/
/*****************************************************************************/
/*****************************************************************************/

/*****
    Permet de repositionner la fenêtre sur l'écran quand celle-ci déborde de l'écran.
    (Cas Amiga OS4 et MorphOS)
*****/

void Disp_CheckWindowPositionOver(struct Display *disp)
{
    struct ViewInfo *vi=&disp->ViewInfo;
    struct Window *win=vi->WindowBase;

    if(win!=NULL)
    {
        struct Screen *Scr=win->WScreen;
        LONG PosX=(LONG)win->LeftEdge;
        LONG PosY=(LONG)win->TopEdge;
        LONG DeltaX=(LONG)Scr->Width-(PosX+(LONG)win->Width);
        LONG DeltaY=(LONG)Scr->Height-(PosY+(LONG)win->Height);
        if(DeltaX<0) PosX+=DeltaX;
        if(DeltaY<0) PosY+=DeltaY;
        if(PosX<0) PosX=0;
        if(PosY<0) PosY=0;
        if(PosX!=(LONG)win->LeftEdge || PosY!=(LONG)win->TopEdge)
        {
            ChangeWindowBox(win,(WORD)PosX,(WORD)PosY,win->Width,win->Height);
        }
    }
}


/*****
    Retourne les dimensions actuelles de la vue, et la position de la fenêtre
*****/

void Disp_GetDisplayDimensions(struct Display *disp, LONG *WinLeft, LONG *WinTop, LONG *ViewWidth, LONG *ViewHeight)
{
    struct ViewInfo *vi=&disp->ViewInfo;

    if(vi->DisplayType==DISP_TYPE_WINDOW)
    {
        struct Window *win=vi->WindowBase;
        *WinLeft=(LONG)win->LeftEdge;
        *WinTop=(LONG)win->TopEdge;
        *ViewWidth=vi->ViewWidth;
        *ViewHeight=vi->ViewHeight;
    }
    else
    {
        *WinLeft=disp->InitialWinLeft;
        *WinTop=disp->InitialWinTop;
        *ViewWidth=disp->InitialWinViewWidth;
        *ViewHeight=disp->InitialWinViewHeight;
    }
}


/*****
    Retourne les dimensions minimales que la vue peut adopter
*****/

void Disp_GetViewMinWH(struct Display *disp, LONG *MinWidth, LONG *MinHeight)
{
    *MinWidth=disp->IsToolBar?TBar_GetToolBarWidth():0;
    *MinHeight=0;
}


/*****
    Convertion des dimensions dans les bonnes proportions par rapport au framebuffer
    et par rapport à la résolution de l'écran.
*****/

void Disp_ConvertInnerSizeToProportionalSize(struct Display *disp, struct Screen *Scr, LONG *Width, LONG *Height)
{
    struct FrameBuffer *fb=&disp->FrameBuffer;
    LONG FactorW,FactorH,Ratio=1;
    struct DrawInfo *sdi=GetScreenDrawInfo(Scr);

    if(sdi!=NULL)
    {
        LONG X=(LONG)sdi->dri_Resolution.X;
        if(X!=0) Ratio=(((LONG)sdi->dri_Resolution.Y<<16)/X+0x8000)>>16;
        FreeScreenDrawInfo(Scr,sdi);
    }

    if(*Width<0) *Width=0;
    if(*Height<0) *Height=0;
    FactorW=((*Width<<16)+0x7fff)/fb->Width;
    FactorH=((*Height<<16)+0x7fff)/fb->Height;

    if(fb->Height>fb->Width) *Width=((fb->Width*FactorH+0x7fff)>>16);
    else *Height=((fb->Height*FactorW+0x7fff)>>16)/Ratio;
}


/*****
    Retaille la fenêtre avec les nouvelles dimensions, en tenant compte de la toolbar
*****/

BOOL Disp_ResizeWindow(struct Display *disp, LONG NewWidth, LONG NewHeight, BOOL IsKeepProportional)
{
    BOOL IsSuccess=TRUE,IsSizeChanged=FALSE;
    struct ViewInfo *vi=&disp->ViewInfo;
    struct Window *win=vi->WindowBase;
    LONG MinWidth,MinHeight;

    if(IsKeepProportional)
    {
        NewWidth-=vi->BorderLeft+vi->BorderRight;
        NewHeight-=vi->BorderTop+vi->BorderBottom;
        Disp_ConvertInnerSizeToProportionalSize(disp,win->WScreen,&NewWidth,&NewHeight);
        NewWidth+=vi->BorderLeft+vi->BorderRight;
        NewHeight+=vi->BorderTop+vi->BorderBottom;
    }

    /* Redéfinition des limites de la fenêtre pour permettre le ChangeWindowBox() */
    Disp_SetDisplayWindowLimits(disp);

    /* On fait gaffe à la présence de la toolbar avant de retailler la fenêtre */
    Disp_GetViewMinWH(disp,&MinWidth,&MinHeight);

    MinWidth+=vi->BorderLeft+vi->BorderRight;
    MinHeight+=vi->BorderTop+vi->BorderBottom;
    if(NewWidth<MinWidth) NewWidth=MinWidth;
    if(NewHeight<MinHeight) NewHeight=MinHeight;
    if((LONG)win->Width!=NewWidth || (LONG)win->Height!=NewHeight) IsSizeChanged=TRUE;

    if(!IsSizeChanged)
    {
        Disp_RefreshWindowObjects(disp);
        IsSuccess=Disp_RethinkViewType(disp,FALSE,TRUE,FALSE);
    }
    else
    {
        disp->IsOnResize=TRUE;
        ChangeWindowBox(win,win->LeftEdge,win->TopEdge,NewWidth,NewHeight);
    }

    return IsSuccess;
}


/*****
    Retaille l'intérieur de la vue aux dimensions spécifiées
*****/

BOOL Disp_ResizeView(struct Display *disp, LONG NewInnerWidth, LONG NewInnerHeight, BOOL IsKeepProportional)
{
    BOOL IsSuccess=TRUE;
    struct ViewInfo *vi=&disp->ViewInfo;

    if(vi->DisplayType!=DISP_TYPE_NONE)
    {
        struct FrameBuffer *fb=&disp->FrameBuffer;
        LONG Width=vi->BorderLeft+vi->BorderRight;
        LONG Height=vi->BorderTop+vi->BorderBottom;

        if(!vi->IsScaled)
        {
            if(NewInnerWidth>fb->Width) NewInnerWidth=fb->Width;
            if(NewInnerHeight>fb->Height) NewInnerHeight=fb->Height;
        }

        Width+=(NewInnerWidth>=0)?NewInnerWidth:0;
        Height+=(NewInnerHeight>=0)?NewInnerHeight:0;

        IsSuccess=Disp_ResizeWindow(disp,Width,Height,IsKeepProportional);
    }

    return IsSuccess;
}


/*****
    Rafraîchissement des objets de la fenêtre (Bordures et ToolBar)
*****/

void Disp_RefreshWindowObjects(struct Display *disp)
{
    struct ViewInfo *vi=&disp->ViewInfo;

    if(vi->DisplayType!=DISP_TYPE_NONE)
    {
        /* Mise à jour des proportions des sliders de la fenêtre */
        Disp_UpdateWindowFrame(disp,FALSE,FALSE);

        /* Rafraîchissement des bordures de la fenêtre */
        RefreshWindowFrame(vi->WindowBase);

        /* Rafraîchissement de la toolbar */
        TBar_RefreshToolBar(disp);
    }
}


/*****
    Permet de définir le titre de la la fenêtre d'affichage
*****/

void Disp_SetViewTitle(struct Display *disp, const char *Title)
{
    struct Window *win=disp->ViewInfo.WindowBase;

    /* Je préfère éliminer l'ancien titre des fois que le système serait capable
       d'utiliser le buffer de titre pendant qu'on est en train de le changer.
    */
    if(win!=NULL) TVNC_SetWindowTitles(win,NULL,NULL);

    /* Affichage du nouveau titre */
    Sys_StrCopy(disp->ViewTitle,Title,sizeof(disp->ViewTitle));
    if(win!=NULL) TVNC_SetWindowTitles(win,disp->ViewTitle,disp->ViewTitle);
}


/*****
    Retourne les coordonnées et la taille intérieure de la fenêtre à ouvrir
*****/

void Disp_GetInitialWindowPosition(struct Display *disp, LONG *Left, LONG *Top, LONG *InnerWidth, LONG *InnerHeight)
{
    struct ViewInfo *vi=&disp->ViewInfo;
    struct FrameBuffer *fb=&disp->FrameBuffer;
    LONG ScrWidth,ScrHeight;
    LONG WBorLeft,WBorTop,WBorRight,WBorBottom;
    LONG MinWidth,MinHeight;

    /* Récupération des marges de la fenêtre qui doit s'ouvrir dans l'écran public */
    Disp_GetScreenParam(NULL,&ScrWidth,&ScrHeight,&WBorLeft,&WBorTop,&WBorRight,&WBorBottom);

    /* Récupération des dimensions minimales de la vue */
    Disp_GetViewMinWH(disp,&MinWidth,&MinHeight);

    /* Calcule la taille intérieure autorisée de la fenêtre */
    *InnerWidth=disp->InitialWinViewWidth;
    *InnerHeight=disp->InitialWinViewHeight;
    if(!vi->IsScaled)
    {
        if(*InnerWidth>fb->Width) *InnerWidth=fb->Width;
        if(*InnerHeight>fb->Height) *InnerHeight=fb->Height;
    }
    if(*InnerWidth<0) *InnerWidth=fb->Width;
    if(*InnerHeight<0) *InnerHeight=fb->Height;
    if(*InnerWidth<MinWidth) *InnerWidth=MinWidth;
    if(*InnerHeight<MinHeight) *InnerHeight=MinHeight;

    /* Calcule la position de la fenêtre */
    *Left=disp->InitialWinLeft;
    *Top=disp->InitialWinTop;
    if(*Left<0) *Left=(ScrWidth-(*InnerWidth+WBorLeft+WBorRight))/2;
    if(*Top<0) *Top=(ScrHeight-(*InnerHeight+WBorTop+WBorBottom))/2;
    if(*Left<0) *Left=0;
    if(*Top<0) *Top=0;
}


/*****
    Cette fonction redéfinit les limites de la fenêtre en fonction des
    paramètres courants d'affichage.
*****/

void Disp_SetDisplayWindowLimits(struct Display *disp)
{
    struct ViewInfo *vi=&disp->ViewInfo;
    struct Window *win=vi->WindowBase;
    LONG MinWidth,MinHeight;
    LONG MaxWidth=-1,MaxHeight=-1;

    if(!vi->IsScaled)
    {
        /* Calcule des proportions max que peut prendre la fenêtre */
        MaxWidth=disp->FrameBuffer.Width+vi->BorderLeft+vi->BorderRight;
        MaxHeight=disp->FrameBuffer.Height+vi->BorderTop+vi->BorderBottom;
    }

    Disp_GetViewMinWH(disp,&MinWidth,&MinHeight);

    /* Blindage pour l'overlay */
    if(vi->ViewType==VIEW_TYPE_OVERLAY)
    {
        if(MinWidth<=0) MinWidth=1;
        if(MinHeight<=0) MinHeight=1;
    }
    MinWidth+=vi->BorderLeft+vi->BorderRight;
    MinHeight+=vi->BorderTop+vi->BorderBottom;

    /* On set les limites... */
    WindowLimits(win,MinWidth,MinHeight,MaxWidth,MaxHeight);
}


/*****
    Mise à jour des ascenseurs de la fenêtre en fonction de la Vue.
*****/

void Disp_UpdateWindowFrame(struct Display *disp, BOOL RefreshVertical, BOOL RefreshHorizontal)
{
    struct ViewInfo *vi=&disp->ViewInfo;
    struct Window *win=vi->WindowBase;

    if(win!=NULL && disp->FrameG!=NULL)
    {
        struct Gadget **gad=disp->FrameG->Gadget;
        LONG x=vi->ViewX,totalw=disp->FrameBuffer.Width,w=vi->ViewWidth;
        LONG y=vi->ViewY,totalh=disp->FrameBuffer.Height,h=vi->ViewHeight;

        if(vi->IsScaled)
        {
            x=0;
            w=totalw;
            y=0;
            h=totalh;
        }

        SetAttrs(gad[1],
            PGA_Top,x,
            PGA_Total,totalw,
            PGA_Visible,w,
            TAG_DONE);

        SetAttrs(gad[0],
            PGA_Top,y,
            PGA_Total,totalh,
            PGA_Visible,h,
            TAG_DONE);

        if(RefreshVertical) RefreshGList(gad[0],win,NULL,1);
        if(RefreshHorizontal) RefreshGList(gad[1],win,NULL,1);
    }
}


/*****
    Sélection du type de curseur à afficher en fonction de sa position
    dans la vue.
*****/

void Disp_SetAmigaCursor(struct Display *disp, ULONG Type, LONG PosX, LONG PosY, BOOL IsForce)
{
    struct ViewInfo *vi=&disp->ViewInfo;
    struct Window *win=vi->WindowBase;

    if(win!=NULL)
    {
        if((PosX>=vi->BorderLeft && PosX<vi->ViewWidth+vi->BorderLeft &&
            PosY>=vi->BorderTop  && PosY<vi->ViewHeight+vi->BorderTop) || IsForce)
        {
            switch(Type)
            {
                case VIEWCURSOR_VIEWONLY:
                    if(disp->CurrentPointerType!=VIEWCURSOR_VIEWONLY || IsForce)
                    {
                        SetPointer(win,PointerViewOnlyGfx,24,16,0,0);
                    }
                    break;

                case VIEWCURSOR_HIDDEN:
                    if(disp->CurrentPointerType!=VIEWCURSOR_HIDDEN || IsForce)
                    {
                        SetPointer(win,PointerGfx,0,0,0,0);
                    }
                    break;

                case VIEWCURSOR_NORMAL:
                    if(disp->CurrentPointerType!=VIEWCURSOR_NORMAL || IsForce)
                    {
                        ClearPointer(win);
                    }
                    break;
            }

            disp->CurrentPointerType=Type;
        }
        else if(disp->CurrentPointerType!=VIEWCURSOR_NORMAL)
        {
            disp->CurrentPointerType=VIEWCURSOR_NORMAL;
            ClearPointer(win);
        }
    }
}


/*****************************************************************************/
/*****************************************************************************/
/*****                                                                   *****/
/*****  SOUS-ROUTINES D'OUVERTURE/FERMETURE DE L'AFFICHAGE               *****/
/*****                                                                   *****/
/*****************************************************************************/
/*****************************************************************************/

/*****
    Recherche des proportions définies par un ModeID
*****/

BOOL Disp_GetDimensionFromModeID(ULONG ModeID, LONG *Width, LONG *Height)
{
    BOOL IsSuccess=FALSE;
    struct DimensionInfo di;

    /* Récupération de la résolution Nominale du ModeID */
    if(GetDisplayInfoData(NULL,(UBYTE *)&di,sizeof(di),DTAG_DIMS,ModeID)>0)
    {
        *Width=(LONG)(di.Nominal.MaxX-di.Nominal.MinX)+1;
        *Height=(LONG)(di.Nominal.MaxY-di.Nominal.MinY)+1;
        IsSuccess=TRUE;
    }

    return IsSuccess;
}


/*****
    Vérification du format de la bitmap
*****/

LONG Disp_GetScreenType(struct Screen *ScreenBase)
{
    LONG Result=SCRTYPE_GRAPHICS;
    struct BitMap *bm=ScreenBase->RastPort.BitMap;

    /* Test si on est sur un écran AGA */
    if(!(GetBitMapAttr(bm,BMA_FLAGS)&BMF_STANDARD))
    {
        if(CyberGfxBase!=NULL)
        {
            /* On test si notre écran est compatible cybergraphics */
            if(GetCyberMapAttr(bm,CYBRMATTR_ISCYBERGFX))
            {
                /* On test si on est en chunky 8 bits */
                if(GetCyberMapAttr(bm,CYBRMATTR_PIXFMT)!=PIXFMT_LUT8) Result=SCRTYPE_CYBERGRAPHICS;
            }
        }
    }

    return Result;
}


/*****
    Permet de trouver un nom d'écran non prit.
*****/

void Disp_InitPubScreenName(struct Display *disp)
{
    struct List *Ptr=LockPubScreenList();
    LONG Number=2;
    BOOL IsExists=TRUE;

    Sys_StrCopy(disp->PubScrName,disp->AppTitle,sizeof(disp->PubScrName));
    while(IsExists)
    {
        struct PubScreenNode *NodePtr=(struct PubScreenNode *)Ptr->lh_Head;

        /* On scan les écrans pour savoir si le nom courant existe */
        IsExists=FALSE;
        while(!IsExists && NodePtr!=NULL)
        {
            struct Screen *Scr=NodePtr->psn_Screen;

            if(Scr!=NULL)
            {
                char *Title=(char *)Scr->Title;

                if(Title!=NULL)
                {
                    LONG i;

                    for(i=0;!IsExists;i++)
                    {
                        char Char1=disp->PubScrName[i];
                        char Char2=Title[i];

                        if(Char1!=Char2) break;
                        else if(Char1==0 || Char2==0) IsExists=TRUE;
                    }
                }
            }

            NodePtr=(struct PubScreenNode *)NodePtr->psn_Node.ln_Succ;
        }

        if(IsExists)
        {
            /* On génère un nom dérivé du nom "TwinVNC" */
            Sys_SPrintf(disp->PubScrName,"%s [%ld]",disp->AppTitle,Number++);
        }
    }


    UnlockPubScreenList();
}


/*****
    Permet de specifier les flags IDCMP correspondant à l'OS
*****/

void Disp_SetIDCMPFlags(struct Window *win, ULONG Flags)
{
#if !defined(__amigaos4__) && !defined(__MORPHOS__)
    if(OperatingSystemID==OSID_AMIGAOS4) Flags|=IDCMP_EXTENDEDMOUSE;
    ModifyIDCMP(win,Flags);
#endif
#ifdef __MORPHOS__
    ModifyIDCMP(win,Flags);
#endif
#ifdef __amigaos4__
    ModifyIDCMP(win,Flags|IDCMP_EXTENDEDMOUSE);
#endif
}


/*****
    Ouverture d'une zone d'affichage écran.
*****/

BOOL Disp_OpenDisplayCustomScreen(struct Display *disp, BOOL IsScaled, BOOL IsSmoothed, LONG OverlayThreshold, ULONG *ErrorCode)
{
    BOOL IsSuccess=FALSE;
    struct FrameBuffer *fb=&disp->FrameBuffer;
    LONG ViewWidth=fb->Width,ViewHeight=fb->Height;

    *ErrorCode=0;

    if(!IsScaled || !Disp_GetDimensionFromModeID(disp->ModeID,&ViewWidth,&ViewHeight))
    {
        ViewWidth=fb->Width;
        ViewHeight=fb->Height;
    }

    if((disp->ScreenBase=OpenScreenTags(NULL,
        SA_Left,0,
        SA_Top,0,
        SA_Width,(ULONG)ViewWidth,
        SA_Height,(ULONG)ViewHeight,
        SA_Depth,(ULONG)disp->ScrDepth,
        SA_Interleaved,TRUE,
        SA_FullPalette,TRUE,
        SA_Overscan,OSCAN_TEXT,
        SA_Type,CUSTOMSCREEN,
        SA_DisplayID,disp->ModeID,
        SA_Title,disp->AppTitle,
        SA_ShowTitle,FALSE,
        SA_AutoScroll,TRUE,
        SA_Quiet,TRUE,
        SA_ErrorCode,ErrorCode,
        TAG_DONE))!=NULL)
    {
        struct Window *win;

        /* Ouverture de la fenêtre */
        if((win=OpenWindowTags(NULL,
            WA_CustomScreen,disp->ScreenBase,
            WA_RMBTrap,TRUE,
            WA_SimpleRefresh,TRUE,
            WA_BackFill,LAYERS_NOBACKFILL,
            WA_Flags,WFLG_BACKDROP|WFLG_BORDERLESS|WFLG_REPORTMOUSE|WFLG_ACTIVATE,
            WA_IDCMP,0,
            TAG_DONE))!=NULL)
        {
            LONG ScreenType=Disp_GetScreenType(win->WScreen);

            /* Calcule les paramètres d'affichage en fonction des paramètres de la fenêtre */
            Vinf_ResetViewProperties(&disp->ViewInfo,win,fb,ScreenType,DISP_TYPE_CUSTOM_SCREEN,0,IsScaled,IsSmoothed,TRUE,OverlayThreshold);

            /* Allocation des buffers ou de l'overlay et mise à jour des couleurs si nécessaire */
            IsSuccess=Disp_RethinkViewType(disp,TRUE,FALSE,TRUE);
            if(IsSuccess)
            {
                /* Définition du port message de la fenêtre */
                win->UserPort=disp->MPort;
                Disp_SetIDCMPFlags(win,IDCMP_RAWKEY|IDCMP_MOUSEMOVE|IDCMP_MOUSEBUTTONS);
            }
        }
    }

    return IsSuccess;
}


/*****
    Ouverture d'une zone d'affichage écran public.
*****/

BOOL Disp_OpenDisplayPublicScreen(struct Display *disp, BOOL IsScaled, BOOL IsSmoothed, LONG OverlayThreshold, BOOL IsToolBar, BOOL IsScreenBar, ULONG *ErrorCode)
{
    BOOL IsSuccess=FALSE;
    struct FrameBuffer *fb=&disp->FrameBuffer;
    LONG MargeTop=IsToolBar?TBar_GetToolBarHeight():0;
    LONG BarHeight=0;
    LONG ScreenWidth=fb->Width;
    LONG ScreenHeight=fb->Height;

    *ErrorCode=0;

    if(!IsScaled || !Disp_GetDimensionFromModeID(disp->ModeID,&ScreenWidth,&ScreenHeight))
    {
        ScreenWidth=fb->Width;
        ScreenHeight=fb->Height;
    }

    if(IsScreenBar) Disp_GetScreenParam(&BarHeight,NULL,NULL,NULL,NULL,NULL,NULL);

    Disp_InitPubScreenName(disp);
    if((disp->ScreenBase=OpenScreenTags(NULL,
        SA_Left,0,
        SA_Top,0,
        SA_Width,(ULONG)ScreenWidth,
        SA_Height,(ULONG)(ScreenHeight+BarHeight+MargeTop),
        SA_Depth,(ULONG)disp->ScrDepth,
        SA_Interleaved,TRUE,
        SA_Overscan,OSCAN_TEXT,
        SA_DisplayID,disp->ModeID,
        SA_Type,PUBLICSCREEN,
        SA_AutoScroll,TRUE,
        SA_SharePens,TRUE,
        SA_LikeWorkbench,TRUE,
        SA_PubName,disp->PubScrName,
        SA_Title,disp->AppTitle,
        SA_ShowTitle,(ULONG)(IsScreenBar?TRUE:FALSE),
        SA_Quiet,(ULONG)(IsScreenBar?FALSE:TRUE),
        SA_ErrorCode,ErrorCode,
        TAG_DONE))!=NULL)
    {
        struct Window *win;
        LONG ViewWidth=ScreenWidth;
        LONG ViewHeight=ScreenHeight;

        if(IsScaled && disp->IsInitialFullScreenProportional)
        {
            Disp_ConvertInnerSizeToProportionalSize(disp,disp->ScreenBase,&ViewWidth,&ViewHeight);
        }

        /* Truc de l'AmigaOS pour rendre l'écran vraiment public */
        PubScreenStatus(disp->ScreenBase,~PSNF_PRIVATE);

        /* Ouverture de la fenêtre */
        if((win=OpenWindowTags(NULL,
            WA_Left,0,
            WA_Top,(ULONG)BarHeight,
            WA_InnerWidth,(ULONG)ViewWidth,
            WA_InnerHeight,(ULONG)(ViewHeight+MargeTop),
            WA_PubScreen,disp->ScreenBase,
            WA_RMBTrap,TRUE,
            WA_BackFill,LAYERS_NOBACKFILL,
            WA_Flags,WFLG_BACKDROP|WFLG_BORDERLESS|WFLG_REPORTMOUSE|WFLG_ACTIVATE|WFLG_SMART_REFRESH,
            WA_IDCMP,0,
            TAG_DONE))!=NULL)
        {
            if((disp->VisualI=GetVisualInfoA(win->WScreen,NULL))!=NULL)
            {
                /* Détermination du type d'écran pour le rendering: GRAPHICS ou CYBERGRAPHICS */
                LONG ScreenType=Disp_GetScreenType(win->WScreen);

                /* Calcule les paramètres d'affichage en fonction des paramètres de la fenêtre */
                Vinf_ResetViewProperties(&disp->ViewInfo,win,fb,ScreenType,DISP_TYPE_PUBLIC_SCREEN,MargeTop,IsScaled,IsSmoothed,TRUE,OverlayThreshold);

                /* Allocation des gadgets de la toolbar et de leurs couleurs sur un écran 8 bits */
                IsSuccess=TBar_CreateToolBar(disp);

                if(IsSuccess)
                {
                    if(IsToolBar) TBar_ShowToolBar(disp);

                    /* Allocation des buffers ou de l'overlay et mise à jour des couleurs si nécessaire */
                    IsSuccess=Disp_RethinkViewType(disp,TRUE,FALSE,TRUE);
                    if(IsSuccess)
                    {
                        /* Définition du port message de la fenêtre */
                        win->UserPort=disp->MPort;
                        Disp_SetIDCMPFlags(win,
                            IDCMP_GADGETUP|IDCMP_GADGETDOWN|IDCMP_IDCMPUPDATE|
                            IDCMP_RAWKEY|IDCMP_GADGETHELP|IDCMP_MOUSEMOVE|IDCMP_MOUSEBUTTONS|
                            IDCMP_CLOSEWINDOW|IDCMP_NEWSIZE);
                    }
                }
            }
        }
    }

    return IsSuccess;
}


/*****
    Ouverture d'une zone d'affichage fenêtre.
*****/

BOOL Disp_OpenDisplayWindow(struct Display *disp, BOOL IsScaled, BOOL IsSmoothed, LONG OverlayThreshold, BOOL IsToolBar)
{
    BOOL IsSuccess=FALSE;
    struct Screen *Scr;

    if((Scr=LockPubScreen(NULL))!=NULL)
    {
        struct Window *win;
        LONG Left,Top,InnerWidth,InnerHeight;
        LONG MargeTop=IsToolBar?TBar_GetToolBarHeight():0;

        disp->AltPos[0]=~0;
        disp->AltPos[1]=~0;
        disp->AltPos[2]=0;
        disp->AltPos[3]=0;

        /* Ouverture de la fenêtre */
        Disp_GetInitialWindowPosition(disp,&Left,&Top,&InnerWidth,&InnerHeight);
        win=OpenWindowTags(NULL,
            WA_Left,Left,
            WA_Top,Top,
            WA_InnerWidth,InnerWidth,
            WA_InnerHeight,InnerHeight+MargeTop,
            WA_PubScreen,Scr,
            WA_RMBTrap,TRUE,
            WA_Zoom,disp->AltPos,
            WA_BackFill,LAYERS_NOBACKFILL,
            WA_Flags,WFLG_SIZEGADGET|WFLG_SIZEBRIGHT|WFLG_SIZEBBOTTOM|WFLG_SMART_REFRESH|WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_REPORTMOUSE|WFLG_ACTIVATE,
            WA_IDCMP,0,
            TAG_DONE);

        /* Selon les autodocs, il faut libérer l'écran une fois que la fenêtre
           est attachée dessus.
        */
        UnlockPubScreen(NULL,Scr);

        if(win!=NULL)
        {
            if((disp->DrawI=GetScreenDrawInfo(win->WScreen))!=NULL)
            {
                /* Création et attachement des gadgets intégrés à la fenêtre */
                if((disp->FrameG=Gui_CreateFrameGadgets(win,disp->DrawI,100))!=NULL)
                {
                    if((disp->VisualI=GetVisualInfoA(win->WScreen,NULL))!=NULL)
                    {
                        struct ViewInfo *vi=&disp->ViewInfo;

                        /* Détermination du type d'écran pour le rendering: GRAPHICS ou CYBERGRAPHICS */
                        LONG ScreenType=Disp_GetScreenType(win->WScreen);

                        /* Calcule les paramètres d'affichage en fonction des paramètres de la fenêtre */
                        Vinf_ResetViewProperties(vi,win,&disp->FrameBuffer,ScreenType,DISP_TYPE_WINDOW,MargeTop,IsScaled,IsSmoothed,FALSE,OverlayThreshold);

                        /* Allocation des gadgets de la toolbar et de leurs couleurs pour un écran 8 bits */
                        IsSuccess=TBar_CreateToolBar(disp);
                        if(IsSuccess)
                        {
                            if(IsToolBar) TBar_ShowToolBar(disp);

                            /* Allocation des buffers ou de l'overlay et mise à jour des couleurs si nécessaire */
                            IsSuccess=Disp_RethinkViewType(disp,TRUE,FALSE,TRUE);
                            if(IsSuccess)
                            {
                                /* Définition du port message de la fenêtre */
                                win->UserPort=disp->MPort;
                                Disp_SetIDCMPFlags(win,
                                    IDCMP_GADGETUP|IDCMP_GADGETDOWN|IDCMP_IDCMPUPDATE|
                                    IDCMP_NEWSIZE|IDCMP_RAWKEY|IDCMP_GADGETHELP|
                                    IDCMP_MOUSEMOVE|IDCMP_MOUSEBUTTONS|IDCMP_CLOSEWINDOW|
                                    IDCMP_SIZEVERIFY);

                                Disp_SetDisplayWindowLimits(disp);

                                /* On force le retaillage de la fenêtre au cas où le frame buffer change
                                   pendant l'iconification, par exemple.
                                */
                                IsSuccess=Disp_ResizeView(disp,vi->ViewWidth,vi->ViewHeight,FALSE);
                            }
                        }
                    }
                }
            }
        }
    }

    return IsSuccess;
}


/*****
    Cette fonction permet d'obtenir différents paramètres de l'écran Workbench
    avant d'ouvrir son propre écran ou sa propre fenêtre.
*****/

void Disp_GetScreenParam(LONG *ScreenBarHeight, LONG *ScrWidth, LONG *ScrHeight, LONG *WBorLeft, LONG *WBorTop, LONG *WBorRight, LONG *WBorBottom)
{
    struct Screen *WBScr=LockPubScreen(NULL);

    /* Paramètres par défaut */
    if(ScreenBarHeight!=NULL) *ScreenBarHeight=11;
    if(ScrWidth!=NULL) *ScrWidth=0;
    if(ScrHeight!=NULL) *ScrHeight=0;
    if(WBorLeft!=NULL) *WBorLeft=0;
    if(WBorTop!=NULL) *WBorTop=0;
    if(WBorRight!=NULL) *WBorRight=0;
    if(WBorBottom!=NULL) *WBorBottom=0;

    /* Récupération des paramètres de l'écran Workbench */
    if(WBScr!=NULL)
    {
        if(ScreenBarHeight!=NULL) *ScreenBarHeight=WBScr->BarHeight+1;
        if(ScrWidth!=NULL) *ScrWidth=(LONG)WBScr->Width;
        if(ScrHeight!=NULL) *ScrHeight=(LONG)WBScr->Height;
        if(WBorLeft!=NULL) *WBorLeft=(LONG)WBScr->WBorLeft;
        if(WBorTop!=NULL) *WBorTop=(LONG)WBScr->WBorTop+(LONG)WBScr->Font->ta_YSize+1;
        if(WBorRight!=NULL) *WBorRight=(LONG)WBScr->WBorRight;
        if(WBorBottom!=NULL) *WBorBottom=(LONG)WBScr->WBorBottom;

        UnlockPubScreen(NULL,WBScr);
    }
}


/*****************************************************************************/
/*****************************************************************************/
/*****                                                                   *****/
/*****  GESTION DES MESSAGES INTUITION                                   *****/
/*****                                                                   *****/
/*****************************************************************************/
/*****************************************************************************/

/*****
    Gestions des messages de la fenêtre de visualisation
*****/

BOOL Disp_ManageWindowDisplayMessages(struct Display *disp, struct IntuiMessage *msg)
{
    BOOL IsSuccess=TRUE;
    struct ViewInfo *vi=&disp->ViewInfo;
    struct FrameBuffer *fb=&disp->FrameBuffer;
    struct Window *win=vi->WindowBase;

    switch(msg->Class)
    {
        case IDCMP_NEWSIZE:
            Vinf_ResetViewProperties(vi,win,fb,vi->ScreenType,vi->DisplayType,disp->IsToolBar?TBar_GetToolBarHeight():0,vi->IsScaled,vi->IsSmoothed,vi->IsFullScreen,vi->OverlayThreshold);
            if(!disp->IsOnResize && msg->Qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RCOMMAND))
            {
                IsSuccess=Disp_RethinkViewType(disp,FALSE,FALSE,FALSE);

                /* Retaillage proportionnel de la vue */
                if(IsSuccess) IsSuccess=Disp_ResizeWindow(disp,(LONG)win->Width,(LONG)win->Height,TRUE);
            }
            else
            {
                disp->IsOnResize=FALSE;
                Disp_RefreshWindowObjects(disp);
                IsSuccess=Disp_RethinkViewType(disp,FALSE,TRUE,FALSE);
            }
            if(IsSuccess) Disp_CheckWindowPositionOver(disp);
            break;

        case IDCMP_SIZEVERIFY:
            Disp_SetDisplayWindowLimits(disp);
            break;

        case IDCMP_IDCMPUPDATE:
            if(!vi->IsScaled)
            {
                ULONG GadID=GetTagData(GA_ID,0,msg->IAddress);
                LONG TopX=vi->ViewX,TopY=vi->ViewY;
                LONG Width=fb->Width,Height=fb->Height;

                switch(GadID)
                {
                    case 105:
                        if(TopX+vi->ViewWidth<Width-1)
                        {
                            TopX+=10;
                            if(TopX+vi->ViewWidth>=Width) TopX=Width-vi->ViewWidth;
                            Disp_MoveViewPosition(disp,TopX,TopY);
                            Disp_UpdateWindowFrame(disp,FALSE,TRUE);
                        }
                        break;

                    case 104:
                        if(TopX>0)
                        {
                            TopX-=10;
                            if(TopX<0) TopX=0;
                            Disp_MoveViewPosition(disp,TopX,TopY);
                            Disp_UpdateWindowFrame(disp,FALSE,TRUE);
                        }
                        break;

                    case 103:
                        if(TopY+vi->ViewHeight<Height-1)
                        {
                            TopY+=10;
                            if(TopY+vi->ViewHeight>=Height) TopY=Height-vi->ViewHeight;
                            Disp_MoveViewPosition(disp,TopX,TopY);
                            Disp_UpdateWindowFrame(disp,TRUE,FALSE);
                        }
                        break;

                    case 102:
                        if(TopY>0)
                        {
                            TopY-=10;
                            if(TopY<0) TopY=0;
                            Disp_MoveViewPosition(disp,TopX,TopY);
                            Disp_UpdateWindowFrame(disp,TRUE,FALSE);
                        }
                        break;

                    default:
                        GetAttr(PGA_Top,disp->FrameG->Gadget[1],(ULONG *)&TopX);
                        GetAttr(PGA_Top,disp->FrameG->Gadget[0],(ULONG *)&TopY);
                        Disp_MoveViewPosition(disp,TopX,TopY);
                        break;
                }
            }
            break;

        case IDCMP_GADGETHELP:
            {
                struct Gadget *GadgetPtr=(struct Gadget *)msg->IAddress;

                if(GadgetPtr!=NULL && GadgetPtr->UserData!=NULL)
                {
                    if(vi->DisplayType==DISP_TYPE_WINDOW) TVNC_SetWindowTitles(win,GadgetPtr->UserData,NULL);
                    else TVNC_SetWindowTitles(win,NULL,GadgetPtr->UserData);
                } else TVNC_SetWindowTitles(win,disp->ViewTitle,disp->ViewTitle);
            }
            break;
    }

    return IsSuccess;
}
