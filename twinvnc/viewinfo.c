#include "system.h"
#include "gfxinfo.h"
#include "viewinfo.h"
#include "toolbar.h"


/*
    31-12-2015 (Seg)    Nouvelle couche
*/


#define OVER_MARGE  8   /* pour autoriser le déplacement du curseur un peu en dehors de la vue */
#define MAX_COLOR   256


/***** Prototypes */
void Vinf_InitView(struct ViewInfo *, struct FrameBuffer *);
void Vinf_ResetViewProperties(struct ViewInfo *, struct Window *, struct FrameBuffer *, LONG, LONG, LONG, BOOL, BOOL, BOOL, LONG);
void Vinf_MoveView(struct ViewInfo *, LONG, LONG);
void Vinf_RethinkViewType(struct ViewInfo *);
BOOL VInf_PrepareColorMap(struct ViewInfo *);
void VInf_FlushColorMap(struct ViewInfo *);
BOOL VInf_GetRealMouseXY(struct ViewInfo *, LONG, LONG, LONG *, LONG *);


/*****
    Initialisation de la structure
*****/

void Vinf_InitView(struct ViewInfo *vi, struct FrameBuffer *fb)
{
    LONG i;

    /* Initialisation de la table d'index des palettes */
    for(i=0;i<MAX_COLOR;i++) vi->ColorMapID[i]=-1;

    vi->FrameBufferPtr=fb;
    vi->NormalViewX=0;
    vi->NormalViewY=0;
    Vinf_ResetViewProperties(vi,NULL,fb,SCRTYPE_GRAPHICS,DISP_TYPE_NONE,FALSE,FALSE,FALSE,FALSE,0);
}


/*****
    Recalcule les marges de l'affichage et les dimensions de la vue en
    fonction des propriétés de la fenêtre.
*****/

void Vinf_ResetViewProperties(struct ViewInfo *vi, struct Window *win, struct FrameBuffer *fb, LONG ScreenType, LONG DisplayType, LONG MargeTop, BOOL IsScaled, BOOL IsSmoothed, BOOL IsFullScreen, LONG OverlayThreshold)
{
    /* On désactive l'affichage dans la vue pour éviter des refreshs indésirables
       après le recalcul des propriétés de la vue.
    */
    vi->ViewType=VIEW_TYPE_NONE;

    /* Autres inits */
    vi->WindowBase=win;
    vi->ScreenType=ScreenType;
    vi->DisplayType=DisplayType;
    vi->IsScaled=IsScaled;
    vi->IsSmoothed=IsSmoothed;
    vi->IsFullScreen=IsFullScreen;
    vi->OverlayThreshold=OverlayThreshold;
    vi->FrameBufferPtr=fb;

    if(win!=NULL)
    {
        LONG Width=fb->Width,Height=fb->Height;

        /* Recalcul des marges de la zone d'affichage (toolbar comprise) */
        vi->BorderLeft=0;
        vi->BorderTop=MargeTop;
        vi->BorderRight=0;
        vi->BorderBottom=0;
        vi->BorderLeft+=(LONG)win->BorderLeft;
        vi->BorderTop+=(LONG)win->BorderTop;
        vi->BorderRight+=(LONG)win->BorderRight;
        vi->BorderBottom+=(LONG)win->BorderBottom;

        /* Recalcul de la taille de la vue */
        vi->ViewWidth=(LONG)win->Width-(vi->BorderLeft+vi->BorderRight);
        vi->ViewHeight=(LONG)win->Height-(vi->BorderTop+vi->BorderBottom);
        if(!IsScaled)
        {
            if(vi->ViewWidth>Width) vi->ViewWidth=Width;
            if(vi->ViewHeight>Height) vi->ViewHeight=Height;
        }

        /* Recalcul de la position de la vue */
        if(IsScaled || IsFullScreen)
        {
            vi->ViewX=0;
            vi->ViewY=0;
        }
        else
        {
            vi->ViewX=vi->NormalViewX;
            vi->ViewY=vi->NormalViewY;
            if(vi->ViewX+vi->ViewWidth>Width) vi->ViewX=Width-vi->ViewWidth;
            if(vi->ViewY+vi->ViewHeight>Height) vi->ViewY=Height-vi->ViewHeight;
        }
    }
}


/*****
    Pour changer les coordonnées de la vue (en mode normal uniquement)
*****/

void Vinf_MoveView(struct ViewInfo *vi, LONG TopX, LONG TopY)
{
    vi->ViewX=TopX;
    vi->ViewY=TopY;
    vi->NormalViewX=TopX;
    vi->NormalViewY=TopY;
}


/*****
    Recalcul le type de vue à utiliser en fonction des paramètres qui ont
    été demandés par l'utilisateur auparavant.
*****/

void Vinf_RethinkViewType(struct ViewInfo *vi)
{
    vi->ViewType=VIEW_TYPE_NONE;

    if(vi->WindowBase!=NULL)
    {
        /* Définition du type de la vue */
        if(vi->DisplayType!=DISP_TYPE_NONE && vi->ViewWidth>0 && vi->ViewHeight>0)
        {
            LONG Width=vi->FrameBufferPtr->Width;
            LONG Height=vi->FrameBufferPtr->Height;

            vi->ViewType=VIEW_TYPE_NORMAL;
            if(vi->IsScaled && (vi->ViewWidth!=Width || vi->ViewHeight!=Height))
            {
                vi->ViewType=VIEW_TYPE_SCALE;
                if(vi->OverlayThreshold>=0)
                {
                    LONG ThresholdWidth=vi->ViewWidth*100/vi->OverlayThreshold;
                    LONG ThresholdHeight=vi->ViewHeight*100/vi->OverlayThreshold;

                    if(vi->OverlayThreshold<100)
                    {
                        if(ThresholdWidth>Width && ThresholdHeight>Height) vi->ViewType=VIEW_TYPE_OVERLAY;
                    }
                    else
                    {
                        if(ThresholdWidth>=Width && ThresholdHeight>=Height) vi->ViewType=VIEW_TYPE_OVERLAY;
                    }
                }
            }
        }

        if(vi->ViewType==VIEW_TYPE_SCALE && vi->IsSmoothed)
        {
            vi->ViewType=VIEW_TYPE_SMOOTH;
        }
    }
}


/*****
    Allocation des palettes nécessaires.
    Cette fonction doit être utilisée dans le cas d'un écran PUBLIC ou d'une
    fenêtre.
*****/

BOOL VInf_PrepareColorMap(struct ViewInfo *vi)
{
    BOOL IsSuccess=TRUE;
    struct FrameBuffer *fb=vi->FrameBufferPtr;
    struct ColorMap *cm=vi->WindowBase->WScreen->ViewPort.ColorMap;
    LONG i;

    for(i=0;i<fb->ColorCount && i<MAX_COLOR;i++)
    {
        LONG RGB=fb->ColorMapRGB32[i];
        LONG R32=((RGB>>16)&0xff)*0x01010101;
        LONG G32=((RGB>>8)&0xff)*0x01010101;
        LONG B32=(RGB&0xff)*0x01010101;

        vi->ColorMapID[i]=ObtainBestPen(cm,R32,G32,B32,OBP_Precision,PRECISION_EXACT,TAG_DONE);
        if(vi->ColorMapID[i]<0) IsSuccess=FALSE;
    }

    if(!IsSuccess) VInf_FlushColorMap(vi);

    return IsSuccess;
}


/*****
    Libération des palettes allouées par VInf_PrepareColorMap()
*****/

void VInf_FlushColorMap(struct ViewInfo *vi)
{
    struct ColorMap *cm=vi->WindowBase->WScreen->ViewPort.ColorMap;
    LONG i;

    for(i=0;i<MAX_COLOR;i++)
    {
        if(vi->ColorMapID[i]>=0) ReleasePen(cm,vi->ColorMapID[i]);
        vi->ColorMapID[i]=-1;
    }
}


/*****
    Conversion des coordonnées relatives à la fenêtre
*****/

BOOL VInf_GetRealMouseXY(struct ViewInfo *vi, LONG WinX, LONG WinY, LONG *FrameBufX, LONG *FrameBufY)
{
    struct FrameBuffer *fb=vi->FrameBufferPtr;

    if(vi->IsScaled)
    {
        *FrameBufX=0;
        *FrameBufY=0;
        if(vi->ViewWidth) *FrameBufX=((WinX-vi->BorderLeft)*fb->Width)/vi->ViewWidth;
        if(vi->ViewHeight) *FrameBufY=((WinY-vi->BorderTop)*fb->Height)/vi->ViewHeight;
    }
    else
    {
        *FrameBufX=WinX-vi->BorderLeft+vi->ViewX;
        *FrameBufY=WinY-vi->BorderTop+vi->ViewY;
    }

    if(*FrameBufX<0 || *FrameBufX>=fb->Width ||
       *FrameBufY<0 || *FrameBufY>=fb->Height)
    {
        if(*FrameBufX<0) *FrameBufX=0;
        if(*FrameBufY<0) *FrameBufY=0;
        if(*FrameBufX>=fb->Width) *FrameBufX=fb->Width-1;
        if(*FrameBufY>=fb->Height) *FrameBufY=fb->Height-1;
    }

    if(WinX<(vi->BorderLeft-OVER_MARGE) || WinX>=(vi->ViewWidth+vi->BorderLeft+OVER_MARGE) ||
       WinY<(vi->BorderTop-OVER_MARGE) || WinY>=(vi->ViewHeight+vi->BorderTop+OVER_MARGE)) return FALSE;

    return TRUE;
}
