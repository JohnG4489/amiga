#include "system.h"
#include "util.h"
#include "decoderrre.h"
#include "general.h"
#include "twin.h"


/*
    17-01-2016 (Seg)    Modifications suite au changement de l'API decoding
    16-01-2016 (Seg)    Modification des appels graphiques suite à la refonte de display.c
    11-01-2006 (Seg)    Gestion du format du pixel serveur
    16-05-2004 (Seg)    Gestion du décodage RRE
*/


/***** Prototypes */
LONG DecoderRRE(struct Twin *, LONG, LONG, LONG, LONG);


/*****
    Décodage RRE
*****/

LONG DecoderRRE(struct Twin *Twin, LONG x, LONG y, LONG w, LONG h)
{
    LONG CountOfSubRect=0;
    LONG Result=Gal_WaitRead(Twin,(void *)&CountOfSubRect,sizeof(CountOfSubRect));

    if(Result>0)
    {
        struct Display *disp=&Twin->Display;
        struct FrameBuffer *fb=&disp->FrameBuffer;
        ULONG PixelSize=fb->PixelBufferSize;
        BOOL IsBigEndian=(BOOL)fb->PixelFormat->BigEndian;
        ULONG Color=0;

        if((Result=Util_ReadTwinColor(Twin,PixelSize,IsBigEndian,&Color))>0)
        {
            struct Rect16 Rect;

            Color=Frb_ConvertColorForFrameBuffer(fb,Color,PixelSize);
            Frb_FillRect(fb,x,y,w,h,Color);
            disp->CurrentGfxPtr->FillRectFuncPtr(disp->CurrentGfxPtr,x,y,w,h,Color);

            while(--CountOfSubRect>=0 && Result>0)
            {
                if((Result=Util_ReadTwinColor(Twin,PixelSize,IsBigEndian,&Color))>0)
                {
                    Result=Gal_WaitRead(Twin,(void *)&Rect,sizeof(Rect));
                    if(Result>0)
                    {
                        Color=Frb_ConvertColorForFrameBuffer(fb,Color,PixelSize);
                        Frb_FillRect(fb,x+(LONG)Rect.x,y+(LONG)Rect.y,(LONG)Rect.w,(LONG)Rect.h,Color);
                    }
                }
            }

            disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,x,y,w,h);
        }
    }

    return Result;
}
