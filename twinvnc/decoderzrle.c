#include "system.h"
#include "util.h"
#include "decoderzrle.h"
#include "general.h"
#include "twin.h"


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    17-01-2016 (Seg)    Modifications suite au changement de l'API decoding
    16-01-2016 (Seg)    Modification des appels graphiques suite à la refonte de display.c
    08-01-2006 (Seg)    Optimisation du codec et gestion du PixelFormat du serveur
    17-05-2004 (Seg)    Gestion du décodage ZRle
*/


/***** Prototypes */
BOOL OpenDecoderZRle(struct Twin *);
void CloseDecoderZRle(struct Twin *);
LONG DecoderZRle(struct Twin *, LONG, LONG, LONG, LONG);


/*****
    Initialisation du flux ZLib
*****/

BOOL OpenDecoderZRle(struct Twin *Twin)
{
    Twin->DecoderZRle.IsInitStreamZRleOk=FALSE;
    if(Util_ZLibInit(&Twin->DecoderZRle.StreamZRle)==Z_OK) Twin->DecoderZRle.IsInitStreamZRleOk=TRUE;

    return Twin->DecoderZRle.IsInitStreamZRleOk;
}


/*****
    Libération du Flux ZLib
*****/

void CloseDecoderZRle(struct Twin *Twin)
{
    if(Twin->DecoderZRle.IsInitStreamZRleOk) Util_ZLibEnd(&Twin->DecoderZRle.StreamZRle);
    Twin->DecoderZRle.IsInitStreamZRleOk=FALSE;
}


/*****
    Décodage ZRle
*****/

LONG DecoderZRle(struct Twin *Twin, LONG x, LONG y, LONG w, LONG h)
{
    LONG Result;
    ULONG Size=0;

    if((Result=Gal_WaitRead(Twin,(void *)&Size,sizeof(Size)))>0)
    {
        struct Display *disp=&Twin->Display;
        struct FrameBuffer *fb=&disp->FrameBuffer;
        ULONG DestLen=w*h*fb->PixelFormatSize+3*128+1;
        ULONG SizeOfTmpBuf=64*64*fb->PixelBufferSize;

        Result=RESULT_NOT_ENOUGH_MEMORY;
        if(Size>SizeOfTmpBuf) SizeOfTmpBuf=Size;
        if(Util_PrepareDecodeBuffers(Twin,SizeOfTmpBuf,DestLen))
        {
            if((Result=Gal_WaitRead(Twin,(void *)Twin->SrcBufferPtr,Size))>0)
            {
                if(Util_ZLibUnCompress(&Twin->DecoderZRle.StreamZRle,Twin->SrcBufferPtr,Size,Twin->DstBufferPtr,DestLen)>=Z_OK)
                {
                    UBYTE *Ptr=Twin->DstBufferPtr;
                    ULONG PixelSize=(fb->PixelFormatSize<=3?fb->PixelFormatSize:3);
                    BOOL IsBigEndian=(BOOL)fb->PixelFormat->BigEndian;
                    LONG j;

                    for(j=0; j<h; j+=64)
                    {
                        LONG i,absy=y+j,dh=h-j;

                        if(dh>64) dh=64;
                        for(i=0; i<w; i+=64)
                        {
                            LONG absx=x+i,dw=w-i;
                            LONG Mode=(LONG)*(Ptr++),NPal=Mode&127;
                            LONG k;

                            if(dw>64) dw=64;

                            /* Récupération des palettes */
                            for(k=0; k<NPal; k++)
                            {
                                ULONG Color;
                                Ptr=Util_ReadBufferColor(PixelSize,IsBigEndian,Ptr,&Color);
                                Twin->DecoderZRle.Palette[k]=Frb_ConvertColorForFrameBuffer(fb,Color,PixelSize);
                            }

                            if(NPal==1)
                            {
                                Frb_FillRect(fb,absx,absy,dw,dh,Twin->DecoderZRle.Palette[0]);
                                disp->CurrentGfxPtr->FillRectFuncPtr(disp->CurrentGfxPtr,absx,absy,dw,dh,Twin->DecoderZRle.Palette[0]);
                            }
                            else
                            {
                                if(!(Mode&128))
                                {
                                    /* !Rle */
                                    if(NPal==0)
                                    {
                                        /* Raw */
                                        ULONG CountOfPixels=dw*dh;
                                        Frb_ConvertBufferForFrameBuffer(fb,Ptr,PixelSize,IsBigEndian,CountOfPixels);
                                        Frb_CopyRectFromBuffer(fb,Ptr,PixelSize,absx,absy,dw,dh);
                                        Ptr+=PixelSize*CountOfPixels;
                                    }
                                    else
                                    {
                                        LONG bppp=(NPal>16)?8:((NPal>4)?4:((NPal>2)?2:1));
                                        LONG Mask=(1<<bppp)-1;
                                        LONG Line=dh,l;

                                        for(l=absy; --Line>=0; l++)
                                        {
                                            LONG NBits=0,Data=0;
                                            LONG Col=dw,c;

                                            for(c=absx; --Col>=0; c++)
                                            {
                                                if(NBits<=0) {Data=(LONG)*(Ptr++); NBits=8;}
                                                NBits-=bppp;
                                                Frb_WritePixel(fb,c,l,Twin->DecoderZRle.Palette[(Data>>NBits)&Mask]);
                                            }
                                        }
                                    }
                                }
                                else
                                {
                                    /* Rle */
                                    LONG Idx=0,CountOfPixels=dw*dh;

                                    if(NPal==0)
                                    {
                                        while(Idx<CountOfPixels)
                                        {
                                            LONG Len=1,Data;
                                            ULONG Color;

                                            /* Récupère la couleur */
                                            Ptr=Util_ReadBufferColor(PixelSize,IsBigEndian,Ptr,&Color);

                                            /* Récupère la longueur */
                                            while((Data=(LONG)*(Ptr++))==255) Len+=Data;
                                            Len+=Data;

                                            Color=Frb_ConvertColorForFrameBuffer(fb,Color,PixelSize);
                                            Frb_FillScanLineRect(fb,absx,absy,dw,dh,Idx,Len,Color);
                                            Idx+=Len;
                                        }
                                    }
                                    else
                                    {
                                        while(Idx<CountOfPixels)
                                        {
                                            /* Récupère l'entête */
                                            LONG Len=1;
                                            LONG Head=(LONG)*(Ptr++);

                                            /* Récupère la longueur */
                                            if(Head&128)
                                            {
                                                LONG Data;

                                                while((Data=(LONG)*(Ptr++))==255) Len+=Data;
                                                Len+=Data;
                                            }

                                            /* Remplissage */
                                            Frb_FillScanLineRect(fb,absx,absy,dw,dh,Idx,Len,Twin->DecoderZRle.Palette[Head&127]);
                                            Idx+=Len;
                                        }
                                    }
                                }

                                disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,absx,absy,dw,dh);
                            }
                        }
                    }
                } else Result=RESULT_CORRUPTED_DATA;
            }
        }
    }

    return Result;
}
