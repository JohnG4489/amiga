#ifndef DECODERZRLE_H
#define DECODERZRLE_H

#include <zlib.h>

struct ZRleData
{
    z_stream StreamZRle;
    BOOL IsInitStreamZRleOk;
    ULONG Palette[128];
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL OpenDecoderZRle(struct Twin *);
extern void CloseDecoderZRle(struct Twin *);
extern LONG DecoderZRle(struct Twin *, LONG, LONG, LONG, LONG);

#endif  /* DECODEZRLE_H */
