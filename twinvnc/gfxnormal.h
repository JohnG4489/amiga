#ifndef GFXNORMAL_H
#define GFXNORMAL_H

#ifndef GFXINFO_H
#include "gfxinfo.h"
#endif


struct GfxNormal
{
    struct GfxInfo GfxI;
    void *NormalBufferPtr;
    LONG NormalBufferSize;
    LONG OffsetX;
    LONG OffsetY;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL Gfxn_Create(struct GfxNormal *, struct ViewInfo *);
extern void Gfxn_Free(struct GfxNormal *);
extern void Gfxn_Reset(struct GfxNormal *);

#endif  /* GFXNORMAL_H */
