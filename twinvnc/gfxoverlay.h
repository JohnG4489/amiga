#ifndef GFXOVERLAY_H
#define GFXOVERLAY_H

#ifndef GFXINFO_H
#include "gfxinfo.h"
#endif


#define OVERLAY_SUCCESS             0
#define OVERLAY_NOT_AVAILABLE       1
#define OVERLAY_NO_OVERLAY_MEMORY   2
#define OVERLAY_BAD_FORMAT          3
#define OVERLAY_NOT_ENOUGH_MEMORY   4


struct GfxOverlay
{
    struct GfxInfo GfxI;
    APTR VLHandle;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern ULONG Gfxo_Create(struct GfxOverlay *, struct ViewInfo *);
extern void Gfxo_Free(struct GfxOverlay *);
extern void Gfxo_Reset(struct GfxOverlay *);
extern void Gfxo_FillColorKey(struct GfxOverlay *);

#endif  /* GFXOVERLAY_H */

