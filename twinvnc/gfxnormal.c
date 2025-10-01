#include "system.h"
#include "gfxnormal.h"


/*
    31-12-2015 (Seg)    Refonte du code
    18-12-2005 (Seg)    Modification pour parer un bug de WriteChunkyPixel(). Les
                        blits sont maintenant alignés sur 32 octets.
    16-11-2005 (Seg)    Restructuration de l'affichage
    20-12-2004 (Seg)    Gestion de l'affichage normal du projet
*/


#define ALIGN_NORMAL    32


/***** Prototypes */
BOOL Gfxn_Create(struct GfxNormal *, struct ViewInfo *);
void Gfxn_Free(struct GfxNormal *);
void Gfxn_Reset(struct GfxNormal *);

void Gfxn_UpdateParam(struct GfxNormal *);

void Gfxn_GraphicsFillRect8(struct GfxNormal *, LONG, LONG, LONG, LONG, ULONG);
void Gfxn_GraphicsFillRect16(struct GfxNormal *, LONG, LONG, LONG, LONG, ULONG);
void Gfxn_GraphicsFillRect24(struct GfxNormal *, LONG, LONG, LONG, LONG, ULONG);
void Gfxn_CybergraphicsFillRect8(struct GfxNormal *, LONG, LONG, LONG, LONG, ULONG);
void Gfxn_CybergraphicsFillRect16(struct GfxNormal *, LONG, LONG, LONG, LONG, ULONG);
void Gfxn_CybergraphicsFillRect24(struct GfxNormal *, LONG, LONG, LONG, LONG, ULONG);

void Gfxn_GraphicsRefreshRect8(struct GfxNormal *, LONG, LONG, LONG, LONG);
void Gfxn_GraphicsRefreshRect16(struct GfxNormal *, LONG, LONG, LONG, LONG);
void Gfxn_GraphicsRefreshRect24(struct GfxNormal *, LONG, LONG, LONG, LONG);
void Gfxn_CybergraphicsRefreshRect8(struct GfxNormal *, LONG, LONG, LONG, LONG);
void Gfxn_CybergraphicsRefreshRect16(struct GfxNormal *, LONG, LONG, LONG, LONG);
void Gfxn_CybergraphicsRefreshRect24(struct GfxNormal *, LONG, LONG, LONG, LONG);

LONG Gfxn_GetNormalHeight(struct GfxNormal *, LONG, LONG, LONG);
BOOL Gfxn_ClipToView(struct GfxNormal *, LONG *, LONG *, LONG *, LONG *, BOOL);


/*****
    Allocation des tables nécessaires pour la conversion 16 bits
*****/

BOOL Gfxn_Create(struct GfxNormal *gfx, struct ViewInfo *view)
{
    BOOL IsSuccess=TRUE;

    gfx->GfxI.ViewInfoPtr=view;
    Gfxn_UpdateParam(gfx);

    if(view->ViewWidth>0 && view->ViewHeight>0)
    {
        IsSuccess=FALSE;

        gfx->NormalBufferSize=view->ViewWidth*sizeof(LONG)*32;

        if((gfx->NormalBufferPtr=Sys_AllocMem(gfx->NormalBufferSize))!=NULL)
        {
            IsSuccess=TRUE;
        }

        if(!IsSuccess) Gfxn_Free(gfx);
    }

    return IsSuccess;
}


/*****
    Libération des tables allouées pour la conversion 16 bits
*****/

void Gfxn_Free(struct GfxNormal *gfx)
{
    if(gfx->NormalBufferPtr!=NULL) Sys_FreeMem(gfx->NormalBufferPtr);
    gfx->NormalBufferPtr=NULL;
    gfx->NormalBufferSize=0;
}


/*****
    Définition des fonctions graphiques adaptées à la vue et au type d'écran donné
*****/

void Gfxn_Reset(struct GfxNormal *gfx)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    gfx->GfxI.UpdateParamFuncPtr=(void (*)(void *))Gfxn_UpdateParam;

    switch(vi->ScreenType)
    {
        case SCRTYPE_GRAPHICS:
            switch(vi->FrameBufferPtr->PixelBufferSize)
            {
                case 1:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxn_GraphicsFillRect8;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxn_GraphicsRefreshRect8;
                    break;

                case 2:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxn_GraphicsFillRect16;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxn_GraphicsRefreshRect16;
                    break;

                default:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxn_GraphicsFillRect24;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxn_GraphicsRefreshRect24;
                    break;
            }
            break;

        case SCRTYPE_CYBERGRAPHICS:
            switch(vi->FrameBufferPtr->PixelBufferSize)
            {
                case 1:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxn_CybergraphicsFillRect8;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxn_CybergraphicsRefreshRect8;
                    break;

                case 2:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxn_CybergraphicsFillRect16;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxn_CybergraphicsRefreshRect16;
                    break;

                default:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxn_CybergraphicsFillRect24;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxn_CybergraphicsRefreshRect24;
                    break;
            }
            break;
    }
}


/*****
    Mise à jour des paramètres suite à une modification de la vue
*****/

void Gfxn_UpdateParam(struct GfxNormal *gfx)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
    gfx->OffsetX=vi->BorderLeft-vi->ViewX;
    gfx->OffsetY=vi->BorderTop-vi->ViewY;
    Gfxn_Reset(gfx);
}


/******************************************/
/*****                                *****/
/***** FONCTIONS PRIVEES : FillRect() *****/
/*****                                *****/
/******************************************/

/*****
    Remplissage: Graphics - 8 bits -> 8 bits max
*****/

void Gfxn_GraphicsFillRect8(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=gfx->GfxI.ViewInfoPtr->WindowBase->RPort;
        x+=gfx->OffsetX;
        y+=gfx->OffsetY;
        if(gfx->GfxI.ViewInfoPtr->DisplayType!=DISP_TYPE_CUSTOM_SCREEN) Color=gfx->GfxI.ViewInfoPtr->ColorMapID[Color];
        SetAPen(rp,Color);
        RectFill(rp,x,y,x+w-1,y+h-1);
    }
}


/*****
    Remplissage: Graphics - 16 bits -> 8 bits max
*****/

void Gfxn_GraphicsFillRect16(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=gfx->GfxI.ViewInfoPtr->WindowBase->RPort;
        x+=gfx->OffsetX;
        y+=gfx->OffsetY;
        Color=((Color>>8)&0xe0)+((Color>>6)&0x1c)+((Color>>3)&0x03);
        if(gfx->GfxI.ViewInfoPtr->DisplayType!=DISP_TYPE_CUSTOM_SCREEN) Color=gfx->GfxI.ViewInfoPtr->ColorMapID[Color];
        SetAPen(rp,Color);
        RectFill(rp,x,y,x+w-1,y+h-1);
    }
}


/*****
    Remplissage: Graphics - 24 bits -> 8 bits max
*****/

void Gfxn_GraphicsFillRect24(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=gfx->GfxI.ViewInfoPtr->WindowBase->RPort;
        x+=gfx->OffsetX;
        y+=gfx->OffsetY;
        Color=((Color>>16)&0xe0)+((Color>>11)&0x1c)+((Color>>6)&0x03);
        if(gfx->GfxI.ViewInfoPtr->DisplayType!=DISP_TYPE_CUSTOM_SCREEN) Color=gfx->GfxI.ViewInfoPtr->ColorMapID[Color];
        SetAPen(rp,Color);
        RectFill(rp,x,y,x+w-1,y+h-1);
    }
}


/*****
    Remplissage: Cybergraphics - 8 bits -> 8 bits
*****/

void Gfxn_CybergraphicsFillRect8(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;
        x+=gfx->OffsetX;
        y+=gfx->OffsetY;
        Color=vi->FrameBufferPtr->ColorMapRGB32[Color];
        FillPixelArray(rp,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)h,Color);
    }
}


/*****
    Remplissage: Cybergraphics - 16 bits -> 16 bits
*****/

void Gfxn_CybergraphicsFillRect16(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=gfx->GfxI.ViewInfoPtr->WindowBase->RPort;
        x+=gfx->OffsetX;
        y+=gfx->OffsetY;
        Color=((Color<<8)&0xf80000)+((Color<<5)&0xfc00)+(UBYTE)(Color<<3);
        FillPixelArray(rp,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)h,Color);
    }
}


/*****
    Remplissage: Cybergraphics - 24 bits -> 24 bits
*****/

void Gfxn_CybergraphicsFillRect24(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=gfx->GfxI.ViewInfoPtr->WindowBase->RPort;
        x+=gfx->OffsetX;
        y+=gfx->OffsetY;
        FillPixelArray(rp,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)h,Color);
    }
}


/*********************************************/
/*****                                   *****/
/***** FONCTIONS PRIVEES : RefreshRect() *****/
/*****                                   *****/
/*********************************************/

/*****
    Rafraîchissement d'un bloc: Graphics - 8 bits -> 8 bits max
*****/

void Gfxn_GraphicsRefreshRect8(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,(vi->ScreenType==SCRTYPE_GRAPHICS?TRUE:FALSE)))
    {
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;
        UBYTE *SrcPtr=&((UBYTE *)fb->BufferPtr)[x+y*fb->Width];

        x+=gfx->OffsetX;
        y+=gfx->OffsetY;

        if(gfx->GfxI.ViewInfoPtr->DisplayType==DISP_TYPE_CUSTOM_SCREEN)
        {
            WriteChunkyPixels(rp,x,y,x+w-1,y+h-1,SrcPtr,fb->Width);
        }
        else
        {
            LONG Mod=fb->Width-w;
            LONG ht;

            while((ht=Gfxn_GetNormalHeight(gfx,w,h,sizeof(UBYTE)))>0)
            {
                UBYTE *DstPtr=gfx->NormalBufferPtr;
                LONG j=ht;

                while(--j>=0)
                {
                    LONG i=w;

                    while(--i>=0) *(DstPtr++)=gfx->GfxI.ViewInfoPtr->ColorMapID[*(SrcPtr++)];
                    SrcPtr=&SrcPtr[Mod];
                }

                WriteChunkyPixels(rp,x,y,x+w-1,y+ht-1,gfx->NormalBufferPtr,w);
                y+=ht;
                h-=ht;
            }
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Graphics - 16 bits -> 8 bits max
*****/

void Gfxn_GraphicsRefreshRect16(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,(vi->ScreenType==SCRTYPE_GRAPHICS?TRUE:FALSE)))
    {
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;
        UWORD *SrcPtr=&((UWORD *)fb->BufferPtr)[x+y*fb->Width];
        LONG Mod=fb->Width-w;
        LONG ht;

        x+=gfx->OffsetX;
        y+=gfx->OffsetY;

        while((ht=Gfxn_GetNormalHeight(gfx,w,h,sizeof(UBYTE)))>0)
        {
            UBYTE *DstPtr=gfx->NormalBufferPtr;
            LONG j=ht;

            if(gfx->GfxI.ViewInfoPtr->DisplayType==DISP_TYPE_CUSTOM_SCREEN)
            {
                while(--j>=0)
                {
                    LONG i=w;

                    while(--i>=0)
                    {
                        ULONG Color=*(SrcPtr++);
                        *(DstPtr++)=((Color>>8)&0xe0)+((Color>>6)&0x1c)+((Color>>3)&0x03);
                    }
                    SrcPtr=&SrcPtr[Mod];
                }
            }
            else
            {
                while(--j>=0)
                {
                    LONG i=w;

                    while(--i>=0)
                    {
                        ULONG Color=*(SrcPtr++);
                        Color=((Color>>8)&0xe0)+((Color>>6)&0x1c)+((Color>>3)&0x03);
                        *(DstPtr++)=gfx->GfxI.ViewInfoPtr->ColorMapID[Color];
                    }
                    SrcPtr=&SrcPtr[Mod];
                }
            }

            WriteChunkyPixels(rp,x,y,x+w-1,y+ht-1,gfx->NormalBufferPtr,w);
            y+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Graphics - 24 bits -> 8 bits max
*****/

void Gfxn_GraphicsRefreshRect24(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,(vi->ScreenType==SCRTYPE_GRAPHICS?TRUE:FALSE)))
    {
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;
        ULONG *SrcPtr=&((ULONG *)fb->BufferPtr)[x+y*fb->Width];
        LONG Mod=fb->Width-w;
        LONG ht;

        x+=gfx->OffsetX;
        y+=gfx->OffsetY;

        while((ht=Gfxn_GetNormalHeight(gfx,w,h,sizeof(UBYTE)))>0)
        {
            UBYTE *DstPtr=gfx->NormalBufferPtr;
            LONG j=ht;

            if(gfx->GfxI.ViewInfoPtr->DisplayType==DISP_TYPE_CUSTOM_SCREEN)
            {
                while(--j>=0)
                {
                    LONG i=w;

                    while(--i>=0)
                    {
                        ULONG Color=*(SrcPtr++);
                        *(DstPtr++)=((Color>>16)&0xe0)+((Color>>11)&0x1c)+((Color>>6)&0x03);
                    }
                    SrcPtr=&SrcPtr[Mod];
                }
            }
            else
            {
                while(--j>=0)
                {
                    LONG i=w;

                    while(--i>=0)
                    {
                        ULONG Color=*(SrcPtr++);
                        Color=((Color>>16)&0xe0)+((Color>>11)&0x1c)+((Color>>6)&0x03);
                        *(DstPtr++)=gfx->GfxI.ViewInfoPtr->ColorMapID[Color];
                    }
                    SrcPtr=&SrcPtr[Mod];
                }
            }

            WriteChunkyPixels(rp,x,y,x+w-1,y+ht-1,gfx->NormalBufferPtr,w);
            y+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 8 bits -> 8 bits
*****/

void Gfxn_CybergraphicsRefreshRect8(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;
        UBYTE *SrcPtr=&((UBYTE *)fb->BufferPtr)[x+y*fb->Width];

        x+=gfx->OffsetX;
        y+=gfx->OffsetY;
        WriteLUTPixelArray(SrcPtr,(UWORD)0,(UWORD)0,(UWORD)fb->Width,rp,fb->ColorMapRGB32,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)h,CTABFMT_XRGB8);
    }
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 16 bits -> 16 bits
*****/

void Gfxn_CybergraphicsRefreshRect16(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;
        UWORD *SrcPtr=&((UWORD *)fb->BufferPtr)[x+y*fb->Width];
        LONG Mod=fb->Width-w;
        LONG ht;

        x+=gfx->OffsetX;
        y+=gfx->OffsetY;
        while((ht=Gfxn_GetNormalHeight(gfx,w,h,sizeof(ULONG)))>0)
        {
            ULONG *DstPtr=gfx->NormalBufferPtr;
            LONG j=ht;

            while(--j>=0)
            {
                LONG i=w;

                while(--i>=0)
                {
                    ULONG Color=*(SrcPtr++);
                    *(DstPtr++)=((Color<<8)&0xf80000)+((Color<<5)&0xfc00)+(UBYTE)(Color<<3);
                }
                SrcPtr=&SrcPtr[Mod];
            }

            WritePixelArray(gfx->NormalBufferPtr,0,0,(UWORD)(w*sizeof(ULONG)),rp,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)ht,RECTFMT_ARGB);
            y+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 24 bits -> 24 bits
*****/

void Gfxn_CybergraphicsRefreshRect24(struct GfxNormal *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(Gfxn_ClipToView(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;

        WritePixelArray(fb->BufferPtr,(UWORD)x,(UWORD)y,(UWORD)(fb->Width*sizeof(ULONG)),rp,(UWORD)(x+gfx->OffsetX),(UWORD)(y+gfx->OffsetY),(UWORD)w,(UWORD)h,RECTFMT_ARGB);
    }
}


/************************************/
/*****                          *****/
/***** AUTRES FONCTIONS PRIVEES *****/
/*****                          *****/
/************************************/

/*****
    Permet d'obtenir le nombre de lignes utilisables dans le buffer,
    à partir des paramètres donnés en entrée.
*****/

LONG Gfxn_GetNormalHeight(struct GfxNormal *gfx, LONG w, LONG h, LONG BytePerPixel)
{
    LONG CountOfLines=gfx->NormalBufferSize/(w*BytePerPixel);
    if(CountOfLines>h) CountOfLines=h;
    return CountOfLines;
}


/*****
    Clipping de l'affichage en fonction de la vue
*****/

BOOL Gfxn_ClipToView(struct GfxNormal *gfx, LONG *x, LONG *y, LONG *w, LONG *h, BOOL AlignX)
{
    LONG x1=*x,y1=*y;
    LONG x2=*x+*w,y2=*y+*h;
    LONG c1=gfx->GfxI.ViewInfoPtr->ViewX,l1=gfx->GfxI.ViewInfoPtr->ViewY;
    LONG c2=c1+gfx->GfxI.ViewInfoPtr->ViewWidth,l2=l1+gfx->GfxI.ViewInfoPtr->ViewHeight;

    if(AlignX) x1=(x1/ALIGN_NORMAL)*ALIGN_NORMAL;
    if(x1<c1) x1=c1;
    if(y1<l1) y1=l1;
    if(x1>c2) x1=c2;
    if(y1>l2) y1=l2;
    if(AlignX) x2=(((ALIGN_NORMAL-1)+x2)/ALIGN_NORMAL)*ALIGN_NORMAL;
    if(x2<c1) x2=c1;
    if(y2<l1) y2=l1;
    if(x2>c2) x2=c2;
    if(y2>l2) y2=l2;

    *x=x1;
    *y=y1;
    *w=x2-x1;
    *h=y2-y1;
    if(*w>0 && *h>0) return TRUE;

    return FALSE;
}
