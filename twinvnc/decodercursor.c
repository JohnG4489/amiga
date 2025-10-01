#include "system.h"
#include "util.h"
#include "decodercursor.h"
#include "general.h"
#include "twin.h"


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    17-01-2016 (Seg)    Modifications suite au changement de l'API decoding
    16-01-2016 (Seg)    Modification des appels graphiques suite à la refonte de display.c
    11-01-2006 (Seg)    Gestion du format du pixel serveur
    06-09-2004 (Seg)    Gestion du curseur souris
*/


/***** Prototypes */
BOOL OpenDecoderCursor(struct Twin *);
void CloseDecoderCursor(struct Twin *);
LONG DecoderRichCursor(struct Twin *, LONG, LONG, LONG, LONG);
LONG DecoderXCursor(struct Twin *, LONG, LONG, LONG, LONG);
void FreeCursor(struct Twin *);



/*****
    Initialisation de la structure CursorData
*****/

BOOL OpenDecoderCursor(struct Twin *Twin)
{
    struct CursorData *cd=&Twin->Display.Cursor;

    cd->CursorPtr=NULL;
    cd->MaskPtr=NULL;
    cd->TmpPtr=NULL;
    cd->PrevX=0;
    cd->PrevY=0;
    cd->PrevWidth=0;
    cd->PrevHeight=0;

    return TRUE;
}


/*****
    Libération des ressources allouées pour le curseur
*****/

void CloseDecoderCursor(struct Twin *Twin)
{
    FreeCursor(Twin);
}


/*****
    Décode RichCursor
*****/

LONG DecoderRichCursor(struct Twin *Twin, LONG x, LONG y, LONG w, LONG h)
{
    LONG Result=RESULT_SUCCESS;
    struct Display *disp=&Twin->Display;
    struct CursorData *cd=&disp->Cursor;
    void *CursorPtr=NULL;
    UBYTE *MaskPtr=NULL;
    void *TmpPtr=NULL;

    if(w>0 && h>0)
    {
        struct FrameBuffer *fb=&disp->FrameBuffer;
        LONG PixelSize=fb->PixelFormatSize;
        LONG CountOfPixels=w*h;
        ULONG SizeOfPixelArray=CountOfPixels*PixelSize;

        Result=RESULT_NOT_ENOUGH_MEMORY;
        if((CursorPtr=Sys_AllocMem(SizeOfPixelArray))!=NULL)
        {
            if((MaskPtr=Sys_AllocMem(CountOfPixels))!=NULL)
            {
                if((TmpPtr=Sys_AllocMem(SizeOfPixelArray))!=NULL)
                {
                    if((Result=Gal_WaitRead(Twin,CursorPtr,SizeOfPixelArray))>0)
                    {
                        ULONG SizeOfMask=h*((w+7)/8);

                        Frb_ConvertBufferForFrameBuffer(fb,CursorPtr,PixelSize,(BOOL)fb->PixelFormat->BigEndian,CountOfPixels);

                        if((Result=Gal_WaitRead(Twin,TmpPtr,SizeOfMask))>0)
                        {
                            LONG j;
                            for(j=0;j<h;j++)
                            {
                                LONG i;
                                for(i=0;i<w;i++)
                                {
                                    BOOL Flag=FALSE;
                                    if(((UBYTE *)TmpPtr)[i/8+j*((w+7)/8)]&(1<<(7-(i%8)))) Flag=TRUE;
                                    MaskPtr[i+j*w]=Flag;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    FreeCursor(Twin);
    cd->HotX=x;
    cd->HotY=y;
    cd->Width=w;
    cd->Height=h;
    cd->CursorPtr=CursorPtr;
    cd->MaskPtr=MaskPtr;
    cd->TmpPtr=TmpPtr;
    if(Result>0) Disp_RefreshCursor(disp);

    return Result;
}


/*****
    Décode XCursor
*****/

LONG DecoderXCursor(struct Twin *Twin, LONG x, LONG y, LONG w, LONG h)
{
    LONG Result=RESULT_SUCCESS;
    struct Display *disp=&Twin->Display;
    struct CursorData *cd=&disp->Cursor;
    void *CursorPtr=NULL;
    UBYTE *MaskPtr=NULL;
    void *TmpPtr=NULL;

    if(w>0 && h>0)
    {
        ULONG PixelSize=disp->FrameBuffer.PixelFormatSize;
        ULONG SizeOfPixelArray=w*h*PixelSize;

        Result=RESULT_NOT_ENOUGH_MEMORY;
        if((CursorPtr=Sys_AllocMem(SizeOfPixelArray))!=NULL)
        {
            if((MaskPtr=Sys_AllocMem(w*h))!=NULL)
            {
                if((TmpPtr=Sys_AllocMem(SizeOfPixelArray))!=NULL)
                {
                    ULONG Color[2];

                    Result=Util_ReadTwinColor(Twin,3,TRUE,&Color[1]);
                    if(Result>0) Result=Util_ReadTwinColor(Twin,3,TRUE,&Color[0]);

                    if(Result>0)
                    {
                        LONG Width=(w+7)/8;
                        LONG Size=Width*h;

                        if((Result=Gal_WaitRead(Twin,TmpPtr,2*Size))>0)
                        {
                            UBYTE *CursorChunky=(UBYTE *)CursorPtr;
                            UBYTE *MaskChunky=(UBYTE *)MaskPtr;
                            UBYTE *CursorBitMap=(UBYTE *)TmpPtr;
                            UBYTE *MaskBitMap=&CursorBitMap[Size];
                            LONG j;

                            for(j=0;j<h;j++)
                            {
                                LONG i;
                                for(i=0;i<w;i++)
                                {
                                    LONG c=i/8,b=7-i%8;

                                    switch(PixelSize)
                                    {
                                        case 1:
                                            CursorChunky[i]=(CursorBitMap[c]&1<<b)?Color[1]:Color[0];
                                            break;

                                        case 2:
                                            ((UWORD *)CursorChunky)[i]=(CursorBitMap[c]&1<<b)?Color[1]:Color[0];
                                            break;

                                        default:
                                            ((ULONG *)CursorChunky)[i]=(CursorBitMap[c]&1<<b)?Color[1]:Color[0];
                                            break;
                                    }
                                    MaskChunky[i]=(MaskBitMap[c]&1<<b)?TRUE:FALSE;
                                }
                                CursorChunky=&CursorChunky[w*PixelSize];
                                MaskChunky=&MaskChunky[w];
                                CursorBitMap=&CursorBitMap[Width];
                                MaskBitMap=&MaskBitMap[Width];
                            }
                        }
                    }
                }
            }
        }
    }

    FreeCursor(Twin);
    cd->HotX=x;
    cd->HotY=y;
    cd->Width=w;
    cd->Height=h;
    cd->CursorPtr=CursorPtr;
    cd->MaskPtr=MaskPtr;
    cd->TmpPtr=TmpPtr;
    if(Result>0) Disp_RefreshCursor(disp);

    return Result;
}


/*****
    Autres routines liées à la gestion du curseur
*****/

void FreeCursor(struct Twin *Twin)
{
    struct CursorData *cd=&Twin->Display.Cursor;

    if(cd->CursorPtr!=NULL) Sys_FreeMem(cd->CursorPtr);
    if(cd->MaskPtr!=NULL) Sys_FreeMem(cd->MaskPtr);
    if(cd->TmpPtr!=NULL) Sys_FreeMem(cd->TmpPtr);
    cd->CursorPtr=NULL;
    cd->MaskPtr=NULL;
    cd->TmpPtr=NULL;
    cd->HotX=0;
    cd->HotY=0;
    cd->Width=0;
    cd->Height=0;
}
