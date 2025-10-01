#include "system.h"
#include "gfxoverlay.h"


/*
    07-01-2016 (Seg)    Refonte du code
    16-11-2005 (Seg)    Restructuration de l'affichage
    17-12-2004 (Seg)    Gestion de l'overlay
*/


/***** Prototypes */
ULONG Gfxo_Create(struct GfxOverlay *, struct ViewInfo *);
void Gfxo_Free(struct GfxOverlay *);
void Gfxo_Reset(struct GfxOverlay *);

void Gfxo_UpdateParam(struct GfxOverlay *);
void Gfxo_FillColorKey(struct GfxOverlay *);

void Gfxo_GraphicsFillRect(struct GfxOverlay *, LONG, LONG, LONG, LONG, ULONG);
void Gfxo_CybergraphicsFillRect8(struct GfxOverlay *, LONG, LONG, LONG, LONG, ULONG);
void Gfxo_CybergraphicsFillRect16(struct GfxOverlay *, LONG, LONG, LONG, LONG, ULONG);
void Gfxo_CybergraphicsFillRect24(struct GfxOverlay *, LONG, LONG, LONG, LONG, ULONG);

void Gfxo_GraphicsRefreshRect(struct GfxOverlay *, LONG, LONG, LONG, LONG);
void Gfxo_CybergraphicsRefreshRect8(struct GfxOverlay *, LONG, LONG, LONG, LONG);
void Gfxo_CybergraphicsRefreshRect16(struct GfxOverlay *, LONG, LONG, LONG, LONG);
void Gfxo_CybergraphicsRefreshRect24(struct GfxOverlay *, LONG, LONG, LONG, LONG);


/*****
    Création de l'overlay
*****/

ULONG Gfxo_Create(struct GfxOverlay *gfx, struct ViewInfo *view)
{
    ULONG Result=OVERLAY_NOT_AVAILABLE;

    gfx->GfxI.ViewInfoPtr=view;
    Gfxo_Reset(gfx);

    if(CGXVideoBase!=NULL)
    {
        struct Window *win=view->WindowBase;
        struct FrameBuffer *fb=view->FrameBufferPtr;
        LONG Error=0;

        gfx->VLHandle=CreateVLayerHandleTags(win->WScreen,
                VOA_SrcType,SRCFMT_RGB16,
                VOA_SrcWidth,fb->Width,
                VOA_SrcHeight,fb->Height,
                VOA_UseColorKey,TRUE,
                VOA_Error,&Error,
                TAG_DONE);

        if(gfx->VLHandle!=NULL)
        {
            if(!AttachVLayerTags(gfx->VLHandle,win,
                    VOA_LeftIndent,(ULONG)(view->BorderLeft-(LONG)win->BorderLeft),
                    VOA_TopIndent,(ULONG)(view->BorderTop-(LONG)win->BorderTop),
                    VOA_RightIndent,(ULONG)(view->BorderRight-(LONG)win->BorderRight),
                    VOA_BottomIndent,(ULONG)(view->BorderBottom-(LONG)win->BorderBottom),
                    TAG_DONE))
            {
                /* Overlay available! */
                Gfxo_FillColorKey(gfx);
                Result=OVERLAY_SUCCESS;
            }
            else
            {
                DeleteVLayerHandle(gfx->VLHandle);
                gfx->VLHandle=NULL;
            }
        }
        else
        {
            /* Overlay unavailable! */
            switch(Error)
            {
                case VOERR_INVSCRMODE:
                    break;

                case VOERR_NOOVLMEMORY:
                    Result=OVERLAY_NO_OVERLAY_MEMORY;
                    break;

                case VOERR_INVSRCFMT:
                    Result=OVERLAY_BAD_FORMAT;
                    break;

                case VOERR_NOMEMORY:
                    Result=OVERLAY_NOT_ENOUGH_MEMORY;
                    break;
            }
        }
    }

    return Result;
}


/*****
    Libération de l'overlay
*****/

void Gfxo_Free(struct GfxOverlay *gfx)
{
    if(CGXVideoBase!=NULL && gfx->VLHandle!=NULL)
    {
        DetachVLayer(gfx->VLHandle);
        DeleteVLayerHandle(gfx->VLHandle);
        gfx->VLHandle=NULL;
    }
}


/*****
    Définition des fonctions graphiques adaptées à la vue et au type d'écran donné
*****/

void Gfxo_Reset(struct GfxOverlay *gfx)
{
    struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;

    gfx->GfxI.UpdateParamFuncPtr=(void (*)(void *))Gfxo_UpdateParam;

    switch(vi->ScreenType)
    {
        case SCRTYPE_GRAPHICS:
            gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxo_GraphicsFillRect;
            gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxo_GraphicsRefreshRect;
            break;

        case SCRTYPE_CYBERGRAPHICS:
            switch(vi->FrameBufferPtr->PixelBufferSize)
            {
                case 1:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxo_CybergraphicsFillRect8;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxo_CybergraphicsRefreshRect8;
                    break;

                case 2:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxo_CybergraphicsFillRect16;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxo_CybergraphicsRefreshRect16;
                    break;

                default:
                    gfx->GfxI.FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxo_CybergraphicsFillRect24;
                    gfx->GfxI.RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxo_CybergraphicsRefreshRect24;
                    break;
            }
            break;
    }
}


/*****
    Remplissage de la fenêtre avec la couleur de l'overlay
*****/

void Gfxo_FillColorKey(struct GfxOverlay *gfx)
{
    if(CGXVideoBase!=NULL && gfx->VLHandle!=NULL)
    {
        ULONG ColorID=GetVLayerAttr(gfx->VLHandle,VOA_ColorKeyPen);
        struct ViewInfo *vi=gfx->GfxI.ViewInfoPtr;
        struct RastPort *rp=vi->WindowBase->RPort;

        SetAPen(rp,ColorID);
        RectFill(rp,
            vi->BorderLeft,vi->BorderTop,
            vi->BorderLeft+vi->ViewWidth-1,vi->BorderTop+vi->ViewHeight-1);
    }
}


/*****
    Mise à jour des paramètres suite à une modification de la vue
*****/

void Gfxo_UpdateParam(struct GfxOverlay *gfx)
{
    Gfxo_Reset(gfx);
    Gfxo_FillColorKey(gfx);
}


/******************************************/
/*****                                *****/
/***** FONCTIONS PRIVEES : FillRect() *****/
/*****                                *****/
/******************************************/

/*****
    Remplissage: Graphics
*****/

void Gfxo_GraphicsFillRect(struct GfxOverlay *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    /* NOTE: L'overlay ne fonctionne qu'en RTG */
}


/*****
    Remplissage: Cybergraphics - 8 bits -> 16 bits
*****/

void Gfxo_CybergraphicsFillRect8(struct GfxOverlay *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    if(LockVLayer(gfx->VLHandle))
    {
        struct FrameBuffer *fb=gfx->GfxI.ViewInfoPtr->FrameBufferPtr;
        UWORD *Ptr=&((UWORD *)GetVLayerAttr(gfx->VLHandle,VOA_BaseAddress))[x+y*fb->Width];
        LONG Mod=fb->Width-w;

        Color=fb->ColorMapRGB32[Color];
        Color=((Color>>8)&0xf800)+((Color>>5)&0x07e0)+((Color>>3)&0x001f);

        /* Conversion en Little Endian */
        Color=(Color>>8)+(Color<<8);

        while(--h>=0)
        {
            LONG i=w;
            while(--i>=0) *(Ptr++)=(UWORD)Color;
            Ptr=&Ptr[Mod];
        }

        UnLockVLayer(gfx->VLHandle);
    }
}


/*****
    Remplissage: Cybergraphics - 16 bits -> 16 bits
*****/

void Gfxo_CybergraphicsFillRect16(struct GfxOverlay *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    if(LockVLayer(gfx->VLHandle))
    {
        struct FrameBuffer *fb=gfx->GfxI.ViewInfoPtr->FrameBufferPtr;
        UWORD *Ptr=&((UWORD *)GetVLayerAttr(gfx->VLHandle,VOA_BaseAddress))[x+y*fb->Width];
        LONG Mod=fb->Width-w;

        while(--h>=0)
        {
            LONG i=w;
            while(--i>=0) *(Ptr++)=(UWORD)Color;
            Ptr=&Ptr[Mod];
        }

        UnLockVLayer(gfx->VLHandle);
    }
}


/*****
    Remplissage: Cybergraphics - 24 bits -> 16 bits
*****/

void Gfxo_CybergraphicsFillRect24(struct GfxOverlay *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    if(LockVLayer(gfx->VLHandle))
    {
        struct FrameBuffer *fb=gfx->GfxI.ViewInfoPtr->FrameBufferPtr;
        UWORD *Ptr=&((UWORD *)GetVLayerAttr(gfx->VLHandle,VOA_BaseAddress))[x+y*fb->Width];
        LONG Mod=fb->Width-w;

        Color=((Color>>8)&0xf800)+((Color>>5)&0x07e0)+((Color>>3)&0x001f);

        /* Conversion en Little Endian */
        Color=(Color>>8)+(Color<<8);

        while(--h>=0)
        {
            LONG i=w;
            while(--i>=0) *(Ptr++)=(UWORD)Color;
            Ptr=&Ptr[Mod];
        }

        UnLockVLayer(gfx->VLHandle);
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

void Gfxo_GraphicsRefreshRect(struct GfxOverlay *gfx, LONG x, LONG y, LONG w, LONG h)
{
    /* NOTE: L'overlay ne fonctionne qu'en RTG */
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 8 bits -> 16 bits
*****/

void Gfxo_CybergraphicsRefreshRect8(struct GfxOverlay *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(LockVLayer(gfx->VLHandle))
    {
        struct FrameBuffer *fb=gfx->GfxI.ViewInfoPtr->FrameBufferPtr;
        ULONG OffsetBase=x+y*fb->Width;
        UWORD *Ptr=&((UWORD *)GetVLayerAttr(gfx->VLHandle,VOA_BaseAddress))[OffsetBase];
        LONG Mod=fb->Width-w;
        UBYTE *BufferPtr=&((UBYTE *)fb->BufferPtr)[OffsetBase];

        while(--h>=0)
        {
            LONG i=w;

            while(--i>=0)
            {
                ULONG Color=fb->ColorMapRGB32[*(BufferPtr++)];
                Color=((Color>>8)&0xf800)+((Color>>5)&0x07e0)+((Color>>3)&0x001f);
                *(Ptr++)=(UWORD)((Color>>8)+(Color<<8));
            }
            Ptr=&Ptr[Mod];
            BufferPtr=&BufferPtr[Mod];
        }

        UnLockVLayer(gfx->VLHandle);
    }
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 16 bits -> 16 bits
*****/

void Gfxo_CybergraphicsRefreshRect16(struct GfxOverlay *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(LockVLayer(gfx->VLHandle))
    {
        struct FrameBuffer *fb=gfx->GfxI.ViewInfoPtr->FrameBufferPtr;
        ULONG OffsetBase=x+y*fb->Width;
        UWORD *Ptr=&((UWORD *)GetVLayerAttr(gfx->VLHandle,VOA_BaseAddress))[OffsetBase];
        LONG Mod=fb->Width-w;
        UWORD *BufferPtr=&((UWORD *)fb->BufferPtr)[OffsetBase];

        while(--h>=0)
        {
            LONG i=w;

            while(--i>=0)
            {
                ULONG Color=(ULONG)*(BufferPtr++);
                *(Ptr++)=(UWORD)((Color>>8)+(Color<<8));
            }
            Ptr=&Ptr[Mod];
            BufferPtr=&BufferPtr[Mod];
        }

        UnLockVLayer(gfx->VLHandle);
    }
}


/*****
    Rafraîchissement d'un bloc: Cybergraphics - 24 bits -> 16 bits
*****/

void Gfxo_CybergraphicsRefreshRect24(struct GfxOverlay *gfx, LONG x, LONG y, LONG w, LONG h)
{
    if(LockVLayer(gfx->VLHandle))
    {
        struct FrameBuffer *fb=gfx->GfxI.ViewInfoPtr->FrameBufferPtr;
        ULONG OffsetBase=x+y*fb->Width;
        UWORD *Ptr=&((UWORD *)GetVLayerAttr(gfx->VLHandle,VOA_BaseAddress))[OffsetBase];
        LONG Mod=fb->Width-w;
        ULONG *BufferPtr=&((ULONG *)fb->BufferPtr)[OffsetBase];

        while(--h>=0)
        {
            LONG i=w;

            while(--i>=0)
            {
                ULONG Color=*(BufferPtr++);
                Color=((Color>>8)&0xf800)+((Color>>5)&0x07e0)+((Color>>3)&0x001f);
                *(Ptr++)=(UWORD)((Color>>8)+(Color<<8));
            }
            Ptr=&Ptr[Mod];
            BufferPtr=&BufferPtr[Mod];
        }

        UnLockVLayer(gfx->VLHandle);
    }
}
