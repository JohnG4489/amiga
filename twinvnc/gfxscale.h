#ifndef GFXSCALE_H
#define GFXSCALE_H

#ifndef GFXINFO_H
#include "gfxinfo.h"
#endif


struct GfxScale
{
    struct GfxInfo GfxI;
    LONG *ScaleXTable;
    LONG *ScaleYTable;
    void *ScaleBufferPtr;
    LONG ScaleBufferSize;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL Gfxs_Create(struct GfxScale *, struct ViewInfo *);
extern void Gfxs_Free(struct GfxScale *);
extern void Gfxs_Reset(struct GfxScale *);

#endif  /* GFXSCALE_H */
