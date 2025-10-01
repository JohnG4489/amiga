#ifndef WINDOWFILETRANSFER_H
#define WINDOWFILETRANSFER_H

#ifndef GUI_H
#include "gui.h"
#endif

#ifndef FILEMANAGER_H
#include "filemanager.h"
#endif


#define WIN_FT_RESULT_ERROR     -1
#define WIN_FT_RESULT_NONE      0
#define WIN_FT_RESULT_REFRESH   1
#define WIN_FT_RESULT_DOWNLOAD1 2
#define WIN_FT_RESULT_DOWNLOAD2 3
#define WIN_FT_RESULT_CLOSE     4
#define WIN_FT_RESULT_DELETE    5


struct WindowFileTransfer
{
    struct CompleteWindow Window;
    struct FileManager LocalFileManager;
    struct FileManager RemoteFileManager;
    struct Hook ListHook;
    struct Window *ParentWindow;
    char FileItemTextTmp[512];
    ULONG LastSeconds;
    ULONG LastMicros;
    void *LastFIN;
    ULONG GaugeProgress;
    LONG *ColorID;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCS DU PROJET **************/

extern BOOL Win_OpenWindowFileTransfer(struct WindowFileTransfer *, struct Window *, struct FontDef *, struct MsgPort *, ULONG);
extern void Win_CloseWindowFileTransfer(struct WindowFileTransfer *);
extern LONG Win_ManageWindowFileTransferMessages(struct WindowFileTransfer *, struct IntuiMessage *);
extern struct FileManager *Win_BeginRemoteFileManagerRefresh(struct WindowFileTransfer *);
extern void Win_EndRemoteFileManagerRefresh(struct WindowFileTransfer *);
extern const struct FileItemNode *Win_GetRemoteFileInfo(struct WindowFileTransfer *);
extern struct FileManager *Win_BeginLocalFileManagerRefresh(struct WindowFileTransfer *);
extern void Win_EndLocalFileManagerRefresh(struct WindowFileTransfer *);
extern const struct FileItemNode *Win_GetLocalFileInfo(struct WindowFileTransfer *);
extern void Win_SetGaugeProgress(struct WindowFileTransfer *, ULONG);
extern BOOL Win_RefreshFileList(struct WindowFileTransfer *);


#endif  /* WINDOWFILETRANSFER_H */
