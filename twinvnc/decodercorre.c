#include "system.h"
#include "util.h"
#include "decodercorre.h"
#include "general.h"
#include "twin.h"


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    17-01-2016 (Seg)    Modifications suite au changement de l'API decoding
    16-01-2016 (Seg)    Modification des appels graphiques suite à la refonte de display.c
    11-01-2006 (Seg)    Gestion du format du pixel serveur
    17-05-2004 (Seg)    Gestion du décodage CoRRE
*/


/***** Prototypes */
LONG DecoderCoRRE(struct Twin *, LONG, LONG, LONG, LONG);


/*****
    Décodage CoRRE
*****/

LONG DecoderCoRRE(struct Twin *Twin, LONG x, LONG y, LONG w, LONG h)
{
    ULONG CountOfSubRect=0;
    LONG Result=Gal_WaitRead(Twin,(void *)&CountOfSubRect,sizeof(CountOfSubRect));

    if(Result>0)
    {
        struct Display *disp=&Twin->Display;
        struct FrameBuffer *fb=&disp->FrameBuffer;
        ULONG PixelSize=fb->PixelFormatSize;
        BOOL IsBigEndian=(BOOL)fb->PixelFormat->BigEndian;
        ULONG Color=0;

        if((Result=Util_ReadTwinColor(Twin,PixelSize,IsBigEndian,&Color))>0)
        {
            ULONG i;
            struct Rect8 Rect;
            LONG x2=x,y2=y,w2=w,h2=h;

            Frb_ClipToFrameBuffer(fb,&x2,&y2,&w2,&h2);
            Color=Frb_ConvertColorForFrameBuffer(fb,Color,PixelSize);
            Frb_FillRect(fb,x2,y2,w2,h2,Color);

            for(i=0;i<CountOfSubRect && Result>0;i++)
            {
                if((Result=Util_ReadTwinColor(Twin,PixelSize,IsBigEndian,&Color))>0)
                {
                    Result=Gal_WaitRead(Twin,(void *)&Rect,sizeof(Rect));
                    if(Result>0)
                    {
                        x2=(LONG)Rect.x+x;
                        y2=(LONG)Rect.y+y;
                        w2=(LONG)Rect.w;
                        h2=(LONG)Rect.h;
                        Frb_ClipToFrameBuffer(fb,&x2,&y2,&w2,&h2);
                        Color=Frb_ConvertColorForFrameBuffer(fb,Color,PixelSize);
                        Frb_FillRect(fb,x2,y2,w2,h2,Color);
                    }
                }
            }

            disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,x,y,w,h);
        }
    }

    return Result;
}
