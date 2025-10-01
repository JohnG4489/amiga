#include "system.h"
#include "util.h"
#include "general.h"
#include <zlib.h>


/*
    02-07-2017 (Seg)    Modification de l'api 64 bits
    20-03-2016 (Seg)    Utilisation de la codesets
    15-03-2016 (Seg)    Intégration du code de encoding.c ici
    22-02-2016 (Seg)    Quelques fonctions utiles
*/


#define DECODE_BUFFER_SRC_MAX_SIZE  40000
#define DECODE_BUFFER_DST_MAX_SIZE  40000


/***** Prototypes */
BOOL Util_PrepareDecodeBuffers(struct Twin *, ULONG, ULONG);
void Util_FlushDecodeBuffers(struct Twin *, BOOL);
int Util_ZLibUnCompress(z_stream *, void *, ULONG, void *, ULONG);
int Util_ZLibInit(z_stream *);
int Util_ZLibEnd(z_stream *);

LONG Util_ReadTwinColor(struct Twin *, LONG, BOOL, ULONG *);
LONG Util_WaitReadStringUTF8(struct Twin *, char *, LONG);
LONG Util_WaitWriteStringUTF8(struct Twin *, const char *);

ULONG Util_ReadBuffer8(UBYTE **, UBYTE *);
ULONG Util_ReadBuffer16(UBYTE **, UBYTE *);
ULONG Util_ReadBuffer32(UBYTE **, UBYTE *);
void Util_ReadBufferStringUTF8(UBYTE **, UBYTE *, char *, LONG);
void *Util_ReadBufferColor(LONG, BOOL, UBYTE *, ULONG *);
void *Util_ReadBufferColorBE(LONG, UBYTE *, ULONG *);
void *Util_ReadBufferData(UBYTE *, UBYTE *, LONG);
void Util_ConvertLittleEndianPixelToBigEndian(void *, ULONG, LONG);

void Util_ANSItoUTF8(const char *, char *, LONG);
void Util_UTF8toANSI(const char *, char *, LONG);

ULONG Util_Time64ToAmigaTime(const ULONG [2]);
void Util_AmigaTimeToTime64(ULONG, ULONG [2]);

void Util_ShiftL64(ULONG [2], BOOL *);
void Util_ShiftR64(ULONG [2], BOOL *);
void Util_DivU64(ULONG [2], const ULONG [2], ULONG [2]);
void Util_Add64(const ULONG [2], ULONG [2]);
void Util_MulU64(const ULONG [2], ULONG [2]);

void Util_FlushSrcBuffer(struct Twin *);
void Util_FlushDstBuffer(struct Twin *);


/*****
    Agrandissement si nécessaire des buffers source et destination
*****/

BOOL Util_PrepareDecodeBuffers(struct Twin *Twin, ULONG SizeOfSrc, ULONG SizeOfDst)
{
    BOOL IsSuccess=TRUE;

    /* On regarde si le buffer source fait déjà la bonne taille */
    if(SizeOfSrc>0 && Twin->SizeOfSrcBuffer<SizeOfSrc)
    {
        Util_FlushSrcBuffer(Twin);
        if((Twin->SrcBufferPtr=Sys_AllocMem(SizeOfSrc))!=NULL) Twin->SizeOfSrcBuffer=SizeOfSrc;
        else IsSuccess=FALSE;
    }

    /* On regarde si le buffer destination fait déjà la bonne taille */
    if(SizeOfDst>0 && Twin->SizeOfDstBuffer<SizeOfDst)
    {
        Util_FlushDstBuffer(Twin);
        if((Twin->DstBufferPtr=Sys_AllocMem(SizeOfDst))!=NULL) Twin->SizeOfDstBuffer=SizeOfDst;
        else IsSuccess=FALSE;
    }

    return IsSuccess;
}


/*****
    Libération des buffers source et destination
*****/

void Util_FlushDecodeBuffers(struct Twin *Twin, BOOL IsFreeAll)
{
    if(IsFreeAll || Twin->SizeOfSrcBuffer>DECODE_BUFFER_SRC_MAX_SIZE) Util_FlushSrcBuffer(Twin);
    if(IsFreeAll || Twin->SizeOfDstBuffer>DECODE_BUFFER_DST_MAX_SIZE) Util_FlushDstBuffer(Twin);
}


/*****
    Décompression ZLib du buffer de Lecture dans le buffer temporaire
*****/

int Util_ZLibUnCompress(z_stream *Stream, void *SrcPtr, ULONG SrcLen, void *DstPtr, ULONG DstLen)
{
    Stream->next_in=(Bytef *)SrcPtr;
    Stream->avail_in=(uInt)SrcLen;
    Stream->next_out=(Bytef *)DstPtr;
    Stream->avail_out=(uInt)DstLen;
    Stream->data_type=Z_BINARY;

    return inflate(Stream,Z_SYNC_FLUSH);
}


/*****
    Initialisation de la librairie de décompression ZLib
*****/

int Util_ZLibInit(z_stream *Stream)
{
    Stream->next_in=Z_NULL;
    Stream->avail_in=0;
    Stream->next_out=Z_NULL;
    Stream->avail_out=0;
    Stream->total_in=0;
    Stream->total_out=0;
    Stream->zalloc=Z_NULL;
    Stream->zfree=Z_NULL;
    Stream->opaque=Z_NULL;

    return inflateInit(Stream);
}


/*****
    Libération des ressources allouées par la librairie ZLib
*****/

int Util_ZLibEnd(z_stream *Stream)
{
    return inflateEnd(Stream);
}


/*****
    Récupération d'une donnée couleur sur la socket, en tenant compte de la largeur du pixel
*****/

LONG Util_ReadTwinColor(struct Twin *Twin, LONG BytePerColor, BOOL IsBigEndian, ULONG *Color)
{
    LONG Result=Gal_WaitRead(Twin,(void *)Color,BytePerColor);

    if(IsBigEndian) *Color>>=(8*(4-BytePerColor));
    else
    {
        ULONG LocalColor=*Color;
        UBYTE *Ptr=(UBYTE *)&LocalColor;

        switch(BytePerColor)
        {
            default:
                *Color=(ULONG)Ptr[0];
                break;

            case 2:
                *Color=((ULONG)Ptr[1]<<8)+(ULONG)Ptr[0];
                break;

            case 3:
                *Color=((ULONG)Ptr[2]<<16)+((ULONG)Ptr[1]<<8)+(ULONG)Ptr[0];
                break;

            case 4:
                *Color=((ULONG)Ptr[3]<<24)+((ULONG)Ptr[2]<<16)+((ULONG)Ptr[1]<<8)+(ULONG)Ptr[0];
                break;
        }
    }

    return Result;
}




/*****
    Lecture d'une chaîne utf8
    - Str: Buffer destination
    - Size: Taille max du buffer destination
*****/

LONG Util_WaitReadStringUTF8(struct Twin *Twin, char *Str, LONG Size)
{
    LONG Len=0;
    LONG Result=Gal_WaitRead(Twin,(void *)&Len,sizeof(Len));

    if(Result>0)
    {
        if((Result=Gal_WaitFixedRead(Twin,Str,Size-sizeof(char),Len))>0 && Size>0)
        {
            if(Len>Size-sizeof(char)) Len=Size-sizeof(char);
            Str[Len]=0;
            Util_UTF8toANSI(Str,Str,Size);
        }
    }

    return Result;
}


/*****
    Ecriture d'une chaine utf8
    - Str: La chaîne à envoyer
*****/

LONG Util_WaitWriteStringUTF8(struct Twin *Twin, const char *Str)
{
    LONG Len=Sys_StrLen(Str);
    LONG Result=Gal_WaitWrite(Twin,(void *)&Len,sizeof(Len));

    if(Result>0)
    {
        Util_ANSItoUTF8(Str,Twin->Tmp,sizeof(Twin->Tmp)-sizeof(char));
        Result=Gal_WaitWrite(Twin,(void *)Twin->Tmp,Len);
    }

    return Result;
}


/*****
    Lecture d'un octet
*****/

ULONG Util_ReadBuffer8(UBYTE **Buffer, UBYTE *BufferEnd)
{
    ULONG Result=0;

    if((*Buffer)+1<BufferEnd)
    {
        Result=(ULONG)**Buffer;
        (*Buffer)++;
    }

    return Result;
}


/*****
    Lecture d'un mot de 16 bits
*****/

ULONG Util_ReadBuffer16(UBYTE **Buffer, UBYTE *BufferEnd)
{
    ULONG Result=Util_ReadBuffer8(Buffer,BufferEnd);
    Result=(Result<<8)+Util_ReadBuffer8(Buffer,BufferEnd);
    return Result;
}


/*****
    Lecture d'un mot de 32 bits
*****/

ULONG Util_ReadBuffer32(UBYTE **Buffer, UBYTE *BufferEnd)
{
    ULONG Result=Util_ReadBuffer8(Buffer,BufferEnd);
    Result=(Result<<8)+Util_ReadBuffer8(Buffer,BufferEnd);
    Result=(Result<<8)+Util_ReadBuffer8(Buffer,BufferEnd);
    Result=(Result<<8)+Util_ReadBuffer8(Buffer,BufferEnd);
    return Result;
}


/*****
    Lecture d'une chaîne utf8
*****/

void Util_ReadBufferStringUTF8(UBYTE **Buffer, UBYTE *BufferEnd, char *Str, LONG LenMax)
{
    char *Bak=Str;
    ULONG Len=Util_ReadBuffer32(Buffer,BufferEnd),i;

    for(i=0; i<LenMax && i<Len; i++)
    {
        *(Str++)=(char)Util_ReadBuffer8(Buffer,BufferEnd);
    }
    *Str=0;
    Util_UTF8toANSI(Bak,Bak,LenMax);
}


/*****
    Récupération d'une donnée couleur sur un buffer, en tenant compte de la largeur du pixel
*****/

void *Util_ReadBufferColor(LONG BytePerColor, BOOL IsBigEndian, UBYTE *SrcBufferPtr, ULONG *Color)
{
    if(IsBigEndian)
    {
        SrcBufferPtr=Util_ReadBufferColorBE(BytePerColor,SrcBufferPtr,Color);
    }
    else
    {
        switch(BytePerColor)
        {
            default:
                *Color=(ULONG)*(SrcBufferPtr++);
                break;

            case 2:
                *Color=((ULONG)SrcBufferPtr[1]<<8)+(ULONG)SrcBufferPtr[0];
                SrcBufferPtr+=2;
                break;

            case 3:
                *Color=((ULONG)SrcBufferPtr[2]<<16)+((ULONG)SrcBufferPtr[1]<<8)+(ULONG)SrcBufferPtr[0];
                SrcBufferPtr+=3;
                break;

            case 4:
                *Color=(((ULONG)SrcBufferPtr[3])<<24)+(((ULONG)SrcBufferPtr[2])<<16)+(((ULONG)SrcBufferPtr[1])<<8)+(ULONG)SrcBufferPtr[0];
                SrcBufferPtr+=4;
                break;
        }
    }

    return SrcBufferPtr;
}


/*****
    Lecture d'une couleur big endian sur le buffer
*****/

void *Util_ReadBufferColorBE(LONG BytePerColor, UBYTE *SrcBufferPtr, ULONG *Color)
{
    ULONG LocalColor=(ULONG)*(SrcBufferPtr++);
    if(--BytePerColor>0) LocalColor=(LocalColor<<8)+(ULONG)*(SrcBufferPtr++);
    if(--BytePerColor>0) LocalColor=(LocalColor<<8)+(ULONG)*(SrcBufferPtr++);
    if(--BytePerColor>0) LocalColor=(LocalColor<<8)+(ULONG)*(SrcBufferPtr++);
    *Color=LocalColor;

    return SrcBufferPtr;
}


/*****
    Recopie d'une chaîne de données d'un buffer source sur un buffer destination
*****/

void *Util_ReadBufferData(UBYTE *SrcBufferPtr, UBYTE *DstBufferPtr, LONG Len)
{
    while(--Len>=0) *(DstBufferPtr++)=*(SrcBufferPtr++);

    return SrcBufferPtr;
}


/*****
    Conversion d'un buffer little endian en big endian
*****/

void Util_ConvertLittleEndianPixelToBigEndian(void *BufferPtr, ULONG PixelSize, LONG CountOfPixels)
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


/*****
    Conversion d'un time_t 64 bits provenant du protocol Tight
*****/

ULONG Util_Time64ToAmigaTime(const ULONG Time[2])
{
    ULONG D[2]={0,1000},R[2]={0,0},T2[2],Result=Time[1];

    T2[0]=Time[0],
    T2[1]=Time[1];
    Util_DivU64(T2,D,R);
    Result-=(8*365+2)*24*60*60-1*60*60;
    return Result;
}


/*****
    Conversion d'un time_t Amiga en time_t 64 bits pour le protocol Tight
*****/

void Util_AmigaTimeToTime64(ULONG Time, ULONG Time64[2])
{
    ULONG H[2],T[2]={0,(8*365+2)*24*60*60-1*60*60},V[2]={0,1000};
    H[0]=0;
    H[1]=Time;
    Util_Add64(T,H);
    Util_MulU64(V,H);
    Time64[0]=H[0];
    Time64[1]=H[1];
}


/*****
    Routine pour convertir une chaîne ANSI en UTF8, dans le cas où
    la codeset.library n'est pas installée.
*****/

void Util_ANSItoUTF8(const char *Str, char *Dst, LONG SizeOfDst)
{
    if(CodesetsBase!=NULL)
    {
        struct codeset *cs=CodesetsFind("utf-8",TAG_DONE);
        STRPTR StrU8;

        if((StrU8=CodesetsConvertStr(
            CSA_DestCodeset,cs,
            CSA_Source,Str,
            TAG_DONE))!=NULL)
        {
            Sys_StrCopy(Dst,StrU8,SizeOfDst);
            CodesetsFree(StrU8,NULL);
        }
    }
    else
    {
        LONG Pos=0;
        while(*Str!=0 && Pos<SizeOfDst)
        {
            UBYTE Char=(UBYTE)*(Str++);

            if(Char<128) Dst[Pos++]=Char;
            else if(Pos+1<SizeOfDst)
            {
                Dst[Pos++]=0xc0+(Char>>6);
                Dst[Pos++]=0x80+(Char&0x3f);
            }
            else break;
        }
        Dst[Pos]=0;
    }
}


/*****
    Routine pour convertir une chaîne UTF8 en ANSI, dans le cas où
    la codeset.library n'est pas instalée.
*****/

void Util_UTF8toANSI(const char *StrU8, char *Dst, LONG SizeOfDst)
{
    if(CodesetsBase!=NULL)
    {
        CodesetsUTF8ToStr(
            CSA_Source,(ULONG)StrU8,
            CSA_SourceLen,(ULONG)Sys_StrLen(StrU8),
            CSA_Dest,(ULONG)Dst,
            CSA_DestLen,(ULONG)SizeOfDst,
            TAG_DONE);

    }
    else
    {
        static const UBYTE Table[]={0x3f,0x1f,0x0f,0x07,0x3,0x1};
        LONG Pos=0,State=0,Bit=0;
        ULONG Data=0;

        while(*StrU8!=0 && Pos<SizeOfDst)
        {
            UBYTE Char=(UBYTE)*(StrU8++);

            switch(State)
            {
                case 0:
                    if(Char<128) Dst[Pos++]=Char;
                    else
                    {
                        UBYTE Tmp=Char;
                        for(Bit=0; Bit<8 && (Tmp&128)!=0; Bit++) Tmp<<=1;
                        if(Bit>=2)
                        {
                            State=1;
                            Data=Char&Table[(--Bit)-1];
                        } else Bit=0;
                    }
                    break;

                case 1:
                    if((Char&0xc0)==0x80)
                    {
                        Data=(Data<<6)+(Char&0x3f);
                        if(--Bit==0)
                        {
                            Dst[Pos++]=Data>=256?'?':Data;
                            State=0;
                        }
                    }
                    else
                    {
                        StrU8--;
                        State=0;
                    }
                    break;
            }
        }

        Dst[Pos]=0;
    }
}


/*****
    Décalage 64 bits à gauche
*****/

void Util_ShiftL64(ULONG Val64[2], BOOL *Carry)
{
    BOOL NewCarry=(LONG)Val64[0]<0?TRUE:FALSE;

    Val64[0]<<=1;
    if((LONG)Val64[1]<0) Val64[0]|=1;
    Val64[1]<<=1;
    if(Carry!=NULL)
    {
        if(*Carry) Val64[1]|=1;
        *Carry=NewCarry;
    }
}


/*****
    Décalage 64 bits à droite
*****/

void Util_ShiftR64(ULONG Val64[2], BOOL *Carry)
{
    BOOL NewCarry=Val64[1]&1?TRUE:FALSE;

    Val64[1]>>=1;
    if(Val64[0]&1) Val64[1]|=0x80000000;
    Val64[0]>>=1;
    if(Carry!=NULL)
    {
        if(*Carry) Val64[0]|=0x80000000;
        *Carry=NewCarry;
    }
}


/*****
    Division 64 bits non signée
    - N64: Nombre à diviser
    - D64: Diviseur

    Sortie:
    - N64: Contient le quotient
    - R64: Contient le reste
*****/

void Util_DivU64(ULONG N64[2], const ULONG D64[2], ULONG *R64)
{
    LONG i=64;

    R64[0]=R64[1]=0;
    while(--i>=0)
    {
        BOOL Ret=0;
        BOOL Carry=(LONG)N64[0]<0?TRUE:FALSE;

        Util_ShiftL64(R64,&Carry);
        if(R64[0]>D64[0] || (R64[0]==D64[0] && R64[1]>=D64[1]))
        {
            Ret=D64[1]>R64[1]?1:0;
            R64[1]-=D64[1];
            R64[0]-=D64[0]+(ULONG)Ret;
            Ret=1;
        }
        Util_ShiftL64(N64,&Ret);
    }
}


/*****
    Addition de deux nombres 64 bits
*****/

void Util_Add64(const ULONG A64[2], ULONG B[2])
{
    B[1]=A64[1]+B[1];
    B[0]=A64[0]+B[0];
    if(B[1]<A64[1]) B[0]=B[0]+1;
}


/*****
    Multiplication de 2 nombres 64 bits
    - A64: Multiplicand
    - B64: Multiplicateur

    Sortie:
    - B64: Produit
*****/

void Util_MulU64(const ULONG A64[2], ULONG B64[2])
{
    LONG i=64;
    ULONG M64[2],A2[2];

    A2[0]=A64[0];
    A2[1]=A64[1];

    M64[0]=B64[0];
    M64[1]=B64[1];
    B64[0]=B64[1]=0;
    while(--i>=0)
    {
        BOOL Carry=FALSE;
        Util_ShiftR64(A2,&Carry);
        if(Carry) Util_Add64(M64,B64);
        Util_ShiftL64(M64,NULL);
    }
}


/*****
    Libération du buffer source
*****/

void Util_FlushSrcBuffer(struct Twin *Twin)
{
    Twin->SizeOfSrcBuffer=0;
    if(Twin->SrcBufferPtr!=NULL)
    {
        Sys_FreeMem(Twin->SrcBufferPtr);
        Twin->SrcBufferPtr=NULL;
    }
}


/*****
    Libération du buffer destination
*****/

void Util_FlushDstBuffer(struct Twin *Twin)
{
    Twin->SizeOfDstBuffer=0;
    if(Twin->DstBufferPtr!=NULL)
    {
        Sys_FreeMem(Twin->DstBufferPtr);
        Twin->DstBufferPtr=NULL;
    }
}
