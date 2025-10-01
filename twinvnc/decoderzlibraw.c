#include "system.h"
#include "util.h"
#include "decoderzlibraw.h"
#include "general.h"
#include "twin.h"


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    17-01-2016 (Seg)    Modifications suite au changement de l'API decoding
    16-01-2016 (Seg)    Modification des appels graphiques suite à la refonte de display.c
    11-01-2006 (Seg)    Gestion du format du pixel serveur
    17-05-2004 (Seg)    Gestion du décodage ZLib Raw
*/


/***** Prototypes */
BOOL OpenDecoderZLibRaw(struct Twin *);
void CloseDecoderZLibRaw(struct Twin *);
LONG DecoderZLibRaw(struct Twin *, LONG, LONG, LONG, LONG);


/*****
    Initialisation du flux ZLib
*****/

BOOL OpenDecoderZLibRaw(struct Twin *Twin)
{
    Twin->DecoderZLibRaw.IsInitStreamZLibRawOk=FALSE;
    if(Util_ZLibInit(&Twin->DecoderZLibRaw.StreamZLibRaw)==Z_OK) Twin->DecoderZLibRaw.IsInitStreamZLibRawOk=TRUE;

    return Twin->DecoderZLibRaw.IsInitStreamZLibRawOk;
}


/*****
    Libération du Flux ZLib
*****/

void CloseDecoderZLibRaw(struct Twin *Twin)
{
    if(Twin->DecoderZLibRaw.IsInitStreamZLibRawOk) Util_ZLibEnd(&Twin->DecoderZLibRaw.StreamZLibRaw);
    Twin->DecoderZLibRaw.IsInitStreamZLibRawOk=FALSE;
}


/*****
   Décodage ZLib RAW
*****/

LONG DecoderZLibRaw(struct Twin *Twin, LONG x, LONG y, LONG w, LONG h)
{
    LONG Result;
    ULONG Size=0;

    if((Result=Gal_WaitRead(Twin,(void *)&Size,sizeof(Size)))>0)
    {
        struct Display *disp=&Twin->Display;
        struct FrameBuffer *fb=&disp->FrameBuffer;
        LONG CountOfPixels=w*h;
        ULONG DestLen=CountOfPixels*fb->PixelFormatSize;

        Result=RESULT_NOT_ENOUGH_MEMORY;
        if(Util_PrepareDecodeBuffers(Twin,Size,DestLen))
        {
            if((Result=Gal_WaitRead(Twin,(void *)Twin->SrcBufferPtr,Size))>0)
            {
                if(Util_ZLibUnCompress(&Twin->DecoderZLibRaw.StreamZLibRaw,Twin->SrcBufferPtr,Size,Twin->DstBufferPtr,DestLen)==Z_OK)
                {
                    Frb_ConvertBufferForFrameBuffer(fb,Twin->DstBufferPtr,fb->PixelFormatSize,(BOOL)fb->PixelFormat->BigEndian,CountOfPixels);
                    Frb_CopyRectFromBuffer(fb,Twin->DstBufferPtr,fb->PixelFormatSize,x,y,w,h);
                    disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,x,y,w,h);
                } else Result=RESULT_CORRUPTED_DATA;
            }
        }
    }

    return Result;
}
