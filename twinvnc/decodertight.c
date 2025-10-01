#include "system.h"
#include "util.h"
#include "decodertight.h"
#include "general.h"
#include "twin.h"


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    17-01-2016 (Seg)    Modifications suite au changement de l'API decoding
    16-01-2016 (Seg)    Modification des appels graphiques suite à la refonte de display.c
    04-03-2006 (Seg)    Simplification du code du mode Gradient
    25-02-2006 (Seg)    Optimisation du mode Jpeg et du mode Gradient
    14-02-2006 (Seg)    Amélioration de la vitesse du rendu Jpeg
    14-01-2006 (Seg)    Correction d'un problème de lecture des couleurs lié au 24 bits
    11-01-2006 (Seg)    Gestion du format du pixel serveur
    04-08-2005 (Seg)    Correction d'un pb en Jpeg quand on quitte TwinVNC
    29-09-2004 (Seg)    Implémentation rapide du décodage Jpeg
    26-09-2004 (Seg)    Fin du décodage Tight 16 et 32 bits
    03-09-2004 (Seg)    Fin du décodage Tight 8 bits
    29-08-2004 (Seg)    Gestion du décodage Tight
*/


/***** Prototypes */
BOOL OpenDecoderTight(struct Twin *);
void CloseDecoderTight(struct Twin *);
LONG DecoderTight(struct Twin *, LONG, LONG, LONG, LONG);

LONG ReadPalette(struct Twin *);
LONG Gradient(struct Twin *, ULONG, LONG, LONG, LONG, LONG);
LONG Jpeg(struct Twin *, LONG, LONG, LONG, LONG);
LONG PrepareTightData(struct Twin *, ULONG, ULONG);
BOOL FlushTightStream(struct Twin *, ULONG);
LONG ReadLength(struct Twin *, ULONG *);
ULONG GetPixelProperties(struct FrameBuffer *, BOOL *);

void JpegInitSource(j_decompress_ptr);
void JpegTermSource(j_decompress_ptr);
boolean JpegFillInputBuffer(j_decompress_ptr);
void JpegSkipInputData(j_decompress_ptr, long);
void JpegErrorExit(j_common_ptr);


/*****
    Initialisation de la structure DecoderTight
*****/

BOOL OpenDecoderTight(struct Twin *Twin)
{
    Twin->DecoderTight.Flags[0]=FALSE;
    Twin->DecoderTight.Flags[1]=FALSE;
    Twin->DecoderTight.Flags[2]=FALSE;
    Twin->DecoderTight.Flags[3]=FALSE;
    Twin->DecoderTight.NPal=0;

    /* Création du Handle d'erreur */
    Twin->DecoderTight.JpegDec.err=jpeg_std_error(&Twin->DecoderTight.JpegErr);
    Twin->DecoderTight.JpegErr.error_exit=JpegErrorExit;

    /* Initialisation de la structure Jpeg */
    Twin->DecoderTight.JpegDec.client_data=(void *)Twin;
    jpeg_create_decompress(&Twin->DecoderTight.JpegDec);

    /* Changement de quelques paramètres dans la structure Jpeg */
    Twin->DecoderTight.JpegDec.out_color_space=JCS_RGB;

    Twin->DecoderTight.JpegSrcManager.init_source=JpegInitSource;
    Twin->DecoderTight.JpegSrcManager.fill_input_buffer=JpegFillInputBuffer;
    Twin->DecoderTight.JpegSrcManager.skip_input_data=JpegSkipInputData;
    Twin->DecoderTight.JpegSrcManager.resync_to_restart=jpeg_resync_to_restart;
    Twin->DecoderTight.JpegSrcManager.term_source=JpegTermSource;
    Twin->DecoderTight.JpegDec.src=&Twin->DecoderTight.JpegSrcManager;

    return TRUE;
}


/*****
    Libération des ressources allouées dans la structure DecoderTight
*****/

void CloseDecoderTight(struct Twin *Twin)
{
    FlushTightStream(Twin,15);
    jpeg_destroy_decompress(&Twin->DecoderTight.JpegDec);
}


/*****
    Décodage Tight
*****/

LONG DecoderTight(struct Twin *Twin, LONG x, LONG y, LONG w, LONG h)
{
    LONG Result;
    UBYTE Control=0;

    if((Result=Gal_WaitRead(Twin,(void *)&Control,sizeof(Control)))>0)
    {
        if(FlushTightStream(Twin,(ULONG)Control))
        {
            struct Display *disp=&Twin->Display;
            struct FrameBuffer *fb=&Twin->Display.FrameBuffer;

            Control>>=4;
            switch(Control)
            {
                case RFB_TIGHT_FILL:
                    {
                        BOOL IsBigEndian;
                        LONG PixelSize=GetPixelProperties(fb,&IsBigEndian);
                        ULONG Color=0;

                        if((Result=Util_ReadTwinColor(Twin,PixelSize,IsBigEndian,&Color))>0)
                        {
                            Color=Frb_ConvertColorForFrameBuffer(fb,Color,PixelSize);
                            Frb_FillRect(fb,x,y,w,h,Color);
                            disp->CurrentGfxPtr->FillRectFuncPtr(disp->CurrentGfxPtr,x,y,w,h,Color);
                        }
                    }
                    break;

                case RFB_TIGHT_JPEG:
                    if((Result=Jpeg(Twin,x,y,w,h))>0) disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,x,y,w,h);
                    break;

                default: /* Filter */
                    {
                        UBYTE FilterId=RFB_TIGHT_FILTER_COPY;

                        if(Control&RFB_TIGHT_FLG_EXPLICIT_FILTER) Result=Gal_WaitRead(Twin,(void *)&FilterId,sizeof(FilterId));

                        if(Result>0)
                        {
                            ULONG StreamId=Control&3;

                            switch(FilterId)
                            {
                                case RFB_TIGHT_FILTER_COPY:
                                    {
                                        BOOL IsBigEndian;
                                        LONG PixelSize=PixelSize=GetPixelProperties(fb,&IsBigEndian);
                                        LONG CountOfPixels=w*h;

                                        if((Result=PrepareTightData(Twin,StreamId,CountOfPixels*PixelSize))>0)
                                        {
                                            Frb_ConvertBufferForFrameBuffer(fb,Twin->DstBufferPtr,PixelSize,IsBigEndian,CountOfPixels);
                                            Frb_CopyRectFromBuffer(fb,Twin->DstBufferPtr,PixelSize,x,y,w,h);
                                        }
                                    }
                                    break;

                                case RFB_TIGHT_FILTER_PALETTE:
                                    if((Result=ReadPalette(Twin))>0)
                                    {
                                        ULONG SizeOfRect;

                                        if(Twin->DecoderTight.NPal<=2) SizeOfRect=h*((w+7)/8); else SizeOfRect=h*w;
                                        if((Result=PrepareTightData(Twin,StreamId,SizeOfRect))>0)
                                        {
                                            if(Twin->DecoderTight.NPal<=2) Frb_CopyBitmap(fb,Twin->DstBufferPtr,x,y,w,h,Twin->DecoderTight.Palette[0],Twin->DecoderTight.Palette[1]);
                                            else Frb_CopyChunky8(fb,Twin->DstBufferPtr,x,y,w,h,Twin->DecoderTight.Palette);
                                        }
                                    }
                                    break;

                                case RFB_TIGHT_FILTER_GRADIENT:
                                    Result=Gradient(Twin,StreamId,x,y,w,h);
                                    break;

                                default:
                                    Result=RESULT_CORRUPTED_DATA;
                                    break;
                            }

                            disp->CurrentGfxPtr->RefreshRectFuncPtr(disp->CurrentGfxPtr,x,y,w,h);
                        }
                    }
                    break;
            }
        }
    }

    return Result;
}


/*****
    Lecture d'une table d'index de couleurs
*****/

LONG ReadPalette(struct Twin *Twin)
{
    UBYTE Count;
    LONG Result=Gal_WaitRead(Twin,(void *)&Count,sizeof(Count));

    if(Result>0)
    {
        struct FrameBuffer *fb=&Twin->Display.FrameBuffer;
        BOOL IsBigEndian;
        ULONG ColorSize=GetPixelProperties(fb,&IsBigEndian);
        LONG ColorCount=(LONG)Count+1;
        LONG ReadLen=ColorCount*ColorSize;

        Twin->DecoderTight.NPal=ColorCount;

        Result=RESULT_NOT_ENOUGH_MEMORY;
        if(Util_PrepareDecodeBuffers(Twin,ReadLen,0))
        {
            void *SrcPtr=Twin->SrcBufferPtr;

            if((Result=Gal_WaitRead(Twin,SrcPtr,ReadLen))>0)
            {
                ULONG *ColorPtr=Twin->DecoderTight.Palette;

                Frb_ConvertBufferForFrameBuffer(fb,SrcPtr,ColorSize,IsBigEndian,ColorCount);
                while(--ColorCount>=0)
                {
                    SrcPtr=Util_ReadBufferColor(ColorSize,TRUE,SrcPtr,ColorPtr);
                    ColorPtr++;
                }
            }
        }
    }

    return Result;
}


/*****
    Décodage du format Gradient de Tight
*****/

LONG Gradient(struct Twin *Twin, ULONG StreamId, LONG x, LONG y, LONG w, LONG h)
{
    LONG Result;
    struct FrameBuffer *fb=&Twin->Display.FrameBuffer;
    BOOL IsBigEndian;
    ULONG PixelSize=GetPixelProperties(fb,&IsBigEndian);
    LONG PixelCount=w*h;

    if((Result=PrepareTightData(Twin,StreamId,PixelCount*PixelSize))>0)
    {
        void *Ptr=Twin->DstBufferPtr;

        /* Conversion du buffer de couleurs en big endian pour que ce soit plus rapide à traiter */
        if(!IsBigEndian) Util_ConvertLittleEndianPixelToBigEndian(Ptr,PixelSize,w*h);

        Result=RESULT_NOT_ENOUGH_MEMORY;
        if(Util_PrepareDecodeBuffers(Twin,(PixelCount+2*w)*sizeof(ULONG),0))
        {
            const struct PixelFormat *pf=fb->PixelFormat;
            LONG ShiftR=pf->RedShift,ShiftG=pf->GreenShift,ShiftB=pf->BlueShift;
            LONG MaxR=pf->RedMax,MaxG=pf->GreenMax,MaxB=pf->BlueMax;
            ULONG *prevrow=Twin->SrcBufferPtr,*currow=&prevrow[w],*ColorBuffer=&currow[w];
            ULONG *ColorPtr=ColorBuffer,*Tmp=prevrow;
            LONG j;

            for(j=w;--j>=0;) *(Tmp++)=0;

            for(j=h;--j>=0;)
            {
                UBYTE *curpix=(UBYTE *)currow,*prevpix=(UBYTE *)prevrow;
                LONG i;

                /* Swap des buffers */
                Tmp=currow; currow=prevrow; prevrow=Tmp;

                for(i=0;i<w;i++)
                {
                    ULONG Color;
                    LONG ProdR=(LONG)*(prevpix++);
                    LONG ProdG=(LONG)*(prevpix++);
                    LONG ProdB=(LONG)*(prevpix++);

                    if(i>0)
                    {
                        ProdR-=prevpix[0-6]-curpix[0-3];
                        ProdG-=prevpix[1-6]-curpix[1-3];
                        ProdB-=prevpix[2-6]-curpix[2-3];
                        if(ProdR<0) ProdR=0; else if(ProdR>MaxR) ProdR=MaxR;
                        if(ProdG<0) ProdG=0; else if(ProdG>MaxG) ProdG=MaxG;
                        if(ProdB<0) ProdB=0; else if(ProdB>MaxB) ProdB=MaxB;
                    }

                    Ptr=Util_ReadBufferColorBE(PixelSize,Ptr,&Color);
                    ProdR=((Color>>ShiftR)+ProdR)&MaxR;
                    ProdG=((Color>>ShiftG)+ProdG)&MaxG;
                    ProdB=((Color>>ShiftB)+ProdB)&MaxB;
                    *(curpix++)=(UBYTE)ProdR;
                    *(curpix++)=(UBYTE)ProdG;
                    *(curpix++)=(UBYTE)ProdB;

                    *(ColorPtr++)=(ProdR<<ShiftR)+(ProdG<<ShiftG)+(ProdB<<ShiftB);
                }
            }

            Frb_ConvertBufferForFrameBuffer(fb,ColorBuffer,sizeof(ULONG),TRUE,PixelCount);
            Frb_CopyRectFromBuffer(fb,ColorBuffer,sizeof(ULONG),x,y,w,h);

            Result=RESULT_SUCCESS;
        }
    }

    return Result;
}


/*****
    Décodage du format Jpeg de tight
*****/

LONG Jpeg(struct Twin *Twin, LONG x, LONG y, LONG w, LONG h)
{
    ULONG ReadLen=0;
    LONG Result=ReadLength(Twin,&ReadLen);

    if(Result>0)
    {
        Result=RESULT_NOT_ENOUGH_MEMORY;
        if(Util_PrepareDecodeBuffers(Twin,ReadLen,3*w))
        {
            if((Result=Gal_WaitRead(Twin,Twin->SrcBufferPtr,ReadLen))>0)
            {
                Twin->DecoderTight.JpegSrcManager.next_input_byte=Twin->SrcBufferPtr;
                Twin->DecoderTight.JpegSrcManager.bytes_in_buffer=ReadLen;

                if(jpeg_read_header(&Twin->DecoderTight.JpegDec,TRUE)==JPEG_HEADER_OK)
                {
                    struct FrameBuffer *fb=&Twin->Display.FrameBuffer;

                    jpeg_start_decompress(&Twin->DecoderTight.JpegDec);

                    for(;Twin->DecoderTight.JpegDec.output_scanline<Twin->DecoderTight.JpegDec.output_height && !Twin->DecoderTight.IsJpegError;y++)
                    {
                        JSAMPROW rowPointer=(JSAMPROW)Twin->DstBufferPtr;

                        jpeg_read_scanlines(&Twin->DecoderTight.JpegDec,&rowPointer,1);
                        Frb_CopyRectFromBuffer(fb,Twin->DstBufferPtr,3,x,y,w,1);
                    }

                    jpeg_finish_decompress(&Twin->DecoderTight.JpegDec);
                }
            }
        }
    }

    return Result;
}


/*****
    Lecture d'un paquet de données compressées et décompression dans le buffer tight
*****/

LONG PrepareTightData(struct Twin *Twin, ULONG StreamId, ULONG RectLen)
{
    LONG Result=RESULT_SUCCESS;
    ULONG ReadLen=RectLen;
    ULONG LocalSrcLen=0;

    /* On test la longueur minimale pour savoir si les données sont compressées */
    if(RectLen>=12)
    {
        Result=ReadLength(Twin,&ReadLen);
        LocalSrcLen=ReadLen;
    }

    if(Result>0)
    {
        Result=RESULT_NOT_ENOUGH_MEMORY;
        if(Util_PrepareDecodeBuffers(Twin,LocalSrcLen,RectLen))
        {
            if(RectLen>=12)
            {
                /* Lecture dans le buffer source */
                if((Result=Gal_WaitRead(Twin,Twin->SrcBufferPtr,ReadLen))>0)
                {
                    if(!Twin->DecoderTight.Flags[StreamId])
                    {
                        if(Util_ZLibInit(&Twin->DecoderTight.Stream[StreamId])==Z_OK) Twin->DecoderTight.Flags[StreamId]=TRUE;
                        else Result=RESULT_CORRUPTED_DATA;
                    }

                    /* Décompression dans le buffer destination */
                    if(Result>0)
                    {
                        if(Util_ZLibUnCompress(&Twin->DecoderTight.Stream[StreamId],Twin->SrcBufferPtr,LocalSrcLen,Twin->DstBufferPtr,RectLen)!=Z_OK) Result=RESULT_CORRUPTED_DATA;
                    }
                }
            }
            else
            {
                /* Lecture dans le buffer destination */
                Result=Gal_WaitRead(Twin,Twin->DstBufferPtr,ReadLen);
            }
        }
    }

    return Result;
}


/*****
    Fermeture des buffers alloués pour la ZLib
*****/

BOOL FlushTightStream(struct Twin *Twin, ULONG Control)
{
    BOOL IsSuccess=TRUE;
    ULONG i;

    for(i=0; i<4; i++)
    {
        if((Control&1) && Twin->DecoderTight.Flags[i])
        {
            if(Util_ZLibEnd(&Twin->DecoderTight.Stream[i])!=Z_OK) IsSuccess=FALSE;
            else Twin->DecoderTight.Flags[i]=FALSE;
        }
        Control>>=1;
    }

    return IsSuccess;
}


/*****
    Lecture d'une indication de longueur au format Tight
*****/

LONG ReadLength(struct Twin *Twin, ULONG *Len)
{
    LONG Result;
    BYTE Data;

    if((Result=Gal_WaitRead(Twin,(void *)&Data,sizeof(Data)))>0)
    {
        *Len=(ULONG)Data&0x7f;
        if(Data<0)
        {
            if((Result=Gal_WaitRead(Twin,(void *)&Data,sizeof(Data)))>0)
            {
                *Len|=((ULONG)Data&0x7f)<<7;

                if(Data<0)
                {
                    if((Result=Gal_WaitRead(Twin,(void *)&Data,sizeof(Data)))>0)
                    {
                        *Len|=((ULONG)(UBYTE)Data)<<14;
                    }
                }
            }
        }
    }

    return Result;
}


/*****
    Retourne le nombre d'octets utilisés pour un pixel courant
*****/

ULONG GetPixelProperties(struct FrameBuffer *fb, BOOL *IsBigEndian)
{
    const struct PixelFormat *pf=fb->PixelFormat;

    if(pf->Depth==24 && pf->RedMax==255 && pf->GreenMax==255 && pf->BlueMax==255)
    {
        *IsBigEndian=TRUE;
        return 3;
    }

    *IsBigEndian=(BOOL)pf->BigEndian;
    return fb->PixelFormatSize;
}


/*****
    Initialisation du format jpeg Tight
*****/

void JpegInitSource(j_decompress_ptr cinfo)
{
    struct Twin *Twin=(struct Twin *)cinfo->client_data;
    Twin->DecoderTight.IsJpegError=FALSE;
}


/*****
    Hook non utilisé
*****/

void JpegTermSource(j_decompress_ptr cinfo)
{
}


/*****
    Hook pour le format jpeg
*****/

boolean JpegFillInputBuffer(j_decompress_ptr cinfo)
{
    struct Twin *Twin=(struct Twin *)cinfo->client_data;
    Twin->DecoderTight.IsJpegError=TRUE;

    return FALSE;
}


/*****
    Hook pour la décompression jpeg
*****/

void JpegSkipInputData(j_decompress_ptr cinfo, long num_bytes)
{
    struct Twin *Twin=(struct Twin *)cinfo->client_data;
    if(num_bytes<0 || (size_t)num_bytes>Twin->DecoderTight.JpegSrcManager.bytes_in_buffer)
    {
        Twin->DecoderTight.IsJpegError=TRUE;
    }
    else
    {
        Twin->DecoderTight.JpegSrcManager.next_input_byte+=(size_t)num_bytes;
        Twin->DecoderTight.JpegSrcManager.bytes_in_buffer-=(size_t)num_bytes;
    }
}


/*****
    Hook en cas d'erreur
*****/

void JpegErrorExit(j_common_ptr cinfo)
{
    struct Twin *Twin=(struct Twin *)cinfo->client_data;
    Twin->DecoderTight.IsJpegError=TRUE;
}
