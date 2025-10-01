#ifndef CLIENT_H
#define CLIENT_H

#ifndef TWIN_H
#include "twin.h"
#endif


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern LONG SetClientPixelFormat(struct Twin *, const struct PixelFormat *);
extern LONG SetEncodings(struct Twin *, struct ProtocolPrefs *);
extern LONG SetFrameBufferUpdateRequest(struct Twin *, BOOL);
extern LONG SetPointerEvent(struct Twin *, ULONG, ULONG, ULONG);
extern LONG SetKeyEvent(struct Twin *, ULONG, ULONG);
extern LONG SetScale(struct Twin *, ULONG);
extern LONG SetClientCutText(struct Twin *, BOOL);

extern LONG Clt_SetFileListRequest(struct Twin *, const char *, LONG);
extern LONG Clt_SetFileDownloadRequest(struct Twin *, const char *, const char *, const char *, const ULONG [2], ULONG);
extern LONG Clt_SetFileUploadRequest(struct Twin *, const char *, const char *, const char *, const ULONG [2], ULONG, const ULONG [2], BOOL);
extern LONG Clt_SetFileRemoveRequest(struct Twin *, const char *, const char *);
extern LONG Clt_SetFileUploadEndRequest(struct Twin *, ULONG, ULONG);

//extern LONG Tight2_SetFileListRequest(struct Twin *, const char *, LONG);
extern LONG Tight2_SetFileDownloadDataRequest(struct Twin *, ULONG);
//extern LONG Tight2_SetMkDirRequest(struct Twin *, const char *);
//extern LONG Tight2_SetRenameRequest(struct Twin *, const char *, const char *);
//extern LONG Tight2_SetRemoveRequest(struct Twin *, const char *);
//extern LONG Tight2_SetFileUploadRequest(struct Twin *, const char *, ULONG, ULONG, BOOL);
extern LONG Tight2_SetFileUploadDataRequest(struct Twin *, const UBYTE *, ULONG);
//extern LONG Tight2_SetFileUploadEndRequest(struct Twin *, ULONG, ULONG);

extern void Clt_FlushFTFullPathFileName(struct Twin *);

#endif  /* CLIENT_H */
