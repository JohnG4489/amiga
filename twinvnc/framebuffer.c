#include "system.h"
#include "framebuffer.h"


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    16-01-2016 (Seg)    Modification suite à la refonte de display.c
    14-01-2006 (Seg)    Optimisation des routines de conversion de format de pixel
    11-01-2006 (Seg)    Gestion du format du pixel serveur
    16-11-2005 (Seg)    Restructuration pour l'affichage
    15-04-2004 (Seg)    Gestion du frame buffer
*/


/***** Prototypes */

BOOL Frb_Prepare(struct FrameBuffer *, const struct PixelFormat *pf, LONG, LONG);
void Frb_Flush(struct FrameBuffer *);
void Frb_CalcColorMapTable(struct FrameBuffer *, const struct PixelFormat *);

void Frb_ConvertBufferForFrameBuffer(struct FrameBuffer *, void *, LONG, BOOL, LONG);
ULONG Frb_ConvertColorForFrameBuffer(struct FrameBuffer *, ULONG, LONG);
void Frb_WritePixel(struct FrameBuffer *, LONG, LONG, ULONG);
void Frb_FillScanLineRect(struct FrameBuffer *, LONG, LONG, LONG, LONG, LONG, LONG, ULONG);
void Frb_FillRect(struct FrameBuffer *, LONG, LONG, LONG, LONG, ULONG);
void Frb_CopyBitmap(struct FrameBuffer *, UBYTE *, LONG, LONG, LONG, LONG, ULONG, ULONG);
void Frb_CopyChunky8(struct FrameBuffer *, UBYTE *, LONG, LONG, LONG, LONG, ULONG *);
void Frb_CopyRectFromBuffer(struct FrameBuffer *, void *, ULONG, LONG, LONG, LONG, LONG);
void Frb_CopyRect(struct FrameBuffer *, LONG, LONG, LONG, LONG, LONG, LONG);
void Frb_ReadFromFrameBuffer(struct FrameBuffer *, LONG, LONG, LONG, LONG, void *);
void Frb_CopyRectMasked(struct FrameBuffer *, void *, UBYTE *, LONG, LONG, LONG, LONG, LONG);
BOOL Frb_ClipToFrameBuffer(struct FrameBuffer *, LONG *, LONG *, LONG *, LONG *);

void P_Frb_ConvertFrameBuffer(struct FrameBuffer *, ULONG, void *);
void P_Frb_Get16BitsConversionParameters(struct FrameBuffer *, LONG *, LONG *, LONG *, ULONG *, ULONG *, ULONG *);
void P_Frb_Get32BitsConversionParameters(struct FrameBuffer *, LONG *, LONG *, LONG *, ULONG *, ULONG *, ULONG *);
ULONG P_Frb_GetPixelFormatType(const struct PixelFormat *);
LONG P_Frb_GetCountOfBits(ULONG);


/*****
    Permet d'initialiser le frame buffer
*****/

BOOL Frb_Prepare(struct FrameBuffer *fb, const struct PixelFormat *pf, LONG Width, LONG Height)
{
    ULONG PixelFormatSize=(pf->BitPerPixel+7)/8;
    ULONG PixelBufferSize=(PixelFormatSize!=3?PixelFormatSize:sizeof(ULONG));
    BOOL IsConvert=FALSE,IsFlush=TRUE;

    if(fb->BufferPtr!=NULL)
    {
        if(fb->Width==Width && fb->Height==Height)
        {
            if(fb->PixelBufferSize==PixelBufferSize) IsFlush=FALSE;
            else IsConvert=TRUE;
        }
    }

    if(IsFlush)
    {
        ULONG Size=Width*Height*PixelBufferSize;
        void *NewBuffer=Sys_AllocMem(Size);

        if(NewBuffer!=NULL && IsConvert)
        {
            P_Frb_ConvertFrameBuffer(fb,PixelBufferSize,NewBuffer);
        }

        Frb_Flush(fb);
        if(NewBuffer==NULL) NewBuffer=Sys_AllocMem(Size);

        fb->Width=Width;
        fb->Height=Height;
        fb->BufferPtr=NewBuffer;
        fb->PixelBufferSize=PixelBufferSize;
    }

    fb->PixelFormat=pf;
    fb->PixelFormatSize=PixelFormatSize;

    /* Définition du type du pixel pour aiguiller les routines de conversion */
    fb->PixelFormatType=P_Frb_GetPixelFormatType(pf);

    /* Définition des paramètres servant à la conversion des pixels */
    switch(fb->PixelBufferSize)
    {
        default:
            fb->ConvPixShiftR=0;
            fb->ConvPixShiftG=0;
            fb->ConvPixShiftB=0;
            fb->ConvPixMaskR=255;
            fb->ConvPixMaskG=255;
            fb->ConvPixMaskB=255;
            break;

        case 2:
            P_Frb_Get16BitsConversionParameters(fb,
                &fb->ConvPixShiftR,&fb->ConvPixShiftG,&fb->ConvPixShiftB,
                &fb->ConvPixMaskR,&fb->ConvPixMaskG,&fb->ConvPixMaskB);
            break;

        case 4:
            P_Frb_Get32BitsConversionParameters(fb,
                &fb->ConvPixShiftR,&fb->ConvPixShiftG,&fb->ConvPixShiftB,
                &fb->ConvPixMaskR,&fb->ConvPixMaskG,&fb->ConvPixMaskB);
            break;
    }

    if(fb->BufferPtr!=NULL) return TRUE;

    return FALSE;
}


/*****
    Libération du frame buffer
*****/

void Frb_Flush(struct FrameBuffer *fb)
{
    if(fb->BufferPtr!=NULL)
    {
        //Sys_Printf("FlushFrameBuffer: Ptr=%08lx\n",(ULONG)fb->BufferPtr);
        Sys_FreeMem(fb->BufferPtr);
        fb->BufferPtr=NULL;
        fb->Width=0;
        fb->Height=0;
   }
}


/*****
    Permet de calculer une table de couleur en fonction du pixel format
*****/

void Frb_CalcColorMapTable(struct FrameBuffer *fb, const struct PixelFormat *pf)
{
    LONG i,r=0,g=0,b=0;
    LONG RedMax=pf->RedMax,GreenMax=pf->GreenMax,BlueMax=pf->BlueMax;
    LONG RedShift=pf->RedShift,GreenShift=pf->GreenShift,BlueShift=pf->BlueShift;
    LONG Count=(RedMax+1)*(GreenMax+1)*(BlueMax+1);

    if(Count>256)
    {
        RedMax=7;
        GreenMax=7;
        BlueMax=3;
        RedShift=5;
        GreenShift=2;
        BlueShift=0;
        Count=(RedMax+1)*(GreenMax+1)*(BlueMax+1);
    }

    fb->ColorCount=Count;

    for(i=0;i<Count; i++)
    {
        LONG RGB8,R32,G32,B32;

        if(b>BlueMax) {b=0; g++;}
        if(g>GreenMax) {g=0; r++;}
        RGB8=(r<<RedShift)+(g<<GreenShift)+(b<<BlueShift);

        R32=(RedMax!=0)?(r*255/RedMax)&0xff:0;
        G32=(GreenMax!=0)?(g*255/GreenMax)&0xff:0;
        B32=(BlueMax!=0)?(b*255/BlueMax)&0xff:0;

        fb->ColorMapRGB32[RGB8]=(R32<<16)+(G32<<8)+B32;
        b++;
    }
}


/*****
    Conversion d'un buffer pour être utilisé avec le FrameBuffer
*****/

void Frb_ConvertBufferForFrameBuffer(struct FrameBuffer *fb, void *BufferPtr, LONG PixelSize, BOOL IsBigEndian, LONG CountOfPixels)
{
    switch(fb->PixelFormatType)
    {
        default:
            break;

        case PIXEL_R5G6B5:
            if(!IsBigEndian)
            {
                switch(PixelSize)
                {
                    default:
                        break;

                    case 2:
                        {
                            UWORD *Ptr=(UWORD *)BufferPtr;

                            while(--CountOfPixels>=0)
                            {
                                UWORD Color=(ULONG)*Ptr;
                                *(Ptr++)=(Color>>8)+(Color<<8);
                            }
                        }
                        break;
                }
            }
            break;

        case PIXEL_R8G8B8:
            if(!IsBigEndian)
            {
                switch(PixelSize)
                {
                    default:
                        break;

                    case 3:
                        {
                            UBYTE *Ptr=(UBYTE *)BufferPtr;

                            while(--CountOfPixels>=0)
                            {
                                ULONG Color=(ULONG)Ptr[0];
                                Ptr[0]=Ptr[2];
                                Ptr[2]=(UBYTE)Color;
                                Ptr+=3;
                            }
                        }
                        break;

                    case 4:
                        {
                            ULONG *Ptr=(ULONG *)BufferPtr;

                            while(--CountOfPixels>=0)
                            {
                                ULONG Color=*Ptr;
                                *(Ptr++)=(Color>>24)+((Color>>8)&0x0000ff00)+((Color<<8)&0x00ff0000)+(Color<<24);
                            }
                        }
                        break;
                }
            }
            break;

        case PIXEL_RxGyBz:
            {
                LONG ShiftR=fb->ConvPixShiftR;
                LONG ShiftG=fb->ConvPixShiftG;
                LONG ShiftB=fb->ConvPixShiftB;
                ULONG MaskR=fb->ConvPixMaskR;
                ULONG MaskG=fb->ConvPixMaskG;
                ULONG MaskB=fb->ConvPixMaskB;

                switch(PixelSize)
                {
                    default:
                        break;

                    case 2:
                        {
                            UWORD *Ptr=(UWORD *)BufferPtr;

                            if(IsBigEndian)
                            {
                                while(--CountOfPixels>=0)
                                {
                                    ULONG Color=(ULONG)*Ptr;
                                    ULONG Red  =ShiftR>0?(Color<<ShiftR):(Color>>-ShiftR);
                                    ULONG Green=ShiftG>0?(Color<<ShiftG):(Color>>-ShiftG);
                                    ULONG Blue =ShiftB>0?(Color<<ShiftB):(Color>>-ShiftB);
                                    *(Ptr++)=(Red&MaskR)+(Green&MaskG)+(Blue&MaskB);
                                }
                            }
                            else
                            {
                                while(--CountOfPixels>=0)
                                {
                                    ULONG Red,Green,Blue;
                                    UWORD Color=*Ptr;
                                    Color=(Color>>8)+(Color<<8);
                                    Red  =ShiftR>0?(Color<<ShiftR):(Color>>-ShiftR);
                                    Green=ShiftG>0?(Color<<ShiftG):(Color>>-ShiftG);
                                    Blue =ShiftB>0?(Color<<ShiftB):(Color>>-ShiftB);
                                    *(Ptr++)=(Red&MaskR)+(Green&MaskG)+(Blue&MaskB);
                                }
                            }
                        }
                        break;

                    case 3:
                        {
                            UBYTE *Ptr=(UBYTE *)BufferPtr;

                            if(IsBigEndian)
                            {
                                while(--CountOfPixels>=0)
                                {
                                    ULONG Color=((ULONG)Ptr[0]<<16)+((ULONG)Ptr[1]<<8)+(ULONG)Ptr[2];
                                    ULONG Red  =ShiftR>0?(Color<<ShiftR):(Color>>-ShiftR);
                                    ULONG Green=ShiftG>0?(Color<<ShiftG):(Color>>-ShiftG);
                                    ULONG Blue =ShiftB>0?(Color<<ShiftB):(Color>>-ShiftB);
                                    *(Ptr++)=(UBYTE)((Red&MaskR)>>16);
                                    *(Ptr++)=(UBYTE)((Green&MaskG)>>8);
                                    *(Ptr++)=(UBYTE)(Blue&MaskB);
                                }
                            }
                            else
                            {
                                while(--CountOfPixels>=0)
                                {
                                    ULONG Color=((ULONG)Ptr[2]<<16)+((ULONG)Ptr[1]<<8)+(ULONG)Ptr[0];
                                    ULONG Red  =ShiftR>0?(Color<<ShiftR):(Color>>-ShiftR);
                                    ULONG Green=ShiftG>0?(Color<<ShiftG):(Color>>-ShiftG);
                                    ULONG Blue =ShiftB>0?(Color<<ShiftB):(Color>>-ShiftB);
                                    *(Ptr++)=(UBYTE)((Red&MaskR)>>16);
                                    *(Ptr++)=(UBYTE)((Green&MaskG)>>8);
                                    *(Ptr++)=(UBYTE)(Blue&MaskB);
                                }
                            }
                        }
                        break;

                    case 4:
                        {
                            ULONG *Ptr=(ULONG *)BufferPtr;

                            if(IsBigEndian)
                            {
                                while(--CountOfPixels>=0)
                                {
                                    ULONG Color=*Ptr;
                                    ULONG Red  =ShiftR>0?(Color<<ShiftR):(Color>>-ShiftR);
                                    ULONG Green=ShiftG>0?(Color<<ShiftG):(Color>>-ShiftG);
                                    ULONG Blue =ShiftB>0?(Color<<ShiftB):(Color>>-ShiftB);
                                    *(Ptr++)=(Red&MaskR)+(Green&MaskG)+(Blue&MaskB);
                                }
                            }
                            else
                            {
                                while(--CountOfPixels>=0)
                                {
                                    ULONG Red,Green,Blue;
                                    ULONG Color=*Ptr;
                                    Color=(Color>>24)+((Color>>8)&0x0000ff00)+((Color<<8)&0x00ff0000)+(Color<<24);
                                    Red  =ShiftR>0?(Color<<ShiftR):(Color>>-ShiftR);
                                    Green=ShiftG>0?(Color<<ShiftG):(Color>>-ShiftG);
                                    Blue =ShiftB>0?(Color<<ShiftB):(Color>>-ShiftB);
                                    *(Ptr++)=(Red&MaskR)+(Green&MaskG)+(Blue&MaskB);
                                }
                            }
                        }
                        break;
                }
            }
            break;
    }
}


/*****
    Retourne la couleur adaptée au FrameBuffer
*****/

ULONG Frb_ConvertColorForFrameBuffer(struct FrameBuffer *fb, ULONG Color, LONG ColorSize)
{
    if(fb->PixelFormatType==PIXEL_RxGyBz)
    {
        LONG ShiftR=fb->ConvPixShiftR;
        LONG ShiftG=fb->ConvPixShiftG;
        LONG ShiftB=fb->ConvPixShiftB;
        ULONG Red,Green,Blue;

        switch(ColorSize)
        {
            default:
                break;

            case 2:
                Red  =ShiftR>0?(Color<<ShiftR):(Color>>-ShiftR);
                Green=ShiftG>0?(Color<<ShiftG):(Color>>-ShiftG);
                Blue =ShiftB>0?(Color<<ShiftB):(Color>>-ShiftB);
                Color=(Red&fb->ConvPixMaskR)+(Green&fb->ConvPixMaskG)+(Blue&fb->ConvPixMaskB);
                break;

            case 3:
            case 4:
                Red  =ShiftR>0?(Color<<ShiftR):(Color>>-ShiftR);
                Green=ShiftG>0?(Color<<ShiftG):(Color>>-ShiftG);
                Blue =ShiftB>0?(Color<<ShiftB):(Color>>-ShiftB);
                Color=(Red&fb->ConvPixMaskR)+(Green&fb->ConvPixMaskG)+(Blue&fb->ConvPixMaskB);
                break;
        }
    }

    return Color;
}


/*****
    Tracé d'un point dans le frame buffer
*****/

void Frb_WritePixel(struct FrameBuffer *fb, LONG x, LONG y, ULONG Color)
{
    LONG Offset=x+y*fb->Width;

    switch(fb->PixelBufferSize)
    {
        default:
            ((UBYTE *)fb->BufferPtr)[Offset]=(UBYTE)Color;
            break;

        case 2:
            ((UWORD *)fb->BufferPtr)[Offset]=(UWORD)Color;
            break;

        case 4:
            ((ULONG *)fb->BufferPtr)[Offset]=Color;
            break;
    }
}


/*****
    Remplissage de lignes dans le frame buffer
*****/

void Frb_FillScanLineRect(struct FrameBuffer *fb, LONG x, LONG y, LONG w, LONG h, LONG Offset, LONG Len, ULONG Color)
{
    LONG posline=x+(y+Offset/w)*fb->Width;
    LONG posx=Offset%w;

    switch(fb->PixelBufferSize)
    {
        default:
            {
                UBYTE *Ptr=&((UBYTE *)fb->BufferPtr)[posline];

                while(--Len>=0)
                {
                    if(posx>=w) {posx=0; Ptr=&Ptr[fb->Width];}
                    Ptr[posx++]=(UBYTE)Color;
                }
            }
            break;

        case 2:
            {
                UWORD *Ptr=&((UWORD *)fb->BufferPtr)[posline];

                while(--Len>=0)
                {
                    if(posx>=w) {posx=0; Ptr=&Ptr[fb->Width];}
                    Ptr[posx++]=(UWORD)Color;
                }
            }
            break;

        case 4:
            {
                ULONG *Ptr=&((ULONG *)fb->BufferPtr)[posline];

                while(--Len>=0)
                {
                    if(posx>=w) {posx=0; Ptr=&Ptr[fb->Width];}
                    Ptr[posx++]=Color;
                }
            }
            break;
    }
}


/*****
    Remplissage d'un rectangle dans le frame buffer
*****/

void Frb_FillRect(struct FrameBuffer *fb, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
    LONG Offset=x+y*fb->Width;
    LONG DstMod=fb->Width-w;

    switch(fb->PixelBufferSize)
    {
        default:
            {
                UBYTE *Ptr=&((UBYTE *)fb->BufferPtr)[Offset];

                while(--h>=0)
                {
                    LONG i=w;
                    while(--i>=0) *(Ptr++)=(UBYTE)Color;
                    Ptr=&Ptr[DstMod];
                }
            }
            break;

        case 2:
            {
                UWORD *Ptr=&((UWORD *)fb->BufferPtr)[Offset];

                while(--h>=0)
                {
                    LONG i=w;
                    while(--i>=0) *(Ptr++)=(UWORD)Color;
                    Ptr=&Ptr[DstMod];
                }
            }
            break;

        case 4:
            {
                ULONG *Ptr=&((ULONG *)fb->BufferPtr)[Offset];

                while(--h>=0)
                {
                    LONG i=w;
                    while(--i>=0) *(Ptr++)=Color;
                    Ptr=&Ptr[DstMod];
                }
            }
            break;
    }
}


/*****
    Copie d'une bitmap dans le framebuffer
*****/

void Frb_CopyBitmap(struct FrameBuffer *fb, UBYTE *SrcBuffer, LONG x, LONG y, LONG w, LONG h, ULONG Color0, ULONG Color1)
{
    LONG Offset=x+y*fb->Width;
    LONG DstMod=fb->Width-w;

    switch(fb->PixelBufferSize)
    {
        default:
            {
                UBYTE *DstPtr=&((UBYTE *)fb->BufferPtr)[Offset];

                while(--h>=0)
                {
                    LONG wrest;

                    for(wrest=w; wrest>0; wrest-=8)
                    {
                        ULONG Data=(ULONG)*(SrcBuffer++);
                        LONG b=wrest;

                        if(b>8) b=8;
                        while(--b>=0)
                        {
                            *DstPtr++=(UBYTE)(Data&0x80?Color1:Color0);
                            Data<<=1;
                        }
                    }

                    DstPtr=&DstPtr[DstMod];
                }
            }
            break;

        case 2:
            {
                UWORD *DstPtr=&((UWORD *)fb->BufferPtr)[Offset];

                while(--h>=0)
                {
                    LONG wrest;

                    for(wrest=w; wrest>0; wrest-=8)
                    {
                        ULONG Data=(ULONG)*(SrcBuffer++);
                        LONG b=wrest;

                        if(b>8) b=8;
                        while(--b>=0)
                        {
                            *DstPtr++=(UWORD)(Data&0x80?Color1:Color0);
                            Data<<=1;
                        }
                    }

                    DstPtr=&DstPtr[DstMod];
                }
            }
            break;

        case 4:
            {
                ULONG *DstPtr=&((ULONG *)fb->BufferPtr)[Offset];

                while(--h>=0)
                {
                    LONG wrest;

                    for(wrest=w; wrest>0; wrest-=8)
                    {
                        ULONG Data=(ULONG)*(SrcBuffer++);
                        LONG b=wrest;

                        if(b>8) b=8;
                        while(--b>=0)
                        {
                            *DstPtr++=(Data&0x80?Color1:Color0);
                            Data<<=1;
                        }
                    }

                    DstPtr=&DstPtr[DstMod];
                }
            }
            break;
    }
}


/*****
    Copie d'une chunkymap dans le framebuffer
*****/

void Frb_CopyChunky8(struct FrameBuffer *fb, UBYTE *SrcBuffer, LONG x, LONG y, LONG w, LONG h, ULONG *ColorTable)
{
    LONG Offset=x+y*fb->Width;
    LONG DstMod=fb->Width-w;

    switch(fb->PixelBufferSize)
    {
        default:
            {
                UBYTE *DstPtr=&((UBYTE *)fb->BufferPtr)[Offset];

                while(--h>=0)
                {
                    LONG i=w;
                    while(--i>=0) *(DstPtr++)=ColorTable[*(SrcBuffer++)];
                    DstPtr=&DstPtr[DstMod];
                }
            }
            break;

        case 2:
            {
                UWORD *DstPtr=&((UWORD *)fb->BufferPtr)[Offset];

                while(--h>=0)
                {
                    LONG i=w;
                    while(--i>=0) *(DstPtr++)=ColorTable[*(SrcBuffer++)];
                    DstPtr=&DstPtr[DstMod];
                }
            }
            break;

        case 4:
            {
                ULONG *DstPtr=&((ULONG *)fb->BufferPtr)[Offset];

                while(--h>=0)
                {
                    LONG i=w;
                    while(--i>=0) *(DstPtr++)=ColorTable[*(SrcBuffer++)];
                    DstPtr=&DstPtr[DstMod];
                }
            }
            break;
    }
}


/*****
    Copie d'un bloc dans le framebuffer
*****/

void Frb_CopyRectFromBuffer(struct FrameBuffer *fb, void *SrcBuffer, ULONG PixelSize, LONG xdst, LONG ydst, LONG wdst, LONG hdst)
{
    LONG Offset=xdst+ydst*fb->Width;
    LONG DstMod=fb->Width-wdst;

    switch(PixelSize)
    {
        default:
            {
                UBYTE *DstPtr=&((UBYTE *)fb->BufferPtr)[Offset];
                UBYTE *SrcPtr=(UBYTE *)SrcBuffer;

                while(--hdst>=0)
                {
                    LONG i=wdst;
                    while(--i>=0) *(DstPtr++)=*(SrcPtr++);
                    DstPtr=&DstPtr[DstMod];
                }
            }
            break;

        case 2:
            {
                UWORD *DstPtr=&((UWORD *)fb->BufferPtr)[Offset];
                UWORD *SrcPtr=(UWORD *)SrcBuffer;

                while(--hdst>=0)
                {
                    LONG i=wdst;
                    while(--i>=0) *(DstPtr++)=*(SrcPtr++);
                    DstPtr=&DstPtr[DstMod];
                }
            }
            break;

        case 3:
            switch(fb->PixelBufferSize)
            {
                case 2:
                    {
                        UWORD *DstPtr=&((UWORD *)fb->BufferPtr)[Offset];
                        UBYTE *SrcPtr=(UBYTE *)SrcBuffer;

                        while(--hdst>=0)
                        {
                            LONG i=wdst;
                            while(--i>=0)
                            {
                                ULONG Red=(ULONG)*(SrcPtr++);
                                ULONG Green=(ULONG)*(SrcPtr++);
                                ULONG Blue=(ULONG)*(SrcPtr++);
                                *(DstPtr++)=((Red<<8)&0xf800)+((Green<<3)&0x07e0)+((Blue>>3)&0x001f);
                            }
                            DstPtr=&DstPtr[DstMod];
                        }
                    }
                    break;

                case 4:
                    {
                        ULONG *DstPtr=&((ULONG *)fb->BufferPtr)[Offset];
                        UBYTE *SrcPtr=(UBYTE *)SrcBuffer;

                        while(--hdst>=0)
                        {
                            LONG i=wdst;
                            while(--i>=0)
                            {
                                ULONG Color=(ULONG)*(SrcPtr++);
                                Color=(Color<<8)+(ULONG)*(SrcPtr++);
                                Color=(Color<<8)+(ULONG)*(SrcPtr++);
                                *(DstPtr++)=Color;
                            }
                            DstPtr=&DstPtr[DstMod];
                        }
                    }
                    break;
            }
            break;

        case 4:
            switch(fb->PixelBufferSize)
            {
                default:
                    {
                        UBYTE *DstPtr=&((UBYTE *)fb->BufferPtr)[Offset];
                        ULONG *SrcPtr=(ULONG *)SrcBuffer;

                        while(--hdst>=0)
                        {
                            LONG i=wdst;
                            while(--i>=0) *(DstPtr++)=(UBYTE)*(SrcPtr++);
                            DstPtr=&DstPtr[DstMod];
                        }
                    }
                    break;

                case 2:
                    {
                        UWORD *DstPtr=&((UWORD *)fb->BufferPtr)[Offset];
                        ULONG *SrcPtr=(ULONG *)SrcBuffer;

                        while(--hdst>=0)
                        {
                            LONG i=wdst;
                            while(--i>=0) *(DstPtr++)=(UWORD)*(SrcPtr++);
                            DstPtr=&DstPtr[DstMod];
                        }
                    }
                    break;

                case 4:
                    {
                        ULONG *DstPtr=&((ULONG *)fb->BufferPtr)[Offset];
                        ULONG *SrcPtr=(ULONG *)SrcBuffer;

                        while(--hdst>=0)
                        {
                            LONG i=wdst;
                            while(--i>=0) *(DstPtr++)=*(SrcPtr++);
                            DstPtr=&DstPtr[DstMod];
                        }
                    }
                    break;
            }
            break;
    }
}


/*****
    Déplacement d'un bloc dans le framebuffer
*****/

void Frb_CopyRect(struct FrameBuffer *fb, LONG xsrc, LONG ysrc, LONG xdst, LONG ydst, LONG wdst, LONG hdst)
{
    UBYTE *SrcPtr=&((UBYTE *)fb->BufferPtr)[(xsrc+ysrc*fb->Width)*fb->PixelBufferSize];
    UBYTE *DstPtr=&((UBYTE *)fb->BufferPtr)[(xdst+ydst*fb->Width)*fb->PixelBufferSize];
    LONG BytePerRow=fb->Width*fb->PixelBufferSize;
    LONG Len=(LONG)(wdst*fb->PixelBufferSize);

    if(SrcPtr>=DstPtr)
    {
        while(--hdst>=0)
        {
            LONG j=Len;
            UBYTE *TmpSrcPtr=SrcPtr,*TmpDstPtr=DstPtr;

            while(--j>=0) *(TmpDstPtr++)=*(TmpSrcPtr++);
            SrcPtr+=BytePerRow;
            DstPtr+=BytePerRow;
        }
    }
    else
    {
        LONG Offset=(wdst+hdst*fb->Width)*fb->PixelBufferSize;

        SrcPtr=&SrcPtr[Offset];
        DstPtr=&DstPtr[Offset];
        while(--hdst>=0)
        {
            LONG j=Len;
            UBYTE *TmpSrcPtr=(SrcPtr-=BytePerRow),*TmpDstPtr=(DstPtr-=BytePerRow);

            while(--j>=0) *(--TmpDstPtr)=*(--TmpSrcPtr);
        }
    }
}


/*****
    Lecture d'un rectangle dans le framebuffer
*****/

void Frb_ReadFromFrameBuffer(struct FrameBuffer *fb, LONG x, LONG y, LONG w, LONG h, void *DstPtr)
{
    LONG SrcMod=fb->Width-w;

    switch(fb->PixelBufferSize)
    {
        default:
            {
                UBYTE *SrcPtr=&((UBYTE *)fb->BufferPtr)[x+y*fb->Width];
                UBYTE *Dst2Ptr=(UBYTE *)DstPtr;

                while(--h>=0)
                {
                    LONG i=w;
                    while(--i>=0) *(Dst2Ptr++)=*(SrcPtr++);
                    SrcPtr=&SrcPtr[SrcMod];
                }
            }
            break;

        case 2:
            {
                UWORD *SrcPtr=&((UWORD *)fb->BufferPtr)[x+y*fb->Width];
                UWORD *Dst2Ptr=(UWORD *)DstPtr;

                while(--h>=0)
                {
                    LONG i=w;
                    while(--i>=0) *(Dst2Ptr++)=*(SrcPtr++);
                    SrcPtr=&SrcPtr[SrcMod];
                }
            }
            break;

        case 4:
            {
                ULONG *SrcPtr=&((ULONG *)fb->BufferPtr)[x+y*fb->Width];
                ULONG *Dst2Ptr=(ULONG *)DstPtr;

                while(--h>=0)
                {
                    LONG i=w;
                    while(--i>=0) *(Dst2Ptr++)=*(SrcPtr++);
                    SrcPtr=&SrcPtr[SrcMod];
                }
            }
            break;
    }
}


/*****
    Copy d'un rectangle en suivant un mask d'affichage
*****/

void Frb_CopyRectMasked(struct FrameBuffer *fb, void *Data, UBYTE *Mask, LONG Mod, LONG x, LONG y, LONG w, LONG h)
{
    LONG OffsetBuffer=x+y*fb->Width;

    switch(fb->PixelBufferSize)
    {
        default:
            {
                UBYTE *NewBuffer=&((UBYTE *)fb->BufferPtr)[OffsetBuffer];

                while(--h>=0)
                {
                    ULONG i;
                    for(i=0;i<w;i++) if(Mask[i]) NewBuffer[i]=((UBYTE *)Data)[i];
                    NewBuffer=&NewBuffer[fb->Width];
                    Data=(void *)&((UBYTE *)Data)[Mod];
                    Mask=&Mask[Mod];
                }
            }
            break;

        case 2:
            {
                UWORD *NewBuffer=&((UWORD *)fb->BufferPtr)[OffsetBuffer];

                while(--h>=0)
                {
                    ULONG i;
                    for(i=0;i<w;i++) if(Mask[i]) NewBuffer[i]=((UWORD *)Data)[i];
                    NewBuffer=&NewBuffer[fb->Width];
                    Data=(void *)&((UWORD *)Data)[Mod];
                    Mask=&Mask[Mod];
                }
            }
            break;

        case 4:
            {
                ULONG *NewBuffer=&((ULONG *)fb->BufferPtr)[OffsetBuffer];

                while(--h>=0)
                {
                    ULONG i;
                    for(i=0;i<w;i++) if(Mask[i]) NewBuffer[i]=((ULONG *)Data)[i];
                    NewBuffer=&NewBuffer[fb->Width];
                    Data=(void *)&((ULONG *)Data)[Mod];
                    Mask=&Mask[Mod];
                }
            }
            break;
    }
}


/*****
    Clipping en fonction de la taille du framebuffer
*****/

BOOL Frb_ClipToFrameBuffer(struct FrameBuffer *fb, LONG *x, LONG *y, LONG *w, LONG *h)
{
    LONG x2=*x+*w;
    LONG y2=*y+*h;
    LONG c2=fb->Width,l2=fb->Height;

    if(*x<0) *x=0;
    if(*y<0) *y=0;
    if(*x>c2) *x=c2;
    if(*y>l2) *y=l2;
    if(x2<0) x2=0;
    if(y2<0) y2=0;
    if(x2>c2) x2=c2;
    if(y2>l2) y2=l2;
    *w=x2-*x;
    *h=y2-*y;
    if(*w>0 && *h>0) return TRUE;

    return FALSE;
}


/*****
    Conversion du framebuffer
*****/

void P_Frb_ConvertFrameBuffer(struct FrameBuffer *fb, ULONG PixelBufferSize, void *NewBuffer)
{
    LONG Count=fb->Width*fb->Height;

    switch(PixelBufferSize)
    {
        default:
            {
                //UBYTE *DstPtr=(UBYTE *)NewBuffer;

                switch(fb->PixelBufferSize)
                {
                    case 2:
                        break;

                    case 4:
                        break;
                }
            }
            break;

        case 2:
            {
                UWORD *DstPtr=(UWORD *)NewBuffer;

                switch(fb->PixelBufferSize)
                {
                    default:
                        {
                            UBYTE *SrcPtr=(UBYTE *)fb->BufferPtr;

                            while(--Count>=0)
                            {
                                ULONG Color=fb->ColorMapRGB32[*(SrcPtr++)];
                                *(DstPtr++)=(UWORD)(((Color&0xf80000)>>8)+((Color&0x00fc00)>>5)+((Color&0x0000f8)>>3));
                            }
                        }
                        break;

                    case 4:
                        {
                            ULONG *SrcPtr=(ULONG *)fb->BufferPtr;

                            while(--Count>=0)
                            {
                                ULONG Color=*(SrcPtr++);
                                *(DstPtr++)=(UWORD)(((Color&0xf80000)>>8)+((Color&0x00fc00)>>5)+((Color&0x0000f8)>>3));
                            }
                        }
                        break;
                }
            }
            break;

        case 4:
            {
                ULONG *DstPtr=(ULONG *)NewBuffer;

                switch(fb->PixelBufferSize)
                {
                    default:
                        {
                            UBYTE *SrcPtr=(UBYTE *)fb->BufferPtr;

                            while(--Count>=0)
                            {
                                *(DstPtr++)=fb->ColorMapRGB32[*(SrcPtr++)];
                            }
                        }
                        break;

                    case 2:
                        {
                            UWORD *SrcPtr=(UWORD *)fb->BufferPtr;

                            while(--Count>=0)
                            {
                                ULONG Color=(ULONG)*(SrcPtr++);
                                *(DstPtr++)=(ULONG)(UBYTE)(Color<<3)+((Color&0x07e0)<<5)+((Color&0xf800)<<8);
                            }
                        }
                        break;
                }
            }
            break;
    }
}


/*****
    Récupération des paramètres de conversion 16 bits
*****/

void P_Frb_Get16BitsConversionParameters(struct FrameBuffer *fb, LONG *ShiftR, LONG *ShiftG, LONG *ShiftB, ULONG *MaskR, ULONG *MaskG, ULONG *MaskB)
{
    const struct PixelFormat *pf=fb->PixelFormat;
    LONG CountOfBitR=P_Frb_GetCountOfBits((ULONG)pf->RedMax);
    LONG CountOfBitG=P_Frb_GetCountOfBits((ULONG)pf->GreenMax);
    LONG CountOfBitB=P_Frb_GetCountOfBits((ULONG)pf->BlueMax);

    *ShiftR=16-CountOfBitR-(LONG)pf->RedShift;
    *ShiftG=11-CountOfBitG-(LONG)pf->GreenShift;
    *ShiftB=5-CountOfBitB-(LONG)pf->BlueShift;

    if(CountOfBitR>5) CountOfBitR=5;
    if(CountOfBitG>6) CountOfBitG=6;
    if(CountOfBitB>5) CountOfBitB=5;
    *MaskR=((1<<CountOfBitR)-1)<<(16-CountOfBitR);
    *MaskG=((1<<CountOfBitG)-1)<<(11-CountOfBitG);
    *MaskB=((1<<CountOfBitB)-1)<<(5-CountOfBitB);
}


/*****
    Récupération des paramètres de conversion 32 bits
*****/

void P_Frb_Get32BitsConversionParameters(struct FrameBuffer *fb, LONG *ShiftR, LONG *ShiftG, LONG *ShiftB, ULONG *MaskR, ULONG *MaskG, ULONG *MaskB)
{
    const struct PixelFormat *pf=fb->PixelFormat;
    LONG CountOfBitR=P_Frb_GetCountOfBits((ULONG)pf->RedMax);
    LONG CountOfBitG=P_Frb_GetCountOfBits((ULONG)pf->GreenMax);
    LONG CountOfBitB=P_Frb_GetCountOfBits((ULONG)pf->BlueMax);

    *ShiftR=24-CountOfBitR-(LONG)pf->RedShift;
    *ShiftG=16-CountOfBitG-(LONG)pf->GreenShift;
    *ShiftB=8-CountOfBitB-(LONG)pf->BlueShift;

    if(CountOfBitR>8) CountOfBitR=8;
    if(CountOfBitG>8) CountOfBitG=8;
    if(CountOfBitB>8) CountOfBitB=8;
    *MaskR=((1<<CountOfBitR)-1)<<(24-CountOfBitR);
    *MaskG=((1<<CountOfBitG)-1)<<(16-CountOfBitG);
    *MaskB=((1<<CountOfBitB)-1)<<(8-CountOfBitB);
}


/*****
    Détermine le type du pixel retourné par le serveur, à partir des données
    fournies par le PixelFormat.
*****/

ULONG P_Frb_GetPixelFormatType(const struct PixelFormat *pf)
{
    ULONG Type=PIXEL_LUT;
    ULONG PixelSize=(pf->BitPerPixel+7)/8;

    if(PixelSize==2 &&
       pf->RedMax==31 && pf->GreenMax==63 && pf->BlueMax==31 &&
       pf->RedShift==11 && pf->GreenShift==5 && pf->BlueShift==0)
    {
        Type=PIXEL_R5G6B5;
    }
    else if((PixelSize==3 || PixelSize==4) &&
       pf->RedMax==255 && pf->GreenMax==255 && pf->BlueMax==255 &&
       pf->RedShift==16 && pf->GreenShift==8 && pf->BlueShift==0)
    {
        Type=PIXEL_R8G8B8;
    }
    else if(PixelSize>1)
    {
        Type=PIXEL_RxGyBz;
    }

    return Type;
}


/*****
    Calcule le nombre de bits nécessaires pour stocker la valeur Value
*****/

LONG P_Frb_GetCountOfBits(ULONG Value)
{
    LONG Count;

    for(Count=0;Count<32 && Value>(1<<Count);Count++);

    return Count;
}
