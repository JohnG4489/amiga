#include "system.h"
#include "decodercopyrect.h"
#include "general.h"
#include "twin.h"


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    16-01-2016 (Seg)    Modification des appels graphiques suite à la refonte de display.c
    17-05-2004 (Seg)    Gestion du décodage CopyRect
*/


/***** Prototypes */
LONG DecoderCopyRect(struct Twin *, LONG, LONG, LONG, LONG);


/*****
    Décode CopyRect
*****/

LONG DecoderCopyRect(struct Twin *Twin, LONG x, LONG y, LONG w, LONG h)
{
    LONG Result;
    UWORD Coord[2];

    if((Result=Gal_WaitRead(Twin,(void *)Coord,sizeof(Coord)))>0)
    {
        struct Display *disp=&Twin->Display;
        struct ViewInfo *vi=&disp->ViewInfo;
        struct FrameBuffer *fb=&disp->FrameBuffer;

        Disp_ClearCursor(disp);
        Frb_CopyRect(fb,(LONG)Coord[0],(LONG)Coord[1],x,y,w,h);

        /* Déplacement d'une portion de l'écran */
        switch(disp->ViewInfo.DisplayType)
        {
            case DISP_TYPE_PUBLIC_SCREEN:
            case DISP_TYPE_CUSTOM_SCREEN:
                if(vi->ViewType==VIEW_TYPE_NORMAL)
                {
                    struct RastPort *rp=vi->WindowBase->RPort;
                    LONG xsrc=(LONG)Coord[0],ysrc=(LONG)Coord[1];

                    x+=vi->BorderLeft;
                    y+=vi->BorderTop;
                    xsrc+=vi->BorderLeft;
                    ysrc+=vi->BorderTop;

                    ClipBlit(rp,xsrc,ysrc,rp,x,y,w,h,0xc0);
                }
                else
                {
                    disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,x,y,w,h);
                }
                break;

            case DISP_TYPE_WINDOW:
                disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,x,y,w,h);
                break;

            default:
                break;
        }
    }

    return Result;
}
