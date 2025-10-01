#include "system.h"
#include "gfxsmooth.h"


/*
    08-01-2016 (Seg)    Refonte du code
    23-12-2006 (Seg)    Ajout du lissage non RTG (pas termine)
    01-05-2006 (Seg)    Gestion de l'affichage scale lissé du projet
*/


#define ALIGN_SMOOTH            32
#define SMOOTH_CMID_COUNT       512
#define SMOOTH_CMID_GETR(r)     (((r>>6)&7)*255/7)
#define SMOOTH_CMID_GETG(g)     (((g>>3)&7)*255/7)
#define SMOOTH_CMID_GETB(b)     ((b&7)*255/7)

struct SmoothData
{
    LONG Size;
    LONG FactMin;
    LONG FactMax;
    LONG FactSum;
    LONG Offset;

    LONG ShiftMin;
    LONG ShiftMax;
};


/***** Prototypes */
BOOL Gfxsm_Create(struct GfxSmooth *, struct ViewInfo *);
void Gfxsm_Free(struct GfxSmooth *);
void Gfxsm_Reset(struct GfxSmooth *);

void Gfxsm_UpdateParam(struct GfxSmooth *);

void Gfxsm_GraphicsFillRect8(struct GfxSmooth *, LONG, LONG, LONG, LONG, ULONG);
void Gfxsm_GraphicsFillRect16(struct GfxSmooth *, LONG, LONG, LONG, LONG, ULONG);
void Gfxsm_GraphicsFillRect24(struct GfxSmooth *, LONG, LONG, LONG, LONG, ULONG);
void Gfxsm_CybergraphicsFillRect8(struct GfxSmooth *, LONG, LONG, LONG, LONG, ULONG);
void Gfxsm_CybergraphicsFillRect16(struct GfxSmooth *, LONG, LONG, LONG, LONG, ULONG);
void Gfxsm_CybergraphicsFillRect24(struct GfxSmooth *, LONG, LONG, LONG, LONG, ULONG);

void Gfxsm_GraphicsRefreshRect8(struct GfxSmooth *, LONG, LONG, LONG, LONG);
void Gfxsm_GraphicsRefreshRect16(struct GfxSmooth *, LONG, LONG, LONG, LONG);
void Gfxsm_GraphicsRefreshRect24(struct GfxSmooth *, LONG, LONG, LONG, LONG);
void Gfxsm_CybergraphicsRefreshRect8(struct GfxSmooth *, LONG, LONG, LONG, LONG);
void Gfxsm_CybergraphicsRefreshRect16(struct GfxSmooth *, LONG, LONG, LONG, LONG);
void Gfxsm_CybergraphicsRefreshRect24(struct GfxSmooth *, LONG, LONG, LONG, LONG);

LONG Gfxsm_GetShift(LONG);
LONG Gfxsm_FindBestColor(LONG *, LONG, LONG, LONG, LONG);
BOOL Gfxsm_RecalcSmoothRect(struct ViewInfo *, LONG *, LONG *, LONG *, LONG *, BOOL);
LONG Gfxsm_GetSmoothHeight(struct GfxSmooth *, LONG, LONG, LONG);

void Gfxsm_ScaleColorMap8Int(struct GfxSmooth *, UBYTE *, LONG, UBYTE *, struct SmoothData *, struct SmoothData *, LONG, LONG);
void Gfxsm_ScaleColorMap16Int(struct GfxSmooth *, UWORD *, LONG, UBYTE *, struct SmoothData *, struct SmoothData *, LONG, LONG);
void Gfxsm_ScaleColorMap32Int(struct GfxSmooth *, ULONG *, LONG, UBYTE *, struct SmoothData *, struct SmoothData *, LONG, LONG);
void Gfxsm_ScaleTrueColor8Int(struct GfxSmooth *, UBYTE *, LONG, ULONG *, struct SmoothData *, struct SmoothData *, LONG, LONG);
void Gfxsm_ScaleTrueColor16Int(UWORD *, LONG, ULONG *, struct SmoothData *, struct SmoothData *, LONG, LONG);
void Gfxsm_ScaleTrueColor32Int(ULONG *, LONG, ULONG *, struct SmoothData *, struct SmoothData *, LONG, LONG);


/*****
    Allocation des tables nécessaires au smooth
*****/

BOOL Gfxsm_Create(struct GfxSmooth *gfx, struct ViewInfo *view)
{
    BOOL IsSuccess=TRUE;

    gfx->GfxI.ViewInfoPtr=view;
    Gfxsm_UpdateParam(gfx);

    if(view->ViewWidth>0 && view->ViewHeight>0)
    {
        IsSuccess=FALSE;

        gfx->SmoothBufferSize=view->ViewWidth*sizeof(LONG)*32;
        if((gfx->SmoothBufferPtr=Sys_AllocMem(gfx->SmoothBufferSize))!=NULL)
        {
            if((gfx->SmoothXTable=Sys_AllocMem(view->ViewWidth*sizeof(struct SmoothData)))!=NULL)
            {
                if((gfx->SmoothYTable=Sys_AllocMem(view->ViewHeight*sizeof(struct SmoothData)))!=NULL)
                {
                    if((gfx->SmoothColorMapID=(UBYTE *)Sys_AllocMem(SMOOTH_CMID_COUNT))!=NULL)
                    {
                        ULONG i;
                        struct FrameBuffer *fb=view->FrameBufferPtr;
                        LONG Width=fb->Width;
                        LONG Height=fb->Height;

                        for(i=0;i<view->ViewWidth;i++)
                        {
                            struct SmoothData *sd=&((struct SmoothData *)gfx->SmoothXTable)[i];
                            ULONG Min=((i*Width)<<8)/view->ViewWidth;
                            ULONG Max=(((i+1)*Width)<<8)/view->ViewWidth;

                            sd->Offset=Min>>8;
                            sd->Size=((Max+0xff)>>8)-(Min>>8);
                            sd->ShiftMin=Gfxsm_GetShift(0x100-(Min&0xff));
                            sd->ShiftMax=Gfxsm_GetShift(Max&0xff);
                        }

                        for(i=0;i<view->ViewHeight;i++)
                        {
                            struct SmoothData *sd=&((struct SmoothData *)gfx->SmoothYTable)[i];
                            ULONG Min=((i*Height)<<8)/view->ViewHeight;
                            ULONG Max=(((i+1)*Height)<<8)/view->ViewHeight;

                            sd->Offset=(Min>>8)*Width;
                            sd->Size=((Max+0xff)>>8)-(Min>>8);
                            sd->ShiftMin=Gfxsm_GetShift(0x100-(Min&0xff));
                            sd->ShiftMax=Gfxsm_GetShift(Max&0xff);
                        }

                        for(i=0;i<SMOOTH_CMID_COUNT;i++)
                        {
                            LONG Idx=Gfxsm_FindBestColor(fb->ColorMapRGB32,fb->ColorCount,SMOOTH_CMID_GETR(i),SMOOTH_CMID_GETG(i),SMOOTH_CMID_GETB(i));
                            if(view->DisplayType!=DISP_TYPE_CUSTOM_SCREEN) gfx->SmoothColorMapID[i]=(UBYTE)view->ColorMapID[Idx];
                            else gfx->SmoothColorMapID[i]=(UBYTE)Idx;
                        }

                        IsSuccess=TRUE;
                    }
                }
            }
        }

        if(!IsSuccess) Gfxsm_Free(gfx);
    }

    return IsSuccess;
}


/*****
    Libération des tables allouées pour le smooth
*****/

void Gfxsm_Free(struct GfxSmooth *gfx)
{
    if(gfx->SmoothColorMapID!=NULL) Sys_FreeMem((void *)gfx->SmoothColorMapID);
    if(gfx->SmoothXTable!=NULL) Sys_FreeMem(gfx->SmoothXTable);
    if(gfx->SmoothYTable!=NULL) Sys_FreeMem(gfx->SmoothYTable);
    if(gfx->SmoothBufferPtr!=NULL) Sys_FreeMem(gfx->SmoothBufferPtr);
    gfx->SmoothColorMapID=NULL;
    gfx->SmoothXTable=NULL;
    gfx->SmoothYTable=NULL;
    gfx->SmoothBufferPtr=NULL;
    gfx->SmoothBufferSize=0;
}


/*****
    Définition des fonctions graphiques adaptées à la vue et au type d'écran donné
*****/

void Gfxsm_Reset(struct GfxSmooth *gfx)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    gfx->GfxI.UpdateParamFuncPtr=(void (*)(void *))Gfxsm_UpdateParam;

    switch(vi->ScreenType)
    {
        case SCRTYPE_GRAPHICS:
            switch(vi->FrameBufferPtr->PixelBufferSize)
            {
                case 1:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxsm_GraphicsFillRect8;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxsm_GraphicsRefreshRect8;
                    break;

                case 2:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxsm_GraphicsFillRect16;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxsm_GraphicsRefreshRect16;
                    break;

                default:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxsm_GraphicsFillRect24;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxsm_GraphicsRefreshRect24;
                    break;
            }
            break;

        case SCRTYPE_CYBERGRAPHICS:
            switch(vi->FrameBufferPtr->PixelBufferSize)
            {
                case 1:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxsm_CybergraphicsFillRect8;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxsm_CybergraphicsRefreshRect8;
                    break;

                case 2:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxsm_CybergraphicsFillRect16;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxsm_CybergraphicsRefreshRect16;
                    break;

                default:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxsm_CybergraphicsFillRect24;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxsm_CybergraphicsRefreshRect24;
                    break;
            }
            break;
    }
}


/*****
    Mise à jour des paramètres suite à une modification de la vue
*****/

void Gfxsm_UpdateParam(struct GfxSmooth *gfx)
{
    Gfxsm_Reset(gfx);
}


/******************************************/
/*****                                *****/
/***** FONCTIONS PRIVEES : FillRect() *****/
/*****                                *****/
/******************************************/

/*****
    Remplissage: Graphics - 8 bits -> 8 bits max
*****/

void Gfxsm_GraphicsFillRect8(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;

        x+=vi->BorderLeft;
        y+=vi->BorderTop;
        Color=fb->ColorMapRGB32[Color];
        Color=((Color>>15)&0x1c0)+((Color>>10)&0x038)+((Color>>5)&0x07);
        Color=gfx->SmoothColorMapID[Color];
        SetAPen(rp,(UBYTE)Color);
        RectFill(rp,x,y,x+w-1,y+h-1);

        Gfxsm_GraphicsRefreshRect8(gfx,xo,yo,1,ho);
        Gfxsm_GraphicsRefreshRect8(gfx,xo+wo-1,yo,1,ho);
        Gfxsm_GraphicsRefreshRect8(gfx,xo,yo,wo,1);
        Gfxsm_GraphicsRefreshRect8(gfx,xo,yo+ho-1,wo,1);
    }
}


/*****
    Remplissage: Graphics - 16 bits -> 8 bits max
*****/

void Gfxsm_GraphicsFillRect16(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;

        x+=vi->BorderLeft;
        y+=vi->BorderTop;
        Color=((Color>>7)&0x1c0)+((Color>>5)&0x038)+((Color>>2)&0x007);
        Color=gfx->SmoothColorMapID[Color];
        SetAPen(rp,(UBYTE)Color);
        RectFill(rp,x,y,x+w-1,y+h-1);

        Gfxsm_GraphicsRefreshRect16(gfx,xo,yo,1,ho);
        Gfxsm_GraphicsRefreshRect16(gfx,xo+wo-1,yo,1,ho);
        Gfxsm_GraphicsRefreshRect16(gfx,xo,yo,wo,1);
        Gfxsm_GraphicsRefreshRect16(gfx,xo,yo+ho-1,wo,1);
    }
}


/*****
    Remplissage: Graphics - 24 bits -> 8 bits max
*****/

void Gfxsm_GraphicsFillRect24(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;

        x+=vi->BorderLeft;
        y+=vi->BorderTop;

        Color=((Color>>15)&0x1c0)+((Color>>10)&0x038)+((Color>>5)&0x07);
        Color=gfx->SmoothColorMapID[Color];
        SetAPen(rp,(UBYTE)Color);
        RectFill(rp,x,y,x+w-1,y+h-1);

        Gfxsm_GraphicsRefreshRect24(gfx,xo,yo,1,ho);
        Gfxsm_GraphicsRefreshRect24(gfx,xo+wo-1,yo,1,ho);
        Gfxsm_GraphicsRefreshRect24(gfx,xo,yo,wo,1);
        Gfxsm_GraphicsRefreshRect24(gfx,xo,yo+ho-1,wo,1);
    }
}


/*****
    Remplissage: Cybergraphics - 8 bits -> 8 bits
*****/

void Gfxsm_CybergraphicsFillRect8(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;

        x+=vi->BorderLeft;
        y+=vi->BorderTop;
        Color=fb->ColorMapRGB32[Color];
        FillPixelArray(rp,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)h,Color);

        Gfxsm_CybergraphicsRefreshRect8(gfx,xo,yo,1,ho);
        Gfxsm_CybergraphicsRefreshRect8(gfx,xo+wo-1,yo,1,ho);
        Gfxsm_CybergraphicsRefreshRect8(gfx,xo,yo,wo,1);
        Gfxsm_CybergraphicsRefreshRect8(gfx,xo,yo+ho-1,wo,1);
    }
}


/*****
    Remplissage: Cybergraphics - 16 bits -> 16 bits
*****/

void Gfxsm_CybergraphicsFillRect16(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;

        x+=vi->BorderLeft;
        y+=vi->BorderTop;
        Color=((Color<<8)&0xf80000)+((Color<<5)&0xfc00)+(UBYTE)(Color<<3);
        FillPixelArray(rp,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)h,Color);

        Gfxsm_CybergraphicsRefreshRect16(gfx,xo,yo,1,ho);
        Gfxsm_CybergraphicsRefreshRect16(gfx,xo+wo-1,yo,1,ho);
        Gfxsm_CybergraphicsRefreshRect16(gfx,xo,yo,wo,1);
        Gfxsm_CybergraphicsRefreshRect16(gfx,xo,yo+ho-1,wo,1);
    }
}


/*****
    Remplissage: Cybergraphics - 24 bits -> 24 bits
*****/

void Gfxsm_CybergraphicsFillRect24(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
    LONG xo=x,yo=y,wo=w,ho=h;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;

        x+=vi->BorderLeft;
        y+=vi->BorderTop;
        FillPixelArray(rp,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)h,Color);

        Gfxsm_CybergraphicsRefreshRect24(gfx,xo,yo,1,ho);
        Gfxsm_CybergraphicsRefreshRect24(gfx,xo+wo-1,yo,1,ho);
        Gfxsm_CybergraphicsRefreshRect24(gfx,xo,yo,wo,1);
        Gfxsm_CybergraphicsRefreshRect24(gfx,xo,yo+ho-1,wo,1);
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

void Gfxsm_GraphicsRefreshRect8(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,TRUE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;
        struct SmoothData *sdx=&((struct SmoothData *)gfx->SmoothXTable)[x];
        struct SmoothData *sdy=&((struct SmoothData *)gfx->SmoothYTable)[y];
        LONG xs,ys,ht;

        xs=vi->BorderLeft+x;
        ys=vi->BorderTop+y;
        while((ht=Gfxsm_GetSmoothHeight(gfx,w,h,sizeof(UBYTE)))>0)
        {
            Gfxsm_ScaleColorMap8Int(gfx,(UBYTE *)fb->BufferPtr,fb->Width,(UBYTE *)gfx->SmoothBufferPtr,sdx,sdy,w,ht);
            WriteChunkyPixels(rp,xs,ys,xs+w-1,ys+ht-1,(UBYTE *)gfx->SmoothBufferPtr,w);
            sdy=&sdy[ht];
            ys+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Graphics - 16 bits -> 8 bits max
*****/

void Gfxsm_GraphicsRefreshRect16(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,TRUE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;
        struct SmoothData *sdx=&((struct SmoothData *)gfx->SmoothXTable)[x];
        struct SmoothData *sdy=&((struct SmoothData *)gfx->SmoothYTable)[y];
        LONG xs,ys,ht;

        xs=vi->BorderLeft+x;
        ys=vi->BorderTop+y;
        while((ht=Gfxsm_GetSmoothHeight(gfx,w,h,sizeof(UBYTE)))>0)
        {
            Gfxsm_ScaleColorMap16Int(gfx,(UWORD *)fb->BufferPtr,fb->Width,(UBYTE *)gfx->SmoothBufferPtr,sdx,sdy,w,ht);
            WriteChunkyPixels(rp,xs,ys,xs+w-1,ys+ht-1,(UBYTE *)gfx->SmoothBufferPtr,w);
            sdy=&sdy[ht];
            ys+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Graphics - 24 bits -> 8 bits max
*****/

void Gfxsm_GraphicsRefreshRect24(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,TRUE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;
        struct SmoothData *sdx=&((struct SmoothData *)gfx->SmoothXTable)[x];
        struct SmoothData *sdy=&((struct SmoothData *)gfx->SmoothYTable)[y];
        LONG xs,ys,ht;

        xs=vi->BorderLeft+x;
        ys=vi->BorderTop+y;
        while((ht=Gfxsm_GetSmoothHeight(gfx,w,h,sizeof(UBYTE)))>0)
        {
            Gfxsm_ScaleColorMap32Int(gfx,(ULONG *)fb->BufferPtr,fb->Width,(UBYTE *)gfx->SmoothBufferPtr,sdx,sdy,w,ht);

            WriteChunkyPixels(rp,xs,ys,xs+w-1,ys+ht-1,(UBYTE *)gfx->SmoothBufferPtr,w);
            sdy=&sdy[ht];
            ys+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 8 bits -> 8 bits
*****/

void Gfxsm_CybergraphicsRefreshRect8(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;
        struct SmoothData *sdx=&((struct SmoothData *)gfx->SmoothXTable)[x];
        struct SmoothData *sdy=&((struct SmoothData *)gfx->SmoothYTable)[y];
        LONG xs,ys,ht;

        xs=vi->BorderLeft+x;
        ys=vi->BorderTop+y;
        while((ht=Gfxsm_GetSmoothHeight(gfx,w,h,sizeof(ULONG)))>0)
        {
            Gfxsm_ScaleTrueColor8Int(gfx,(UBYTE *)fb->BufferPtr,fb->Width,gfx->SmoothBufferPtr,sdx,sdy,w,ht);

            WritePixelArray((APTR)gfx->SmoothBufferPtr,0,0,(UWORD)(w*sizeof(ULONG)),rp,
                    (UWORD)xs,(UWORD)ys,(UWORD)w,(UWORD)ht,RECTFMT_ARGB);
            sdy=&sdy[ht];
            ys+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 16 bits -> 16 bits
*****/

void Gfxsm_CybergraphicsRefreshRect16(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;
        struct SmoothData *sdx=&((struct SmoothData *)gfx->SmoothXTable)[x];
        struct SmoothData *sdy=&((struct SmoothData *)gfx->SmoothYTable)[y];
        LONG xs,ys,ht;

        xs=vi->BorderLeft+x;
        ys=vi->BorderTop+y;
        while((ht=Gfxsm_GetSmoothHeight(gfx,w,h,sizeof(ULONG)))>0)
        {
            Gfxsm_ScaleTrueColor16Int((UWORD *)fb->BufferPtr,fb->Width,gfx->SmoothBufferPtr,sdx,sdy,w,ht);

            WritePixelArray((APTR)gfx->SmoothBufferPtr,0,0,(UWORD)(w*sizeof(ULONG)),rp,
                    (UWORD)xs,(UWORD)ys,(UWORD)w,(UWORD)ht,RECTFMT_ARGB);
            sdy=&sdy[ht];
            ys+=ht;
            h-=ht;
        }
    }
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 24 bits -> 24 bits
*****/

void Gfxsm_CybergraphicsRefreshRect24(struct GfxSmooth *gfx, LONG x, LONG y, LONG w, LONG h)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    if(Gfxsm_RecalcSmoothRect(vi,&x,&y,&w,&h,FALSE))
    {
        struct RastPort *rp=vi->WindowBase->RPort;
        struct FrameBuffer *fb=vi->FrameBufferPtr;
        struct SmoothData *sdx=&((struct SmoothData *)gfx->SmoothXTable)[x];
        struct SmoothData *sdy=&((struct SmoothData *)gfx->SmoothYTable)[y];
        LONG xs,ys,ht;

        xs=vi->BorderLeft+x;
        ys=vi->BorderTop+y;
        while((ht=Gfxsm_GetSmoothHeight(gfx,w,h,sizeof(ULONG)))>0)
        {
            Gfxsm_ScaleTrueColor32Int((ULONG *)fb->BufferPtr,fb->Width,gfx->SmoothBufferPtr,sdx,sdy,w,ht);
            WritePixelArray((APTR)gfx->SmoothBufferPtr,0,0,(UWORD)(w*sizeof(ULONG)),rp,
                    (UWORD)xs,(UWORD)ys,(UWORD)w,(UWORD)ht,RECTFMT_ARGB);
            sdy=&sdy[ht];
            ys+=ht;
            h-=ht;
        }
    }
}


/*********************************************/
/*****                                   *****/
/***** AUTRES FONCTIONS PRIVEES          *****/
/*****                                   *****/
/*********************************************/

/*****
    Sous-routine utilisée pour la création des tables de scale
*****/

LONG Gfxsm_GetShift(LONG Dec)
{
    if(Dec>=0xc0) return 8;
    if(Dec>=0x60) return 7;
    if(Dec>=0x30) return 6;
    if(Dec>=0x10) return 5;
    if(Dec>=0x08) return 4;
    if(Dec>=0x04) return 3;
    if(Dec>=0x02) return 2;
    if(Dec>=0x01) return 1;

    return 0;
}


/*****
    Permet de trouver la couleur la plus proche de celle desirée
*****/

LONG Gfxsm_FindBestColor(LONG *Pal, LONG NPal, LONG r, LONG v, LONG b)
{
    LONG k,c=0,max=255*255*3;

    /* Cherche la couleur la plus proche */
    for(k=0;--NPal>=0;k++)
    {
        LONG p=*Pal++;
        LONG dist=b-(LONG)(UBYTE)p;
        dist*=dist;
        if(dist<=max)
        {
            LONG vd=v-(LONG)(UBYTE)(p>>=8);
            dist+=vd*vd;
            if(dist<=max)
            {
                LONG rd=r-(p>>=8);
                dist+=rd*rd;
                if(dist<max)
                {
                    c=k;
                    max=dist;
                }
            }
        }
    }

    return c;
}


/*****
    Retourne les coordonnées "scalées" correspondant à la taille de la vue
*****/

BOOL Gfxsm_RecalcSmoothRect(struct ViewInfo *vi, LONG *x, LONG *y, LONG *w, LONG *h, BOOL AlignX)
{
    struct FrameBuffer *fb=vi->FrameBufferPtr;
    LONG ViewX1=*x*vi->ViewWidth/fb->Width;
    LONG ViewY1=*y*vi->ViewHeight/fb->Height;
    LONG ViewX2=*x+*w;
    LONG ViewY2=*y+*h;

    ViewX2=ViewX2*vi->ViewWidth/fb->Width;
    ViewY2=ViewY2*vi->ViewHeight/fb->Height;

    if(AlignX && ViewX1<=ViewX2)
    {
        ViewX1=(ViewX1/ALIGN_SMOOTH)*ALIGN_SMOOTH;
        ViewX2=(((ALIGN_SMOOTH-1)+ViewX2)/ALIGN_SMOOTH)*ALIGN_SMOOTH;
    }

    if(ViewX2>=vi->ViewWidth) ViewX2=vi->ViewWidth-1;
    if(ViewY2>=vi->ViewHeight) ViewY2=vi->ViewHeight-1;

    *x=ViewX1;
    *y=ViewY1;
    *w=ViewX2-ViewX1+1;
    *h=ViewY2-ViewY1+1;

    if(*w>0 && *h>0) return TRUE;

    return FALSE;
}


/*****
    Permet d'obtenir le nombre de lignes utilisables dans le buffer de smooth,
    à partir des paramètres donnés en entrée.
*****/

LONG Gfxsm_GetSmoothHeight(struct GfxSmooth *gfx, LONG w, LONG h, LONG BytePerPixel)
{
    LONG CountOfLines=gfx->SmoothBufferSize/(w*BytePerPixel);
    if(CountOfLines>h) CountOfLines=h;
    return CountOfLines;
}


/*****
    Scale lissé d'une source 8 bits pour un affichage Graphics
*****/

void Gfxsm_ScaleColorMap8Int(struct GfxSmooth *gfx, UBYTE *SrcPtr, LONG SrcWidth, UBYTE *DstPtr, struct SmoothData *sdx, struct SmoothData *sdy, LONG w, LONG h)
{
    ULONG *cm=gfx->GfxI.ViewInfoPtr->FrameBufferPtr->ColorMapRGB32;
    for(;--h>=0;sdy++)
    {
        struct SmoothData *sdx2;
        UBYTE *PtrY=&SrcPtr[sdy->Offset];
        LONG w2=w;

        for(sdx2=sdx;--w2>=0;sdx2++)
        {
            /* Calcul d'un Pixel */
            UBYTE *PtrXY=&PtrY[sdx2->Offset];
            ULONG area=0,rsum=0,gsum=0,bsum=0;
            ULONG ShiftMinY=sdy->ShiftMin;
            LONG hp=sdy->Size,Mod=SrcWidth-sdx2->Size;

            while(--hp>=0)
            {
                ULONG ShiftMinX=sdx2->ShiftMin;
                LONG wp=sdx2->Size;

                while(--wp>=0)
                {
                    ULONG RGB=cm[*(PtrXY++)];
                    ULONG Shift=(ShiftMinX*ShiftMinY)>>3;

                    area+=1<<Shift;
                    rsum+=((RGB>>16)&0xff)<<Shift;
                    gsum+=((RGB>>8)&0xff)<<Shift;
                    bsum+=(RGB&0xff)<<Shift;
                    ShiftMinX=(!wp?sdx2->ShiftMax:8);
                }

                ShiftMinY=(!hp?sdy->ShiftMax:8);
                PtrXY=&PtrXY[Mod];
            }

            area=(1<<24)/area;
            rsum=rsum*area+0x7fffff;
            gsum=gsum*area+0x7fffff;
            bsum=bsum*area+0x7fffff;
            *(DstPtr++)=gfx->SmoothColorMapID[((rsum>>23)&0x1c0)+((gsum>>26)&0x038)+((bsum>>29)&0x07)];
        }
    }
}


/*****
    Scale lissé d'une source 16 bits pour un affichage Graphics
*****/

void Gfxsm_ScaleColorMap16Int(struct GfxSmooth *gfx, UWORD *SrcPtr, LONG SrcWidth, UBYTE *DstPtr, struct SmoothData *sdx, struct SmoothData *sdy, LONG w, LONG h)
{
    for(;--h>=0;sdy++)
    {
        struct SmoothData *sdx2;
        UWORD *PtrY=&SrcPtr[sdy->Offset];
        LONG w2=w;

        for(sdx2=sdx;--w2>=0;sdx2++)
        {
            /* Calcul d'un Pixel */
            UWORD *PtrXY=&PtrY[sdx2->Offset];
            ULONG area=0,rsum=0,gsum=0,bsum=0;
            ULONG ShiftMinY=sdy->ShiftMin;
            LONG hp=sdy->Size,Mod=SrcWidth-sdx2->Size;

            while(--hp>=0)
            {
                ULONG ShiftMinX=sdx2->ShiftMin;
                LONG wp=sdx2->Size;

                while(--wp>=0)
                {
                    ULONG RGB=(ULONG)*(PtrXY++);
                    ULONG Shift=(ShiftMinX*ShiftMinY)>>3;

                    area+=1<<Shift;
                    rsum+=((RGB>>8)&0xf8)<<Shift;
                    gsum+=((RGB>>3)&0xfc)<<Shift;
                    bsum+=((RGB<<3)&0xf8)<<Shift;
                    ShiftMinX=(!wp?sdx2->ShiftMax:8);
                }

                ShiftMinY=(!hp?sdy->ShiftMax:8);
                PtrXY=&PtrXY[Mod];
            }

            area=(1<<24)/area;
            rsum=rsum*area+0x7fffff;
            gsum=gsum*area+0x7fffff;
            bsum=bsum*area+0x7fffff;
            *(DstPtr++)=gfx->SmoothColorMapID[((rsum>>23)&0x1c0)+((gsum>>26)&0x038)+((bsum>>29)&0x07)];
        }
    }
}


/*****
    Scale lissé d'une source 32 bits pour un affichage Graphics
*****/

void Gfxsm_ScaleColorMap32Int(struct GfxSmooth *gfx, ULONG *SrcPtr, LONG SrcWidth, UBYTE *DstPtr, struct SmoothData *sdx, struct SmoothData *sdy, LONG w, LONG h)
{
    for(;--h>=0;sdy++)
    {
        struct SmoothData *sdx2;
        ULONG *PtrY=&SrcPtr[sdy->Offset];
        LONG w2=w;

        for(sdx2=sdx;--w2>=0;sdx2++)
        {
            /* Calcul d'un Pixel */
            ULONG *PtrXY=&PtrY[sdx2->Offset];
            ULONG area=0,rsum=0,gsum=0,bsum=0;
            ULONG ShiftMinY=sdy->ShiftMin;
            LONG hp=sdy->Size,Mod=SrcWidth-sdx2->Size;

            while(--hp>=0)
            {
                ULONG ShiftMinX=sdx2->ShiftMin;
                LONG wp=sdx2->Size;

                while(--wp>=0)
                {
                    ULONG RGB=*(PtrXY++);
                    ULONG Shift=(ShiftMinX*ShiftMinY)>>3;

                    area+=1<<Shift;
                    rsum+=((RGB>>16)&0xff)<<Shift;
                    gsum+=((RGB>>8)&0xff)<<Shift;
                    bsum+=(RGB&0xff)<<Shift;
                    ShiftMinX=(!wp?sdx2->ShiftMax:8);
                }

                ShiftMinY=(!hp?sdy->ShiftMax:8);
                PtrXY=&PtrXY[Mod];
            }

            area=(1<<24)/area;
            rsum=rsum*area+0x7fffff;
            gsum=gsum*area+0x7fffff;
            bsum=bsum*area+0x7fffff;
            *(DstPtr++)=gfx->SmoothColorMapID[((rsum>>23)&0x1c0)+((gsum>>26)&0x038)+((bsum>>29)&0x07)];
        }
    }
}


/*****
    Scale lissé d'une source 8 bits pour un affichage CyberGraphics
*****/

void Gfxsm_ScaleTrueColor8Int(struct GfxSmooth *gfx, UBYTE *SrcPtr, LONG SrcWidth, ULONG *DstPtr, struct SmoothData *sdx, struct SmoothData *sdy, LONG w, LONG h)
{
    ULONG *ColorMapRGB32=gfx->GfxI.ViewInfoPtr->FrameBufferPtr->ColorMapRGB32;

    for(;--h>=0;sdy++)
    {
        struct SmoothData *sdx2;
        UBYTE *PtrY=&SrcPtr[sdy->Offset];
        LONG w2=w;

        for(sdx2=sdx;--w2>=0;sdx2++)
        {
            /* Calcul d'un Pixel */
            UBYTE *PtrXY=&PtrY[sdx2->Offset];
            ULONG area=0,rsum=0,gsum=0,bsum=0;
            ULONG ShiftMinY=sdy->ShiftMin;
            LONG hp=sdy->Size,Mod=SrcWidth-sdx2->Size;

            while(--hp>=0)
            {
                ULONG ShiftMinX=sdx2->ShiftMin;
                LONG wp=sdx2->Size;

                while(--wp>=0)
                {
                    ULONG RGB=ColorMapRGB32[*(PtrXY++)];
                    ULONG Shift=(ShiftMinX*ShiftMinY)>>3;

                    area+=1<<Shift;
                    rsum+=((RGB>>16)&0xff)<<Shift;
                    gsum+=((RGB>>8)&0xff)<<Shift;
                    bsum+=(RGB&0xff)<<Shift;
                    ShiftMinX=(!wp?sdx2->ShiftMax:8);
                }

                ShiftMinY=(!hp?sdy->ShiftMax:8);
                PtrXY=&PtrXY[Mod];
            }

            area=(1<<24)/area;
            rsum=rsum*area+0x7fffff;
            gsum=gsum*area+0x7fffff;
            bsum=bsum*area+0x7fffff;
            *(DstPtr++)=((rsum>>8)&0xff0000)+((gsum>>16)&0x00ff00)+(bsum>>24);
        }
    }
}


/*****
    Scale lissé d'une source 16 bits pour un affichage CyberGraphics
*****/

void Gfxsm_ScaleTrueColor16Int(UWORD *SrcPtr, LONG SrcWidth, ULONG *DstPtr, struct SmoothData *sdx, struct SmoothData *sdy, LONG w, LONG h)
{
    for(;--h>=0;sdy++)
    {
        struct SmoothData *sdx2;
        UWORD *PtrY=&SrcPtr[sdy->Offset];
        LONG w2=w;

        for(sdx2=sdx;--w2>=0;sdx2++)
        {
            /* Calcul d'un Pixel */
            UWORD *PtrXY=&PtrY[sdx2->Offset];
            ULONG area=0,rsum=0,gsum=0,bsum=0;
            ULONG ShiftMinY=sdy->ShiftMin;
            LONG hp=sdy->Size,Mod=SrcWidth-sdx2->Size;

            while(--hp>=0)
            {
                ULONG ShiftMinX=sdx2->ShiftMin;
                LONG wp=sdx2->Size;

                while(--wp>=0)
                {
                    ULONG RGB=(ULONG)*(PtrXY++);
                    ULONG Shift=(ShiftMinX*ShiftMinY)>>3;

                    area+=1<<Shift;
                    rsum+=((RGB>>8)&0xf8)<<Shift;
                    gsum+=((RGB>>3)&0xfc)<<Shift;
                    bsum+=((RGB<<3)&0xf8)<<Shift;
                    ShiftMinX=(!wp?sdx2->ShiftMax:8);
                }

                ShiftMinY=(!hp?sdy->ShiftMax:8);
                PtrXY=&PtrXY[Mod];
            }

            area=(1<<24)/area;
            rsum=rsum*area+0x7fffff;
            gsum=gsum*area+0x7fffff;
            bsum=bsum*area+0x7fffff;
            *(DstPtr++)=((rsum>>8)&0xf80000)+((gsum>>16)&0x00fc00)+(bsum>>24);
        }
    }
}


/*****
    Scale lissé d'une source 32 bits pour un affichage CyberGraphics
*****/

void Gfxsm_ScaleTrueColor32Int(ULONG *SrcPtr, LONG SrcWidth, ULONG *DstPtr, struct SmoothData *sdx, struct SmoothData *sdy, LONG w, LONG h)
{
    for(;--h>=0;sdy++)
    {
        struct SmoothData *sdx2;
        ULONG *PtrY=&SrcPtr[sdy->Offset];
        LONG w2=w;

        for(sdx2=sdx;--w2>=0;sdx2++)
        {
            /* Calcul d'un Pixel */
            ULONG *PtrXY=&PtrY[sdx2->Offset];
            ULONG area=0,rsum=0,gsum=0,bsum=0;
            ULONG ShiftMinY=sdy->ShiftMin;
            LONG hp=sdy->Size;
            LONG Mod=SrcWidth-sdx2->Size;

            while(--hp>=0)
            {
                ULONG ShiftMinX=sdx2->ShiftMin;
                LONG wp=sdx2->Size;

                while(--wp>=0)
                {
                    ULONG RGB=*(PtrXY++);
                    ULONG Shift=(ShiftMinX*ShiftMinY)>>3;

                    area+=1<<Shift;
                    rsum+=((RGB>>8)&0xff00)<<Shift;
                    gsum+=(RGB&0xff00)<<Shift;
                    bsum+=(RGB&0xff)<<Shift;
                    ShiftMinX=(!wp?sdx2->ShiftMax:8);
                }

                ShiftMinY=(!hp?sdy->ShiftMax:8);
                PtrXY=&PtrXY[Mod];
            }

            area=(1<<24)/area;
            rsum=(((rsum>>16)*area+0x7fff)&0xff0000);
            gsum=((((gsum>>16)*area+0x7fff)>>8)&0xff00);
            bsum=(bsum*area+0x7fffff)>>24;
            *(DstPtr++)=rsum+gsum+bsum;
        }
    }
}
