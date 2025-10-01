#ifndef GENERAL_H
#define GENERAL_H

#ifndef SOCKET_H
#include "socket.h"
#endif


enum
{
    GAL_CTRL_NONE=0,
    GAL_CTRL_REFRESH,
    GAL_CTRL_ICONIFY,
    GAL_CTRL_SWITCH_VIEW,
    GAL_CTRL_SWITCH_DISPLAY,
    GAL_CTRL_SWITCH_SCALE,
    GAL_CTRL_SWITCH_TOOLBAR,
    GAL_CTRL_SWITCH_SCREENBAR,
    GAL_CTRL_OPTIONS,
    GAL_CTRL_INFORMATION,
    GAL_CTRL_RECONNECT,
    GAL_CTRL_FILE_TRANSFER,
    GAL_CTRL_RESIZE_FULL,
    GAL_CTRL_VIEW_RATIO,
    GAL_CTRL_ABOUT,
    GAL_CTRL_EXIT
};


#define Gal_WaitWrite(Twin,Buffer,SizeOfBuffer) SocketWrite((Twin)->Connect.Socket,(char *)(Buffer),(long)(SizeOfBuffer))


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern LONG Gal_WaitEvent(struct Twin *, ULONG *);
extern LONG Gal_WaitRead(struct Twin *, void *, LONG);
extern LONG Gal_WaitFixedRead(struct Twin *, void *, LONG, LONG);
//extern LONG Gal_WaitWrite(struct Twin *, void *, LONG);
extern LONG Gal_ManageMessagePort(struct Twin *, ULONG);
extern const struct PixelFormat *Gal_GetPixelFormatFromDepth(struct Twin *, ULONG);
extern struct Window *Gal_GetParentWindow(struct Twin *);
extern struct Screen *Gal_GetBestPublicScreen(struct Window *, BOOL *, ULONG *);
extern char *Gal_MakeDisplayTitle(struct Twin *);
extern char *Gal_MakeAppTitle(struct Twin *);
extern void Gal_RemoveIconApplication(struct Twin *);
extern LONG Gal_IconifyApplication(struct Twin *);
extern LONG Gal_ExpandApplication(struct Twin *);
extern void Gal_InitDisplay(struct Twin *);
extern LONG Gal_OpenDisplay(struct Twin *);
extern LONG Gal_ChangeDisplay(struct Twin *);
extern BOOL Gal_AllocAppResources(struct Twin *);
extern void Gal_FreeAllAppResources(struct Twin *);
extern BOOL Gal_GetScreenModeID(BOOL, ULONG *, ULONG *, LONG, LONG, BOOL *);
extern void Gal_HideOpennedWindow(struct Twin *, struct Screen *, ULONG);
extern BOOL Gal_ShowHiddenWindow(struct Twin *, ULONG);
extern BOOL Gal_EasyOpenWindowConnection(struct Twin *);
extern BOOL Gal_EasyOpenWindowStatus(struct Twin *);
extern BOOL Gal_EasyOpenWindowInformation(struct Twin *);
extern BOOL Gal_EasyOpenWindowOptions(struct Twin *);
extern BOOL Gal_EasyOpenWindowFileTransfer(struct Twin *);
//extern void WriteLog(const char *);


#endif  /* GENERAL_H */
