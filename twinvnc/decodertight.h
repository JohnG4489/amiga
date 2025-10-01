#ifndef DECODERTIGHT_H
#define DECODERTIGHT_H

#ifndef JPEGLIB_H

#ifndef _SIZE_T
#define _SIZE_T
typedef int size_t;
#endif

#undef GLOBAL
#include <stdio.h>
#include <jpeglib.h>
#define JPEGLIB_H
#endif

struct TightData
{
    z_stream Stream[4];
    BOOL Flags[4];
    ULONG Palette[256];
    ULONG NPal;
    struct jpeg_decompress_struct JpegDec;
    struct jpeg_error_mgr JpegErr;
    struct jpeg_source_mgr JpegSrcManager;
    BOOL IsJpegError;
};

/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL OpenDecoderTight(struct Twin *);
extern void CloseDecoderTight(struct Twin *);
extern LONG DecoderTight(struct Twin *, LONG, LONG, LONG, LONG);

#endif  /* DECODETIGHT_H */
