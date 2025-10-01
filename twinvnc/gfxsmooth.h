#ifndef GFXSMOOTH_H
#define GFXSMOOTH_H

#ifndef GFXINFO_H
#include "gfxinfo.h"
#endif


struct GfxSmooth
{
    struct GfxInfo GfxI;
    void *SmoothXTable;
    void *SmoothYTable;
    void *SmoothBufferPtr;
    LONG SmoothBufferSize;
    UBYTE *SmoothColorMapID;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL Gfxsm_Create(struct GfxSmooth *, struct ViewInfo *);
extern void Gfxsm_Free(struct GfxSmooth *);
extern void Gfxsm_Reset(struct GfxSmooth *);

#endif  /* GFXSMOOTH_H */

