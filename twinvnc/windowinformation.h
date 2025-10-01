#ifndef WINDOWINFORMATION_H
#define WINDOWINFORMATION_H

#ifndef GUI_H
#include "gui.h"
#endif

#define WIN_INFORMATION_RESULT_NONE     0
#define WIN_INFORMATION_RESULT_CLOSE    1


struct WindowInformation
{
    struct CompleteWindow Window;
    struct Connect *Connect;
    char *Host;
    char IP[24];
    char Protocol[16];
    char BitPerPixel[4];
    char Depth[4];
    char BigEndian[16];
    char TrueColor[16];
    char RGBMax[16];
    char RGBShift[16];
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCS DU PROJET **************/

extern BOOL Win_OpenWindowInformation(struct WindowInformation *, struct Window *, struct Connect *, char *, struct FontDef *, struct MsgPort *);
extern void Win_CloseWindowInformation(struct WindowInformation *);
extern LONG Win_ManageWindowInformationMessages(struct WindowInformation *, struct IntuiMessage *);


#endif  /* WINDOWINFORMATION_H */
