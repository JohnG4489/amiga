#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#ifndef RFB_H
#include "rfb.h"
#endif


#define PIXEL_LUT           0
#define PIXEL_R5G6B5        1
#define PIXEL_R8G8B8        2
#define PIXEL_RxGyBz        3


struct FrameBuffer
{
    LONG Width;
    LONG Height;
    void *BufferPtr;
    const struct PixelFormat *PixelFormat;
    LONG ColorMapRGB32[256];
    LONG ColorCount;
    ULONG PixelBufferSize;
    ULONG PixelFormatSize;
    ULONG PixelFormatType;
    LONG ConvPixShiftR;
    LONG ConvPixShiftG;
    LONG ConvPixShiftB;
    ULONG ConvPixMaskR;
    ULONG ConvPixMaskG;
    ULONG ConvPixMaskB;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL Frb_Prepare(struct FrameBuffer *, const struct PixelFormat *, LONG, LONG);
extern void Frb_Flush(struct FrameBuffer *);
extern void Frb_CalcColorMapTable(struct FrameBuffer *, const struct PixelFormat *);

extern void Frb_ConvertBufferForFrameBuffer(struct FrameBuffer *, void *, LONG, BOOL, LONG);
extern ULONG Frb_ConvertColorForFrameBuffer(struct FrameBuffer *, ULONG, LONG);
extern void Frb_WritePixel(struct FrameBuffer *, LONG, LONG, ULONG);
extern void Frb_FillScanLineRect(struct FrameBuffer *, LONG, LONG, LONG, LONG, LONG, LONG, ULONG);
extern void Frb_FillRect(struct FrameBuffer *, LONG, LONG, LONG, LONG, ULONG);
extern void Frb_CopyBitmap(struct FrameBuffer *, UBYTE *, LONG, LONG, LONG, LONG, ULONG, ULONG);
extern void Frb_CopyChunky8(struct FrameBuffer *, UBYTE *, LONG, LONG, LONG, LONG, ULONG *);
extern void Frb_CopyRectFromBuffer(struct FrameBuffer *, void *, ULONG, LONG, LONG, LONG, LONG);
extern void Frb_CopyRect(struct FrameBuffer *, LONG, LONG, LONG, LONG, LONG, LONG);
extern void Frb_ReadFromFrameBuffer(struct FrameBuffer *, LONG, LONG, LONG, LONG, void *);
extern void Frb_CopyRectMasked(struct FrameBuffer *, void *, UBYTE *, LONG, LONG, LONG, LONG, LONG);
extern BOOL Frb_ClipToFrameBuffer(struct FrameBuffer *, LONG *, LONG *, LONG *, LONG *);


#endif  /* FRAMEBUFFER_H */
