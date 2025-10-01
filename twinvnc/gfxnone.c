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
    D�finition des fonctions graphiques adapt�es � la vue et au type d'�cran donn�
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
    Rafra�chissement d'un bloc: tous
*****/

void Gfxno_RefreshRect(struct GfxInfo *gfx, LONG x, LONG y, LONG w, LONG h)
{
}
