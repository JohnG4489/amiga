#include "system.h"
#include "gfxnone.h"


/*
    31-12-2015 (Seg)    Nouveau code
*/


/***** Prototypes */
void Gfxno_Reset(struct GfxInfo *);

void Gfxno_FillRect(struct GfxInfo *, LONG, LONG, LONG, LONG, ULONG);
void Gfxno_RefreshRect(struct GfxInfo *, LONG, LONG, LONG, LONG);


/*****
    Définition des fonctions graphiques adaptées à la vue et au type d'écran donné
*****/

void Gfxno_Reset(struct GfxInfo *gfx)
{
    gfx->FillRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG, ULONG))Gfxno_FillRect;
    gfx->RefreshRectFuncPtr=(void (*)(void *, LONG, LONG, LONG, LONG))Gfxno_RefreshRect;
}


/******************************************/
/*****                                *****/
/***** FONCTIONS PRIVEES : FillRect() *****/
/*****                                *****/
/******************************************/

/*****
    Remplissage: tous
*****/

void Gfxno_FillRect(struct GfxInfo *gfx, LONG x, LONG y, LONG w, LONG h, ULONG Color)
{
}


/*********************************************/
/*****                                   *****/
/***** FONCTIONS PRIVEES : RefreshRect() *****/
/*****                                   *****/
/*********************************************/

/*****
    Rafraîchissement d'un bloc: tous
*****/

void Gfxno_RefreshRect(struct GfxInfo *gfx, LONG x, LONG y, LONG w, LONG h)
{
}
