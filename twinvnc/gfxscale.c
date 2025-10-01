#include "system.h"
#include "gfxscale.h"


/*
    13-02-2016 (Seg)    Optimisation de l'algo
    16-01-2016 (Seg)    Refonte du code
    03-12-2006 (Seg)    Optimisation des routines de scale et correction d'un bug
                        de compilation SAS/C dans la fonction RecalcScaleRect().
    18-12-2005 (Seg)    Modification pour parer un bug de WriteChunkyPixel(). Les
                        blits sont maintenant alignes sur 32 octets.
    16-11-2005 (Seg)    Restructuration de l'affichage
    27-04-2005 (Seg)    Correction d'un bug graphique quand le scale est de
                        facteur 1.
    20-03-2005 (Seg)    Correction des bugs graphiques lies au scale
    17-01-2005 (Seg)    Scale soft sans dithering maintenant operationnel
    20-12-2004 (Seg)    Gestion de l'affichage scale du projet
*/


#define ALIGN_SCALE     32


/***** Prototypes */
BOOL Gfxs_Create(struct GfxScale *, struct ViewInfo *);
void Gfxs_Free(struct GfxScale *);
void Gfxs_Reset(struct GfxScale *);

void Gfxs_UpdateParam(struct GfxScale *);

void Gfxs_GraphicsFillRect8(struct GfxScale *, LONG, LONG, LONG, LONG, ULONG);
void Gfxs_GraphicsFillRect16(struct GfxScale *, LONG, LONG, LONG, LONG, ULONG);
void Gfxs_GraphicsFillRect24(struct GfxScale *, LONG, LONG, LONG, LONG, ULONG);
void Gfxs_CybergraphicsFillRect8(struct GfxScale *, LONG, LONG, LONG, LONG, ULONG);
void Gfxs_CybergraphicsFillRect16(struct GfxScale *, LONG, LONG, LONG, LONG, ULONG);
void Gfxs_CybergraphicsFillRect24(struct GfxScale *, LONG, LONG, LONG, LONG, ULONG);

void Gfxs_GraphicsRefreshRect8(struct GfxScale *, LONG, LONG, LONG, LONG);
void Gfxs_GraphicsRefreshRect16(struct GfxScale *, LONG, LONG, LONG, LONG);
void Gfxs_GraphicsRefreshRect24(struct GfxScale *, LONG, LONG, LONG, LONG);
void Gfxs_CybergraphicsRefreshRect8(struct GfxScale *, LONG, LONG, LONG, LONG);
void Gfxs_CybergraphicsRefreshRect16(struct GfxScale *, LONG, LONG, LONG, LONG);
void Gfxs_CybergraphicsRefreshRect24(struct GfxScale *, LONG, LONG, LONG, LONG);

LONG Gfxs_GetScaleHeight(struct GfxScale *, LONG, LONG, LONG);
BOOL Gfxs_ConvertFBCoordToViewCoord(struct GfxScale *, LONG *, LONG *, LONG *, LONG *, BOOL);


/*****
    Allocation des tables nécessaires au scale
*****/

BOOL Gfxs_Create(struct GfxScale *gfx, struct ViewInfo *view)
{
    BOOL IsSuccess=TRUE;

    gfx->GfxI.ViewInfoPtr=view;
    Gfxs_UpdateParam(gfx);

    if(view->ViewWidth>0 && view->ViewHeight>0)
    {
        IsSuccess=FALSE;

        gfx->ScaleBufferSize=view->ViewWidth*sizeof(LONG)*32;
        if((gfx->ScaleBufferPtr=Sys_AllocMem(gfx->ScaleBufferSize))!=NULL)
        {
            if((gfx->ScaleXTable=(LONG *)Sys_AllocMem(view->ViewWidth*sizeof(ULONG)))!=NULL)
            {
                if((gfx->ScaleYTable=(LONG *)Sys_AllocMem(view->ViewHeight*sizeof(ULONG)))!=NULL)
                {
                    ULONG i;
                    LONG Width=view->FrameBufferPtr->Width;
                    LONG Height=view->FrameBufferPtr->Height;

                    for(i=0;i<view->ViewWidth;i++) gfx->ScaleXTable[i]=i*Width/view->ViewWidth;
                    for(i=0;i<view->ViewHeight;i++) gfx->ScaleYTable[i]=(i*Height/view->ViewHeight)*Width;

                    IsSuccess=TRUE;
                }
            }
       }

        if(!IsSuccess) Gfxs_Free(gfx);
    }

    return IsSuccess;
}


/*****
    Libération des tables allouées pour le scale
*****/

void Gfxs_Free(struct GfxScale *gfx)
{
    if(gfx->ScaleXTable!=NULL) Sys_FreeMem((void *)gfx->ScaleXTable);
    if(gfx->ScaleYTable!=NULL) Sys_FreeMem((void *)gfx->ScaleYTable);
    if(gfx->ScaleBufferPtr!=NULL) Sys_FreeMem(gfx->ScaleBufferPtr);
    gfx->ScaleXTable=NULL;
    gfx->ScaleYTable=NULL;
    gfx->ScaleBufferPtr=NULL;
    gfx->ScaleBufferSize=0;
}


/*****
    Définition des fonctions graphiques adaptées à la vue et au type d'écran donné
*****/

void Gfxs_Reset(struct GfxScale *gfx)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    gfx->GfxI.UpdateParamFuncPtr=(void (*)(void *))Gfxs_UpdateParam;

    switch(vi->ScreenType)
    {
        case SCRTYPE_GRAPHICS:
            switch(vi->FrameBufferPtr->PixelBufferSize)
            {
                case 1:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxs_GraphicsFillRect8;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxs_GraphicsRefreshRect8;
                    break;

                case 2:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxs_GraphicsFillRect16;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxs_GraphicsRefreshRect16;
                    break;

                default:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxs_GraphicsFillRect24;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxs_GraphicsRefreshRect24;
                    break;
            }
            break;

        case SCRTYPE_CYBERGRAPHICS:
            switch(vi->FrameBufferPtr->PixelBufferSize)
            {
                case 1:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxs_CybergraphicsFillRect8;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxs_CybergraphicsRefreshRect8;
                    break;

                case 2:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxs_CybergraphicsFillRect16;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxs_CybergraphicsRefreshRect16;
                    break;

                default:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxs_CybergraphicsFillRect24;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxs_CybergraphicsRefreshRect24;
                    break;
            }
            break;
    }
}


/*****
    Mise à jour des paramètres suite à une modification de la vue
*****/

void Gfxs_UpdateParam(struct GfxScale *gfx)
{
    Gfxs_Reset(gfx);
}


/******************************************/
/*****                                *****/
/***** FONCTIONS PRIVEES : FillRect() *****/
/*****                                *****/
/******************************************/

/*****
    Remplissage: Graphics - 8 bits -> 8 bits max
*****/

void Gfxs_GraphicsFillRect8(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;

        x+=vi->BorderLeft;
        y+=vi->BorderTop;

        if(vi->DisplayType!=DISP_TYPE_CUSTOM_SCREEN) Color=vi->ColorMapID[Color];
        SetAPen(rp,(UBYTE)Color);
        RectFill(rp,x,y,x+w-1,y+h-1);

        Gfxs_GraphicsRefreshRect8(gfx,xo,yo,1,ho);
        Gfxs_GraphicsRefreshRect8(gfx,xo+wo-1,yo,1,ho);
        Gfxs_GraphicsRefreshRect8(gfx,xo,yo,wo,1);
        Gfxs_GraphicsRefreshRect8(gfx,xo,yo+ho-1,wo,1);
    }
}


/*****
    Remplissage: Graphics - 16 bits -> 8 bits max
*****/

void Gfxs_GraphicsFillRect16(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;

        x+=vi->BorderLeft;
        y+=vi->BorderTop;

        Color=((Color>>8)&0xe0)+((Color>>6)&0x1c)+((Color>>3)&0x03);
        if(vi->DisplayType!=DISP_TYPE_CUSTOM_SCREEN) Color=vi->ColorMapID[Color];
        SetAPen(rp,(UBYTE)Color);
        RectFill(rp,x,y,x+w-1,y+h-1);

        Gfxs_GraphicsRefreshRect16(gfx,xo,yo,1,ho);
        Gfxs_GraphicsRefreshRect16(gfx,xo+wo-1,yo,1,ho);
        Gfxs_GraphicsRefreshRect16(gfx,xo,yo,wo,1);
        Gfxs_GraphicsRefreshRect16(gfx,xo,yo+ho-1,wo,1);
    }
}


/*****
    Remplissage: Graphics - 24 bits -> 8 bits max
*****/

void Gfxs_GraphicsFillRect24(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;

        x+=vi->BorderLeft;
        y+=vi->BorderTop;

        Color=((Color>>16)&0xe0)+((Color>>11)&0x1c)+((Color>>6)&0x03);
        if(vi->DisplayType!=DISP_TYPE_CUSTOM_SCREEN) Color=vi->ColorMapID[Color];
        SetAPen(rp,(UBYTE)Color);
        RectFill(rp,x,y,x+w-1,y+h-1);

        Gfxs_GraphicsRefreshRect24(gfx,xo,yo,1,ho);
        Gfxs_GraphicsRefreshRect24(gfx,xo+wo-1,yo,1,ho);
        Gfxs_GraphicsRefreshRect24(gfx,xo,yo,wo,1);
        Gfxs_GraphicsRefreshRect24(gfx,xo,yo+ho-1,wo,1);
    }
}


/*****
    Remplissage: Cybergraphics - 8 bits -> 8 bits
*****/

void Gfxs_CybergraphicsFillRect8(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        x+=vi->BorderLeft;
        y+=vi->BorderTop;

        Color=vi->FrameBufferPtr->ColorMapRGB32[Color];
        FillPixelArray(vi->WindowBase->RPort,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)h,Color);

        Gfxs_CybergraphicsRefreshRect8(gfx,xo,yo,1,ho);
        Gfxs_CybergraphicsRefreshRect8(gfx,xo+wo-1,yo,1,ho);
        Gfxs_CybergraphicsRefreshRect8(gfx,xo,yo,wo,1);
        Gfxs_CybergraphicsRefreshRect8(gfx,xo,yo+ho-1,wo,1);
    }
}


/*****
    Remplissage: Cybergraphics - 16 bits -> 16 bits
*****/

void Gfxs_CybergraphicsFillRect16(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        x+=vi->BorderLeft;
        y+=vi->BorderTop;

        Color=((Color<<8)&0xf80000)+((Color<<5)&0xfc00)+(UBYTE)(Color<<3);
        FillPixelArray(vi->WindowBase->RPort,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)h,Color);

        Gfxs_CybergraphicsRefreshRect16(gfx,xo,yo,1,ho);
        Gfxs_CybergraphicsRefreshRect16(gfx,xo+wo-1,yo,1,ho);
        Gfxs_CybergraphicsRefreshRect16(gfx,xo,yo,wo,1);
        Gfxs_CybergraphicsRefreshRect16(gfx,xo,yo+ho-1,wo,1);
    }
}


/*****
    Remplissage: Cybergraphics - 24 bits -> 24 bits
*****/

void Gfxs_CybergraphicsFillRect24(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        x+=vi->BorderLeft;
        y+=vi->BorderTop;

        FillPixelArray(vi->WindowBase->RPort,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)h,Color);

        Gfxs_CybergraphicsRefreshRect24(gfx,xo,yo,1,ho);
        Gfxs_CybergraphicsRefreshRect24(gfx,xo+wo-1,yo,1,ho);
        Gfxs_CybergraphicsRefreshRect24(gfx,xo,yo,wo,1);
        Gfxs_CybergraphicsRefreshRect24(gfx,xo,yo+ho-1,wo,1);
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

void Gfxs_GraphicsRefreshRect8(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,TRUE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;
        void *BufferPtr=vi->FrameBufferPtr->BufferPtr;
        LONG *OffsetYPtr=&gfx->ScaleYTable[y];
        LONG xs=vi->BorderLeft+x,ys=vi->BorderTop+y,ht;

        while((ht=Gfxs_GetScaleHeight(gfx,w,h,sizeof(UBYTE)))>0)
        {
            UBYTE *DstPtr=gfx->ScaleBufferPtr;
            LONG j=ht;

            while(--j>=0)
            {
                LONG *OffsetXPtr=&gfx->ScaleXTable[x];
                UBYTE *BasePtr=&((UBYTE *)BufferPtr)[*(OffsetYPtr++)];
                LONG i=w;

                if(vi->DisplayType==DISP_TYPE_CUSTOM_SCREEN)
                {
                    while(--i>=0) *(DstPtr++)=BasePtr[*(OffsetXPtr++)];
                }
                else
                {
                    while(--i>=0) *(DstPtr++)=vi->ColorMapID[BasePtr[*(OffsetXPtr++)]];
                }
            }

            WriteChunkyPixels(rp,xs,ys,xs+w-1,ys+ht-1,(UBYTE *)gfx->ScaleBufferPtr,w);
            ys+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Graphics - 16 bits -> 8 bits max
*****/

void Gfxs_GraphicsRefreshRect16(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,TRUE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;
        void *BufferPtr=vi->FrameBufferPtr->BufferPtr;
        LONG *OffsetYPtr=&gfx->ScaleYTable[y];
        LONG xs=vi->BorderLeft+x,ys=vi->BorderTop+y,ht;

        while((ht=Gfxs_GetScaleHeight(gfx,w,h,sizeof(UBYTE)))>0)
        {
            UBYTE *DstPtr=gfx->ScaleBufferPtr;
            LONG j=ht;

            while(--j>=0)
            {
                LONG *OffsetXPtr=&gfx->ScaleXTable[x];
                UWORD *BasePtr=&((UWORD *)BufferPtr)[*(OffsetYPtr++)];
                LONG i=w;

                if(vi->DisplayType==DISP_TYPE_CUSTOM_SCREEN)
                {
                    while(--i>=0)
                    {
                        ULONG Color=BasePtr[*(OffsetXPtr++)];
                        *(DstPtr++)=((Color>>8)&0xe0)+((Color>>6)&0x1c)+((Color>>3)&0x03);
                    }
                }
                else
                {
                    while(--i>=0)
                    {
                        ULONG Color=BasePtr[*(OffsetXPtr++)];
                        Color=((Color>>8)&0xe0)+((Color>>6)&0x1c)+((Color>>3)&0x03);
                        *(DstPtr++)=vi->ColorMapID[Color];
                    }
                }
            }

            WriteChunkyPixels(rp,xs,ys,xs+w-1,ys+ht-1,(UBYTE *)gfx->ScaleBufferPtr,w);
            ys+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Graphics - 24 bits -> 8 bits max
*****/

void Gfxs_GraphicsRefreshRect24(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,TRUE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;
        void *BufferPtr=vi->FrameBufferPtr->BufferPtr;
        LONG *OffsetYPtr=&gfx->ScaleYTable[y];
        LONG xs=vi->BorderLeft+x,ys=vi->BorderTop+y,ht;

        while((ht=Gfxs_GetScaleHeight(gfx,w,h,sizeof(UBYTE)))>0)
        {
            UBYTE *DstPtr=gfx->ScaleBufferPtr;
            LONG j=ht;

            while(--j>=0)
            {
                LONG *OffsetXPtr=&gfx->ScaleXTable[x];
                ULONG *BasePtr=&((ULONG *)BufferPtr)[*(OffsetYPtr++)];
                LONG i=w;

                if(vi->DisplayType==DISP_TYPE_CUSTOM_SCREEN)
                {
                    while(--i>=0)
                    {
                        ULONG Color=BasePtr[*(OffsetXPtr++)];
                        *(DstPtr++)=((Color>>16)&0xe0)+((Color>>11)&0x1c)+((Color>>6)&0x03);
                    }
                }
                else
                {
                    while(--i>=0)
                    {
                        ULONG Color=BasePtr[*(OffsetXPtr++)];
                        Color=((Color>>16)&0xe0)+((Color>>11)&0x1c)+((Color>>6)&0x03);
                        *(DstPtr++)=vi->ColorMapID[Color];
                    }
                }
            }

            WriteChunkyPixels(rp,xs,ys,xs+w-1,ys+ht-1,(UBYTE *)gfx->ScaleBufferPtr,w);
            ys+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 8 bits -> 8 bits
*****/

void Gfxs_CybergraphicsRefreshRect8(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;
        void *BufferPtr=vi->FrameBufferPtr->BufferPtr;
        LONG *OffsetYPtr=&gfx->ScaleYTable[y];
        LONG xs=vi->BorderLeft+x,ys=vi->BorderTop+y,ht;

        while((ht=Gfxs_GetScaleHeight(gfx,w,h,sizeof(UBYTE)))>0)
        {
            UBYTE *DstPtr=gfx->ScaleBufferPtr;
            LONG j=ht;

            while(--j>=0)
            {
                LONG *OffsetXPtr=&gfx->ScaleXTable[x];
                UBYTE *BasePtr=&((UBYTE *)BufferPtr)[*(OffsetYPtr++)];
                LONG i=w;

                while(--i>=0) *(DstPtr++)=BasePtr[*(OffsetXPtr++)];
            }

            WriteLUTPixelArray((APTR)gfx->ScaleBufferPtr,0,0,(UWORD)w,rp,vi->FrameBufferPtr->ColorMapRGB32,(UWORD)xs,(UWORD)ys,(UWORD)w,(UWORD)ht,CTABFMT_XRGB8);
            ys+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 16 bits -> 16 bits
*****/

void Gfxs_CybergraphicsRefreshRect16(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;
        void *BufferPtr=vi->FrameBufferPtr->BufferPtr;
        LONG *OffsetYPtr=&gfx->ScaleYTable[y];
        LONG xs=vi->BorderLeft+x,ys=vi->BorderTop+y,ht;

        while((ht=Gfxs_GetScaleHeight(gfx,w,h,sizeof(ULONG)))>0)
        {
            ULONG *DstPtr=gfx->ScaleBufferPtr;
            LONG j=ht;

            while(--j>=0)
            {
                LONG *OffsetXPtr=&gfx->ScaleXTable[x];
                UWORD *BasePtr=&((UWORD *)BufferPtr)[*(OffsetYPtr++)];
                LONG i=w;

                while(--i>=0)
                {
                    ULONG Color=BasePtr[*(OffsetXPtr++)];
                    *(DstPtr++)=((Color<<8)&0xf80000)+((Color<<5)&0xfc00)+(UBYTE)(Color<<3);
                }
            }

            WritePixelArray((APTR)gfx->ScaleBufferPtr,0,0,(UWORD)(w*sizeof(ULONG)),rp,(UWORD)xs,(UWORD)ys,(UWORD)w,(UWORD)ht,RECTFMT_ARGB);
            ys+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 24 bits -> 24 bits
*****/

void Gfxs_CybergraphicsRefreshRect24(struct GfxScale *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(Gfxs_ConvertFBCoordToViewCoord(gfx,&x,&y,&w,&h,FALSE))
    {
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;
        void *BufferPtr=vi->FrameBufferPtr->BufferPtr;
        LONG *OffsetYPtr=&gfx->ScaleYTable[y];
        LONG xs=vi->BorderLeft+x,ys=vi->BorderTop+y,ht;

        while((ht=Gfxs_GetScaleHeight(gfx,w,h,sizeof(ULONG)))>0)
        {
            ULONG *DstPtr=gfx->ScaleBufferPtr;
            LONG j=ht;

            while(--j>=0)
            {
                LONG *OffsetXPtr=&gfx->ScaleXTable[x];
                ULONG *BasePtr=&((ULONG *)BufferPtr)[*(OffsetYPtr++)];
                LONG i=w;

                while(--i>=0) *(DstPtr++)=BasePtr[*(OffsetXPtr++)];
            }

            WritePixelArray((APTR)gfx->ScaleBufferPtr,0,0,(UWORD)(w*sizeof(ULONG)),rp,(UWORD)xs,(UWORD)ys,(UWORD)w,(UWORD)ht,RECTFMT_ARGB);
            ys+=ht;
            h-=ht;
        }
    }
}


/************************************/
/*****                          *****/
/***** AUTRES FONCTIONS PRIVEES *****/
/*****                          *****/
/************************************/

/*****
    Permet d'obtenir le nombre de lignes utilisables dans le buffer de scale,
    à partir des paramètres donnés en entrée.
*****/

LONG Gfxs_GetScaleHeight(struct GfxScale *gfx, LONG w, LONG h, LONG BytePerPixel)
{
    LONG CountOfLines=gfx->ScaleBufferSize/(w*BytePerPixel);
    if(CountOfLines>h) CountOfLines=h;
    return CountOfLines;
}


/*****
    Transforme les coordonnées du frame buffer en coordonnées de la vue
*****/

BOOL Gfxs_ConvertFBCoordToViewCoord(struct GfxScale *gfx, LONG *x, LONG *y, LONG *w, LONG *h, BOOL AlignX)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
    struct FrameBuffer *fb=vi->FrameBufferPtr;
    LONG x2=((*x+*w)*vi->ViewWidth)/fb->Width;
    LONG y2=((*y+*h)*vi->ViewHeight)/fb->Height;

    *x=(*x*vi->ViewWidth)/fb->Width;
    *y=(*y*vi->ViewHeight)/fb->Height;

    if(AlignX && *x<=x2)
    {
        *x=(*x/ALIGN_SCALE)*ALIGN_SCALE;
        x2=(((ALIGN_SCALE-1)+x2)/ALIGN_SCALE)*ALIGN_SCALE;
    }

    if(x2>=vi->ViewWidth) x2=vi->ViewWidth-1;
    if(y2>=vi->ViewHeight) y2=vi->ViewHeight-1;

    *w=x2-*x+1;
    *h=y2-*y+1;

    if(*w>0 && *h>0) return TRUE;

    return FALSE;
}
