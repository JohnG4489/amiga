#ifndef WINDOWOPTIONS_H
#define WINDOWOPTIONS_H

#ifndef GUI_H
#include "gui.h"
#endif


#define WIN_OPTIONS_RESULT_NONE      0
#define WIN_OPTIONS_RESULT_TEST      1
#define WIN_OPTIONS_RESULT_USE       2
#define WIN_OPTIONS_RESULT_CANCEL    3
#define WIN_OPTIONS_RESULT_SAVEAS    4


struct WindowOptions
{
    struct CompleteWindow Window;
    struct WindowGadgets *GadgetsPage[5];
    struct CustomMotif *MotifsPage[5];
    LONG Page;
    char DisplayedScreenMode[32];
    char DisplayedFont[128];
    struct Window *ParentWindow;
    struct ProtocolPrefs *PPrefs;
    struct GlobalPrefs *GPrefs;
    struct ScreenModeRequester *SMReq;
    struct FontRequester *FontReq;
    struct Hook ControlKeyHook;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCS DU PROJET **************/

extern BOOL Win_OpenWindowOptions(
    struct WindowOptions *, struct Window *,
    struct ProtocolPrefs *, struct GlobalPrefs *,
    struct ScreenModeRequester *, struct FontRequester *,
    struct FontDef *, struct MsgPort *);
extern void Win_CloseWindowOptions(struct WindowOptions *);
extern void Win_SetOptionsPage(struct WindowOptions *, LONG);
extern void Win_RefreshOptionsPage(struct WindowOptions *);
extern LONG Win_ManageWindowOptionsMessages(struct WindowOptions *, struct IntuiMessage *);


#endif  /* WINDOWOPTIONS_H */
