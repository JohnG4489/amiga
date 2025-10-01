#ifndef TWINGADGETCLASS_H
#define TWINGADGETCLASS_H

#define TIMG_Image       (TAG_USER+1)
#define TIMG_ColorMapRGB (TAG_USER+2)
#define TIMG_ColorMapID  (TAG_USER+3)
#define TIMG_PosX        (TAG_USER+4)
#define TIMG_PosY        (TAG_USER+5)
#define TIMG_Width       (TAG_USER+6)
#define TIMG_Height      (TAG_USER+7)


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern Class *Tgc_MakeTwinGadgetClass(void);
extern void Tgc_FreeTwinGadgetClass(Class *);

#endif  /* TWINGADGETCLASS_H */
