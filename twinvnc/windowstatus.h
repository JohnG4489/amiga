#ifndef WINDOWSTATUS_H
#define WINDOWSTATUS_H

#ifndef GUI_H
#include "gui.h"
#endif


#define WIN_STATUS_RESULT_NONE      0
#define WIN_STATUS_RESULT_CANCEL    1


struct WindowStatus
{
    struct CompleteWindow Window;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCS DU PROJET **************/

extern BOOL Win_OpenWindowStatus(struct WindowStatus *, struct Window *, struct FontDef *, struct MsgPort *);
extern void Win_CloseWindowStatus(struct WindowStatus *);
extern void Win_SetWindowStatusMessage(struct WindowStatus *, const char *, const char *);
extern LONG Win_ManageWindowStatusMessages(struct WindowStatus *, struct IntuiMessage *);


#endif  /* WINDOWSTATUS_H */
