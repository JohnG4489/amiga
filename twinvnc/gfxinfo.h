#ifndef GFXINFO_H
#define GFXINFO_H

#ifndef VIEWINFO_H
#include "viewinfo.h"
#endif

#ifndef FRAMEBUFFER_H
#include "framebuffer.h"
#endif

struct GfxInfo
{
    struct ViewInfo *ViewInfoPtr;
    void (*FillRectFuncPtr)(void *, LONG, LONG, LONG, LONG, ULONG);
    void (*RefreshRectFuncPtr)(void *, LONG, LONG, LONG, LONG);
    void (*UpdateParamFuncPtr)(void *);
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

#endif  /* GFXINFO_H */
