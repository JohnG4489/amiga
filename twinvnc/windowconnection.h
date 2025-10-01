#ifndef WINDOWCONNECTION_H
#define WINDOWCONNECTION_H

#ifndef GUI_H
#include "gui.h"
#endif


#define WIN_CONNECTION_RESULT_NONE      0
#define WIN_CONNECTION_RESULT_CONNECT   1
#define WIN_CONNECTION_RESULT_OPTIONS   2
#define WIN_CONNECTION_RESULT_QUIT      3


#define HOST_LENGTH             1024
#define PASSWORD_LENGTH         16


struct HistoryStringInfo
{
    char Buffer[400];
    LONG CountOfLines;
    char *NewLine;
    LONG NewLineMaxLen;
    LONG LineNumber;
};


struct PasswordStringInfo
{
    char Password[PASSWORD_LENGTH+1];
    char HideChar;
};


struct WindowConnection
{
    struct CompleteWindow Window;
    struct GlobalPrefs *GPrefs;
    char TmpHostPort[HOST_LENGTH+16]; /* + le port */
    struct Hook HostHook;
    struct Hook PasswordHook;
    struct HistoryStringInfo HostData;
    struct PasswordStringInfo PasswordData;
    struct Gadget *ActiveString;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCS DU PROJET **************/

extern BOOL Win_OpenWindowConnection(struct WindowConnection *, struct Window *, char *, ULONG *, char *, struct GlobalPrefs *, ULONG, struct FontDef *, struct MsgPort *);
extern void Win_CloseWindowConnection(struct WindowConnection *);
extern LONG Win_ManageWindowConnectionMessages(struct WindowConnection *, struct IntuiMessage *);
extern char *Win_GetWindowConnectionHost(struct WindowConnection *, ULONG *);
extern char *Win_GetWindowConnectionPassword(struct WindowConnection *);
extern void Win_SaveHostsHistory(struct WindowConnection *);


#endif  /* WINDOWCONNECTION_H */
