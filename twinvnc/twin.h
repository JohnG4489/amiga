#ifndef TWIN_H
#define TWIN_H

#ifndef CONNECT_H
#include "connect.h"
#endif

#ifndef DECODERHEXTILE_H
#include "decoderhextile.h"
#endif

#ifndef DECODERTIGHT_H
#include "decodertight.h"
#endif

#ifndef DECODERZLIBRAW_H
#include "decoderzlibraw.h"
#endif

#ifndef DECODERZRLE_H
#include "decoderzrle.h"
#endif

#ifndef DISPLAY_H
#include "display.h"
#endif

#ifndef GUI_H
#include "gui.h"
#endif

#ifndef INPUT_H
#include "input.h"
#endif

#ifndef CONFIG_H
#include "config.h"
#endif

#ifndef WINDOWCONNECTION_H
#include "windowconnection.h"
#endif

#ifndef WINDOWSTATUS_H
#include "windowstatus.h"
#endif

#ifndef WINDOWOPTIONS_H
#include "windowoptions.h"
#endif

#ifndef WINDOWINFORMATION_H
#include "windowinformation.h"
#endif

#ifndef WINDOWFILETRANSFER_H
#include "windowfiletransfer.h"
#endif

#ifndef FILESCHEDULER_H
#include "filescheduler.h"
#endif


#define RESULT_EVENT_SERVER                 3
#define RESULT_EVENT_CLIENT                 2
#define RESULT_SUCCESS                      1
#define RESULT_DISCONNECTED                 0
#define RESULT_SOCKET_ERROR                 -1
#define RESULT_PROTOCOL_NEGOTIATION_FAILED  -2
#define RESULT_AUTHENTIFICATION_INVALID     -3
#define RESULT_AUTHENTIFICATION_FAILED      -4
#define RESULT_AUTHENTIFICATION_TOOMANY     -5
#define RESULT_NOT_ENOUGH_MEMORY            -6
#define RESULT_CORRUPTED_DATA               -7
#define RESULT_NO_BSDSOCKET_LIBRARY         -8
#define RESULT_CONNECTION_ERROR             -9
#define RESULT_BAD_SCREEN_MODEID            -10
#define RESULT_BAD_SCREEN_DEPTH             -11
#define RESULT_CANT_OPEN_SCREEN             -12
#define RESULT_NOT_ENOUGH_VIDEO_MEMORY      -13
#define RESULT_RECONNECT                    -14
#define RESULT_EXIT                         -15
#define RESULT_TIMEOUT                      -16

/* Define pour la gestion des fenêtres ouvertes/cachées sur l'écran */
#define COUNTOF_POTENTIALY_HIDDEN_WINDOWS   5
#define HIDDENWINDOW_NONE                   0
#define HIDDENWINDOW_FLG_OPTIONS            1
#define HIDDENWINDOW_FLG_INFORMATION        2
#define HIDDENWINDOW_FLG_CONNECTION         4
#define HIDDENWINDOW_FLG_STATUS             8
#define HIDDENWINDOW_FLG_FILETRANSFER       16
#define HIDDENWINDOW_ALL                    (HIDDENWINDOW_FLG_OPTIONS|HIDDENWINDOW_FLG_INFORMATION|HIDDENWINDOW_FLG_CONNECTION|HIDDENWINDOW_FLG_STATUS|HIDDENWINDOW_FLG_FILETRANSFER)


struct Twin
{
    struct Connect Connect;
    struct Display Display;
    struct WindowConnection WindowConnection;
    struct WindowStatus WindowStatus;
    struct WindowOptions WindowOptions;
    struct WindowInformation WindowInformation;
    struct WindowFileTransfer WindowFileTransfer;
    struct MsgPort *MainMPort;
    struct MsgPort *IconMPort;
    struct AppIcon *AppIcon;
    struct DiskObject DefaultDiskObject;
    struct FileRequester *FileReq;
    struct ScreenModeRequester *SMReq;
    struct FontRequester *FontReq;
    struct ProtocolPrefs RemoteProtocolPrefs;
    struct ProtocolPrefs SavedProtocolPrefs;
    struct ProtocolPrefs CurrentProtocolPrefs;
    struct ProtocolPrefs NewProtocolPrefs;
    struct GlobalPrefs SavedGlobalPrefs;
    struct GlobalPrefs CurrentGlobalPrefs;
    struct GlobalPrefs NewGlobalPrefs;
    struct FontDef Font;
    struct HextileData DecoderHextile;
    struct TightData DecoderTight;
    struct ZLibRawData DecoderZLibRaw;
    struct ZRleData DecoderZRle;
    struct Inputs Inputs;
    struct FileList FileList;
    void *SrcBufferPtr;
    void *DstBufferPtr;
    ULONG SizeOfSrcBuffer;
    ULONG SizeOfDstBuffer;
    char Tmp[4096];
    char *AppPath;
    char Host[SIZEOF_HOST_BUFFER];
    char Password[SIZEOF_PASSWORD_BUFFER];
    ULONG Port;
    char Title[256];
    char *LocalFullPathFileName;
    char *RemoteFullPathFileName;
    void *LocalFileHandle;
    ULONG FTCurrentSize64[2];
    ULONG FTFinalSize64[2];
    ULONG FTModTime;
    BYTE ClipboardSignal;
    LONG WaitRefreshPriority;
    BOOL IsRefreshIncremental;
    LONG HiddenWindowOrder[COUNTOF_POTENTIALY_HIDDEN_WINDOWS];
    BOOL IsCheckProtocolPrefs;
    BOOL IsDisplayChanged;
    LONG WindowConnectionResult;
    z_stream Stream;
    BOOL IsStreamOk;
};


#endif  /* TWIN_H */
