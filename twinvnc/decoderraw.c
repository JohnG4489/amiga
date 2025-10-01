#include "system.h"
#include "util.h"
#include "decoderraw.h"
#include "general.h"
#include "twin.h"


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    17-01-2016 (Seg)    Modifications suite au changement de l'API decoding
    16-01-2016 (Seg)    Modification des appels graphiques suite à la refonte de display.c
    11-01-2006 (Seg)    Gestion du format du pixel serveur
    16-05-2004 (Seg)    Gestion du décodage Raw
*/


/***** Prototypes */
LONG DecoderRaw(struct Twin *, LONG, LONG, LONG, LONG);


/*****
    Décodage RAW
*****/

LONG DecoderRaw(struct Twin *Twin, LONG x, LONG y, LONG w, LONG h)
{
    LONG Result=RESULT_NOT_ENOUGH_MEMORY;
    struct Display *disp=&Twin->Display;
    struct FrameBuffer *fb=&disp->FrameBuffer;
    ULONG PixelSize=fb->PixelFormatSize;
    ULONG BytePerRow=w*PixelSize;
    const int Max=32;

    if(Util_PrepareDecodeBuffers(Twin,Max*BytePerRow,0))
    {
        BOOL IsBigEndian=(BOOL)fb->PixelFormat->BigEndian;

        Result=RESULT_SUCCESS;
        while(h>0 && Result>0)
        {
            LONG CountOfLine=h>Max?Max:h;

            if((Result=Gal_WaitRead(Twin,(void *)Twin->SrcBufferPtr,BytePerRow*CountOfLine))>0)
            {
                Frb_ConvertBufferForFrameBuffer(fb,Twin->SrcBufferPtr,PixelSize,IsBigEndian,w*CountOfLine);
                Frb_CopyRectFromBuffer(fb,Twin->SrcBufferPtr,PixelSize,x,y,w,CountOfLine);

                disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,x,y,w,CountOfLine);
                y+=CountOfLine;
                h-=CountOfLine;
            }
        }
    }

    return Result;
}
