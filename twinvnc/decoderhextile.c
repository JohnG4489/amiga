#include "system.h"
#include "util.h"
#include "decoderhextile.h"
#include "general.h"
#include "twin.h"


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    17-01-2016 (Seg)    Modifications suite au changement de l'API decoding
    16-01-2016 (Seg)    Modification des appels graphiques suite à la refonte de display.c
    11-01-2006 (Seg)    Gestion du format du pixel serveur
    03-01-2006 (Seg)    Elimination de WriteTwinRead() pour aller plus vite
                        et optimisation de l'utilisation du PrepareDecodeBuffers().
    27-05-2004 (Seg)    Optimisation de la partie Raw
    17-05-2004 (Seg)    Gestion du décodage Hextile
*/


/***** Prototypes */
BOOL OpenDecoderHextile(struct Twin *);
void CloseDecoderHextile(struct Twin *);
LONG DecoderHextile(struct Twin *, LONG, LONG, LONG, LONG);


/*****
    Initialisation du flux ZLib
*****/

BOOL OpenDecoderHextile(struct Twin *Twin)
{
    Twin->DecoderHextile.IsInitZLibHexOk=FALSE;
    Twin->DecoderHextile.IsInitZLibHexRawOk=FALSE;

    if(Util_ZLibInit(&Twin->DecoderHextile.StreamZLibHex)==Z_OK) Twin->DecoderHextile.IsInitZLibHexOk=TRUE;
    if(Util_ZLibInit(&Twin->DecoderHextile.StreamZLibHexRaw)==Z_OK) Twin->DecoderHextile.IsInitZLibHexRawOk=TRUE;

    if(Twin->DecoderHextile.IsInitZLibHexOk && Twin->DecoderHextile.IsInitZLibHexRawOk) return TRUE;

    return FALSE;
}


/*****
    Libération du Flux ZLib
*****/

void CloseDecoderHextile(struct Twin *Twin)
{
    if(Twin->DecoderHextile.IsInitZLibHexOk) Util_ZLibEnd(&Twin->DecoderHextile.StreamZLibHex);
    if(Twin->DecoderHextile.IsInitZLibHexRawOk) Util_ZLibEnd(&Twin->DecoderHextile.StreamZLibHexRaw);
    Twin->DecoderHextile.IsInitZLibHexOk=FALSE;
    Twin->DecoderHextile.IsInitZLibHexRawOk=FALSE;
}


/*****
    Décodage Hextile
*****/

LONG DecoderHextile(struct Twin *Twin, LONG x, LONG y, LONG w, LONG h)
{
    LONG Result=RESULT_NOT_ENOUGH_MEMORY;
    struct Display *disp=&Twin->Display;
    struct FrameBuffer *fb=&disp->FrameBuffer;
    ULONG PixelSize=fb->PixelFormatSize;
    ULONG MinDstSize=4+4+1+16*16*(sizeof(UWORD)+PixelSize);
    ULONG MinSrcSize=2*MinDstSize;

    /* On prépare le buffer de destination qui ne peut pas faire plus gros que ce qui est
       calculé ci-dessus.
    */
    if(Util_PrepareDecodeBuffers(Twin,MinSrcSize,MinDstSize))
    {
        ULONG bgcolor=0,fgcolor=0;
        BOOL IsBigEndian=(BOOL)fb->PixelFormat->BigEndian;
        LONG j;

        Result=RESULT_SUCCESS;
        for(j=0;j<h && Result>0;j+=16)
        {
            LONG absy=y+j,dh=h-j,i;

            if(dh>16) dh=16;
            for(i=0;i<w && Result>0;i+=16)
            {
                LONG absx=x+i,dw=w-i;
                UBYTE Mask;

                if(dw>16) dw=16;
                if((Result=Gal_WaitRead(Twin,(void *)&Mask,sizeof(Mask)))>0)
                {
                    LONG CountOfPixels=dw*dh;
                    LONG DestLen=CountOfPixels*PixelSize;

                    if(Mask&RFB_HEXTILE_SUBENC_MASK_RAW)
                    {
                        if((Result=Gal_WaitRead(Twin,(void *)Twin->DstBufferPtr,DestLen))>0)
                        {
                            Frb_ConvertBufferForFrameBuffer(fb,Twin->DstBufferPtr,PixelSize,IsBigEndian,CountOfPixels);
                            Frb_CopyRectFromBuffer(fb,Twin->DstBufferPtr,PixelSize,absx,absy,dw,dh);
                        }
                    }
                    else if(Mask&RFB_HEXTILE_SUBENC_MASK_ZLIBRAW)
                    {
                        UWORD Size=0;

                        if((Result=Gal_WaitRead(Twin,(void *)&Size,sizeof(Size)))>0)
                        {
                            if((Result=Gal_WaitRead(Twin,(void *)Twin->SrcBufferPtr,(LONG)Size))>0)
                            {
                                if(Util_ZLibUnCompress(&Twin->DecoderHextile.StreamZLibHexRaw,Twin->SrcBufferPtr,(ULONG)Size,Twin->DstBufferPtr,DestLen)==Z_OK)
                                {
                                    Frb_ConvertBufferForFrameBuffer(fb,Twin->DstBufferPtr,PixelSize,IsBigEndian,CountOfPixels);
                                    Frb_CopyRectFromBuffer(fb,Twin->DstBufferPtr,PixelSize,absx,absy,dw,dh);
                                }
                            }
                        }
                    }
                    else if(Mask&RFB_HEXTILE_SUBENC_MASK_ZLIBHEX)
                    {
                        UWORD Size=0;

                        if((Result=Gal_WaitRead(Twin,(void *)&Size,sizeof(Size)))>0)
                        {
                            if((Result=Gal_WaitRead(Twin,(void *)Twin->SrcBufferPtr,(LONG)Size))>0)
                            {
                                UBYTE *Ptr=Twin->DstBufferPtr;

                                DestLen+=4+4+1;
                                if(Util_ZLibUnCompress(&Twin->DecoderHextile.StreamZLibHex,Twin->SrcBufferPtr,(ULONG)Size,Ptr,DestLen)==Z_OK)
                                {
                                    /* Récupération de la couleur de fond */
                                    if(Mask&RFB_HEXTILE_SUBENC_MASK_BACKGROUND_SPECIFIED)
                                    {
                                        Ptr=Util_ReadBufferColor(PixelSize,IsBigEndian,Ptr,&bgcolor);
                                        bgcolor=Frb_ConvertColorForFrameBuffer(fb,bgcolor,PixelSize);
                                    }

                                    /* Récupération de la couleur de forme */
                                    if(Mask&RFB_HEXTILE_SUBENC_MASK_FOREGROUND_SPECIFIED)
                                    {
                                        Ptr=Util_ReadBufferColor(PixelSize,IsBigEndian,Ptr,&fgcolor);
                                        fgcolor=Frb_ConvertColorForFrameBuffer(fb,fgcolor,PixelSize);
                                    }

                                    /* Remplissage du fond */
                                    Frb_FillRect(fb,absx,absy,dw,dh,bgcolor);

                                    /* Gestion des sous-rectangles */
                                    if(Mask&RFB_HEXTILE_SUBENC_MASK_ANY_SUBRECTS)
                                    {
                                        UBYTE CountOfSubRects;
                                        LONG k;

                                        Ptr=Util_ReadBufferData(Ptr,(void *)&CountOfSubRects,sizeof(CountOfSubRects));
                                        k=(LONG)CountOfSubRects;

                                        while(--k>=0)
                                        {
                                            LONG x2,y2,Tmp;

                                            if(Mask&RFB_HEXTILE_SUBENC_MASK_SUBRECTS_COLOURED)
                                            {
                                                Ptr=Util_ReadBufferColor(PixelSize,IsBigEndian,Ptr,&fgcolor);
                                                fgcolor=Frb_ConvertColorForFrameBuffer(fb,fgcolor,PixelSize);
                                            }

                                            Tmp=(LONG)*(Ptr++);
                                            x2=absx+(Tmp>>4);
                                            y2=absy+(Tmp&0x0f);
                                            Tmp=(LONG)*(Ptr++);
                                            Frb_FillRect(fb,x2,y2,(Tmp>>4)+1,(Tmp&0x0f)+1,fgcolor);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        /* Récupération de la couleur de fond */
                        if(Mask&RFB_HEXTILE_SUBENC_MASK_BACKGROUND_SPECIFIED)
                        {
                            Result=Util_ReadTwinColor(Twin,PixelSize,IsBigEndian,&bgcolor);
                            bgcolor=Frb_ConvertColorForFrameBuffer(fb,bgcolor,PixelSize);
                        }

                        /* Récupération de la couleur de forme */
                        if(Result>0 && (Mask&RFB_HEXTILE_SUBENC_MASK_FOREGROUND_SPECIFIED))
                        {
                            Result=Util_ReadTwinColor(Twin,PixelSize,IsBigEndian,&fgcolor);
                            fgcolor=Frb_ConvertColorForFrameBuffer(fb,fgcolor,PixelSize);
                        }

                        /* Remplissage du fond */
                        Frb_FillRect(fb,absx,absy,dw,dh,bgcolor);

                        /* Gestion des sous-rectangles */
                        if(Result>0 && (Mask&RFB_HEXTILE_SUBENC_MASK_ANY_SUBRECTS))
                        {
                            UBYTE CountOfSubRects;

                            if((Result=Gal_WaitRead(Twin,(void *)&CountOfSubRects,sizeof(CountOfSubRects)))>0)
                            {
                                UBYTE *Ptr=Twin->DstBufferPtr;
                                LONG ReadSize=sizeof(UWORD);

                                /* Calcule la taille des données à lire */
                                if(Mask&RFB_HEXTILE_SUBENC_MASK_SUBRECTS_COLOURED) ReadSize+=PixelSize;
                                ReadSize*=(LONG)CountOfSubRects;

                                Result=Gal_WaitRead(Twin,(void *)Ptr,ReadSize);
                                if(Result>0)
                                {
                                    LONG k=(LONG)CountOfSubRects;

                                    while(--k>=0)
                                    {
                                        LONG x2,y2,Tmp;

                                        if(Mask&RFB_HEXTILE_SUBENC_MASK_SUBRECTS_COLOURED)
                                        {
                                            Ptr=Util_ReadBufferColor(PixelSize,IsBigEndian,Ptr,&fgcolor);
                                            fgcolor=Frb_ConvertColorForFrameBuffer(fb,fgcolor,PixelSize);
                                        }

                                        Tmp=(LONG)*(Ptr++);
                                        x2=absx+(Tmp>>4);
                                        y2=absy+(Tmp&0x0f);
                                        Tmp=(LONG)*(Ptr++);
                                        Frb_FillRect(fb,x2,y2,(Tmp>>4)+1,(Tmp&0x0f)+1,fgcolor);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,x,absy,w,dh);
        }
    }

    return Result;
}
