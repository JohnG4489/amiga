#ifndef TOOLBAR_H
#define TOOLBAR_H

#ifndef DISPLAY_H
#include "display.h"
#endif

#ifndef GUI_H
#include "gui.h"
#endif

#include <intuition/intuition.h>


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL TBar_CreateToolBar(struct Display *);
extern void TBar_FreeToolBar(struct Display *);
extern void TBar_ShowToolBar(struct Display *);
extern void TBar_HideToolBar(struct Display *);
extern void TBar_RefreshToolBar(struct Display *);
extern LONG TBar_GetToolBarWidth(void);
extern LONG TBar_GetToolBarHeight(void);
extern LONG TBar_ManageToolBarMessages(struct IntuiMessage *);

extern struct CustomClass ToolBarClassList[];


#endif  /* TOOLBAR_H */
