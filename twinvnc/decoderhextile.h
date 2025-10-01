#ifndef DECODERHEXTILE_H
#define DECODERHEXTILE_H

#include <zlib.h>

struct HextileData
{
    z_stream StreamZLibHex;
    z_stream StreamZLibHexRaw;
    BOOL IsInitZLibHexOk;
    BOOL IsInitZLibHexRawOk;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL OpenDecoderHextile(struct Twin *);
extern void CloseDecoderHextile(struct Twin *);
extern LONG DecoderHextile(struct Twin *, LONG, LONG, LONG, LONG);

#endif  /* DECODERHEXTILE_H */
