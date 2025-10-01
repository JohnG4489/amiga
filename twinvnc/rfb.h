#ifndef RFB_H
#define RFB_H


#define RFB_PORT                    5900

#define RFB_PROTOCOL_VERSION        3
#define RFB_PROTOCOL_REVISION       8

#define RFB_PROTOCOL_MIN_VERSION    3
#define RFB_PROTOCOL_MIN_REVISION   3

#define RFB_SIZEOF_PASSWORD         8

struct Rect8
{
    UBYTE x;
    UBYTE y;
    UBYTE w;
    UBYTE h;
};


struct Rect16
{
    UWORD x;
    UWORD y;
    UWORD w;
    UWORD h;
};


/**************************************
    Header
**************************************/

#define RFB_PROTOCOLVERSION_SIZEOF  12
#define RFB_PROTOCOLVERSION_FORMAT  "RFB %03ld.%03ld\n"


/**************************************
    Security Type
**************************************/

#define RFB_SECURITYTYPE_INVALID    0
#define RFB_SECURITYTYPE_NONE       1
#define RFB_SECURITYTYPE_VNCAUTH    2
#define RFB_SECURITYTYPE_TIGHT      16

#define RFB_VNCAUTH_OK              0
#define RFB_VNCAUTH_FAILED          1
#define RFB_VNCAUTH_TOOMANY         2


#define TIGHT_AUTH_NONE     1
#define TIGHT_AUTH_VNC      2

#define FLG_TIGHT_AUTH_VNC  1


/**************************************
    Pixel Format
**************************************/

struct PixelFormat
{
    UBYTE BitPerPixel;
    UBYTE Depth;
    UBYTE BigEndian;
    UBYTE TrueColor;
    UWORD RedMax;
    UWORD GreenMax;
    UWORD BlueMax;
    UBYTE RedShift;
    UBYTE GreenShift;
    UBYTE BlueShift;
    UBYTE Pad1;
    UWORD Pad2;
};


/**************************************
    Set Encodings

    0xFFFFFF00 .. 0xFFFFFF0F -- encoding-specific compression levels;
    0xFFFFFF10 .. 0xFFFFFF1F -- mouse cursor shape data;
    0xFFFFFF20 .. 0xFFFFFF2F -- various protocol extensions;
    0xFFFFFF30 .. 0xFFFFFFDF -- not allocated yet;
    0xFFFFFFE0 .. 0xFFFFFFEF -- quality level for JPEG compressor;
    0xFFFFFFF0 .. 0xFFFFFFFF -- not allocated yet.
**************************************/

#define RFB_ENCODING_RAW                0
#define RFB_ENCODING_COPYRECT           1
#define RFB_ENCODING_RRE                2
#define RFB_ENCODING_CORRE              4
#define RFB_ENCODING_HEXTILE            5
#define RFB_ENCODING_ZLIBRAW            6
#define RFB_ENCODING_TIGHT              7
#define RFB_ENCODING_ZLIBHEX            8
#define RFB_ENCODING_ZRLE               16


/* Hextile et zlibHex */
#define RFB_HEXTILE_SUBENC_MASK_RAW                     1
#define RFB_HEXTILE_SUBENC_MASK_BACKGROUND_SPECIFIED    2
#define RFB_HEXTILE_SUBENC_MASK_FOREGROUND_SPECIFIED    4
#define RFB_HEXTILE_SUBENC_MASK_ANY_SUBRECTS            8
#define RFB_HEXTILE_SUBENC_MASK_SUBRECTS_COLOURED       16
#define RFB_HEXTILE_SUBENC_MASK_ZLIBRAW                 32
#define RFB_HEXTILE_SUBENC_MASK_ZLIBHEX                 64

/* Tight */
#define RFB_ENCODING_COMPRESS_LEVEL0    0xFFFFFF00
#define RFB_ENCODING_COMPRESS_LEVEL1    0xFFFFFF01
#define RFB_ENCODING_COMPRESS_LEVEL2    0xFFFFFF02
#define RFB_ENCODING_COMPRESS_LEVEL3    0xFFFFFF03
#define RFB_ENCODING_COMPRESS_LEVEL4    0xFFFFFF04
#define RFB_ENCODING_COMPRESS_LEVEL5    0xFFFFFF05
#define RFB_ENCODING_COMPRESS_LEVEL6    0xFFFFFF06
#define RFB_ENCODING_COMPRESS_LEVEL7    0xFFFFFF07
#define RFB_ENCODING_COMPRESS_LEVEL8    0xFFFFFF08
#define RFB_ENCODING_COMPRESS_LEVEL9    0xFFFFFF09

#define RFB_ENCODING_XCURSOR            0xFFFFFF10
#define RFB_ENCODING_RICHCURSOR         0xFFFFFF11
#define RFB_ENCODING_POINTERPOS         0xFFFFFF18

#define RFB_ENCODING_LASTRECT           0xFFFFFF20
#define RFB_ENCODING_NEWFBSIZE          0xFFFFFF21

#define RFB_ENCODING_QUALITY_LEVEL0     0xFFFFFFE0
#define RFB_ENCODING_QUALITY_LEVEL1     0xFFFFFFE1
#define RFB_ENCODING_QUALITY_LEVEL2     0xFFFFFFE2
#define RFB_ENCODING_QUALITY_LEVEL3     0xFFFFFFE3
#define RFB_ENCODING_QUALITY_LEVEL4     0xFFFFFFE4
#define RFB_ENCODING_QUALITY_LEVEL5     0xFFFFFFE5
#define RFB_ENCODING_QUALITY_LEVEL6     0xFFFFFFE6
#define RFB_ENCODING_QUALITY_LEVEL7     0xFFFFFFE7
#define RFB_ENCODING_QUALITY_LEVEL8     0xFFFFFFE8
#define RFB_ENCODING_QUALITY_LEVEL9     0xFFFFFFE9

#define RFB_TIGHT_FILL                  8
#define RFB_TIGHT_JPEG                  9
#define RFB_TIGHT_FLG_EXPLICIT_FILTER   4
#define RFB_TIGHT_FILTER_COPY           0
#define RFB_TIGHT_FILTER_PALETTE        1
#define RFB_TIGHT_FILTER_GRADIENT       2



/**************************************
    Messages: Server -> Client
**************************************/

#define RFB_FRAMEBUFFERUPDATE           0
#define RFB_SETCOLOURMAPENTRIES         1
#define RFB_BELL                        2
#define RFB_SERVERCUTTEXT               3
#define RFB_RESIZEFRAMEBUFFER           4
#define RFB_TIGHT_CMD_ID                0xfc


/* Initialisation */
struct ServerInitialisation
{
    UWORD FrameBufferWidth;
    UWORD FrameBufferHeight;
    struct PixelFormat PixelFormat;
    ULONG NameLength;
};


/* Frame Buffer Update */
struct ServerFrameBufferUpdate
{
    UBYTE Type;
    UBYTE Pad;
    UWORD CountOfRect;
};

struct ServerFrameBufferRect
{
    struct Rect16 Rect;
    ULONG EncodingType;
};


/* Set Colour Map Entries */
struct ServerSetColourMapEntries
{
    UBYTE Type;
    UBYTE Pad;
    UWORD FirstColor;
    UWORD CountOfColors;
};

struct ServerSetColourMapRGB
{
    UWORD Red;
    UWORD Green;
    UWORD Blue;
};


/* Bell */
struct ServerBell
{
    UBYTE Type;
};


/* Server Cut Text */
struct ServerCutText
{
    UBYTE Type;
    UBYTE Pad1;
    UWORD Pad2;
    ULONG Length;
};


/* Changement de taille du framebuffer */
struct ServerResizeFrameBuffer
{
    UBYTE Type;
    UBYTE Pad;
    UWORD Width;
    UWORD Height;
};


/**************************************
    Messages: Client -> Servers
**************************************/

#define RFB_SETPIXELFORMAT              0
#define RFB_FIXCOLOURMAPENTRIES         1        /* not currently supported */
#define RFB_SETENCODINGS                2
#define RFB_FRAMEBUFFERUPDATEREQUEST    3
#define RFB_KEYEVENT                    4
#define RFB_POINTEREVENT                5
#define RFB_CLIENTCUTTEXT               6
#define RFB_SETSCALE                    8


/* Initialisation */
struct ClientInitialisation
{
    UBYTE Shared;
};


/* Set Pixel Format */
struct ClientSetPixelFormat
{
    UBYTE Type;
    UBYTE Pad1;
    UWORD Pad2;
    struct PixelFormat PixelFormat;
};


/* Set Encodings */
struct ClientSetEncodings
{
    UBYTE Type;
    UBYTE Pad;
    UWORD CountOfEncodings;
};


/* Frame Buffer Update Request */
struct ClientFrameBufferUpdateRequest
{
    UBYTE Type;
    UBYTE Incremental;
    struct Rect16 Rect;
};


/* Pointer Event */
struct ClientPointerEvent
{
    UBYTE Type;
    UBYTE Mask;
    UWORD x;
    UWORD y;
};


/* Key Event */
struct ClientKeyEvent
{
    UBYTE Type;
    UBYTE DownFlag;
    UWORD Pad;
    ULONG Key;
};


/* Set Scale */
struct ClientSetScale
{
    UBYTE Type;
    UBYTE Scale;
    UWORD Pad;
};


/* Client Cut Text */
struct ClientCutText
{
    UBYTE Type;
    UBYTE Pad1;
    UWORD Pad2;
    ULONG Length;
};


/*********************
    PROTOCOL TIGHT
*********************/

/*
 * Server -> Client
 */

#define TIGHT1_FILE_LIST_DATA                   130
#define TIGHT1_FILE_DOWNLOAD_DATA               131
#define TIGHT1_FILE_UPLOAD_CANCEL               132
#define TIGHT1_FILE_DOWNLOAD_FAILED             133

#define TIGHT2_COMPRESSION_SUPPORT_REPLY        0xFC000101
#define TIGHT2_FILE_LIST_REPLY                  0xFC000103
#define TIGHT2_MD5_REPLY                        0xFC000105
#define TIGHT2_UPLOAD_START_REPLY               0xFC000107
#define TIGHT2_UPLOAD_DATA_REPLY                0xFC000109
#define TIGHT2_UPLOAD_END_REPLY                 0xFC00010B
#define TIGHT2_DOWNLOAD_START_REPLY             0xFC00010D
#define TIGHT2_DOWNLOAD_DATA_REPLY              0xFC00010F
#define TIGHT2_DOWNLOAD_END_REPLY               0xFC000110
#define TIGHT2_MKDIR_REPLY                      0xFC000112
#define TIGHT2_REMOVE_REPLY                     0xFC000114
#define TIGHT2_RENAME_REPLY                     0xFC000116
#define TIGHT2_DIRSIZE_REPLY                    0xFC000118
#define TIGHT2_LAST_REQUEST_FAILED_REPLY        0xFC000119

#define FLG_TIGHT1_FILE_LIST_DATA               0x00004000
#define FLG_TIGHT1_DOWNLOAD_DATA                0x00008000

#define FLG_TIGHT2_COMPRESSION_SUPPORT_REPLY    0x00000001
#define FLG_TIGHT2_FILE_LIST_REPLY              0x00000002
#define FLG_TIGHT2_MD5_REPLY                    0x00000004
#define FLG_TIGHT2_UPLOAD_START_REPLY           0x00000008
#define FLG_TIGHT2_UPLOAD_DATA_REPLY            0x00000010
#define FLG_TIGHT2_UPLOAD_END_REPLY             0x00000020
#define FLG_TIGHT2_DOWNLOAD_START_REPLY         0x00000040
#define FLG_TIGHT2_DOWNLOAD_DATA_REPLY          0x00000080
#define FLG_TIGHT2_DOWNLOAD_END_REPLY           0x00000100
#define FLG_TIGHT2_MKDIR_REPLY                  0x00000200
#define FLG_TIGHT2_REMOVE_REPLY                 0x00000400
#define FLG_TIGHT2_RENAME_REPLY                 0x00000800
#define FLG_TIGHT2_DIRSIZE_REPLY                0x00001000
#define FLG_TIGHT2_LAST_REQUEST_FAILED_REPLY    0x00002000


/* Tight */
struct ServerInteractionCaps
{
    UWORD ServerMessageTypes;
    UWORD ClientMessageTypes;
    UWORD EncodingTypes;
    UWORD Pad;
};


/* Tight */
struct TightCapabilityMsg
{
    LONG Code;
    char Vendor[4];
    char Name[8];
};


/* Réception d'une liste de répertoires/non de fichiers */
struct ServerTight1FileListData
{
    UBYTE Type;
    UBYTE Flags;
    UWORD NumFiles;
    UWORD DataSize;
    UWORD CompressedSize;
    /* Followed by SizeData[NumFiles] */
    /* Followed by Filenames[CompressedSize] */
};


/* Réception d'un paquet de données */
struct ServerTight1FileDownloadData
{
    UBYTE Type;
    UBYTE CompressLevel;
    UWORD RealSize;
    UWORD CompressedSize;
    /* Followed by File[CompressedSize] */
};


/* Structure de données pour une requête filelist */
struct FileListSizeData
{
    ULONG Size;
    ULONG Data;
};


/*
 * Client -> Server
 */

#define TIGHT1_FILE_LIST_REQUEST                130
#define TIGHT1_FILE_DOWNLOAD_REQUEST            131
#define TIGHT1_FILE_UPLOAD_REQUEST              132
#define TIGHT1_FILE_UPLOAD_DATA                 133
#define TIGHT1_FILE_DOWNLOAD_CANCEL             134
#define TIGHT1_FILE_UPLOAD_FAILED               135
#define TIGHT1_FILE_CREATE_DIR_REQUEST          136

#define TIGHT2_COMPRESSION_SUPPORT_REQUEST      0xFC000100
#define TIGHT2_FILE_LIST_REQUEST                0xFC000102
#define TIGHT2_MD5_REQUEST                      0xFC000104
#define TIGHT2_UPLOAD_START_REQUEST             0xFC000106
#define TIGHT2_UPLOAD_DATA_REQUEST              0xFC000108
#define TIGHT2_UPLOAD_END_REQUEST               0xFC00010A
#define TIGHT2_DOWNLOAD_START_REQUEST           0xFC00010C
#define TIGHT2_DOWNLOAD_DATA_REQUEST            0xFC00010E
#define TIGHT2_MKDIR_REQUEST                    0xFC000111
#define TIGHT2_REMOVE_REQUEST                   0xFC000113
#define TIGHT2_RENAME_REQUEST                   0xFC000115
#define TIGHT2_DIRSIZE_REQUEST                  0xFC000117

#define FLG_TIGHT1_FILE_LIST_REQUEST            0x00001000
#define FLG_TIGHT1_FILE_DOWNLOAD_REQUEST        0x00002000
#define FLG_TIGHT1_FILE_UPLOAD_REQUEST          0x00004000

#define FLG_TIGHT2_COMPRESSION_SUPPORT_REQUEST  0x00000001
#define FLG_TIGHT2_FILE_LIST_REQUEST            0x00000002
#define FLG_TIGHT2_MD5_REQUEST                  0x00000004
#define FLG_TIGHT2_UPLOAD_START_REQUEST         0x00000008
#define FLG_TIGHT2_UPLOAD_DATA_REQUEST          0x00000010
#define FLG_TIGHT2_UPLOAD_END_REQUEST           0x00000020
#define FLG_TIGHT2_DOWNLOAD_START_REQUEST       0x00000040
#define FLG_TIGHT2_DOWNLOAD_DATA_REQUEST        0x00000080
#define FLG_TIGHT2_MKDIR_REQUEST                0x00000100
#define FLG_TIGHT2_REMOVE_REQUEST               0x00000200
#define FLG_TIGHT2_RENAME_REQUEST               0x00000400
#define FLG_TIGHT2_DIRSIZE_REQUEST              0x00000800


/* Client File List Request */
struct ClientTight1FileListRequest
{
    UBYTE Type;
    UBYTE Flags;
    UWORD DirNameSize;
};


/* Client File download request */
struct ClientTight1FileDownloadRequest
{
    UBYTE Type;
    UBYTE CompressedLevel;
    UWORD FileNameSize;
    ULONG Position;
};


/* Requete d'Upload de fichier */
struct ClientTight1FileUploadRequest
{
    UBYTE Type;
    UBYTE CompressedLevel;
    UWORD FileNameSize;
    ULONG Position;
    /* Followed by char FileName[FileNameSize] */
};


/* Envoi de donnees en upload */
struct ClientTight1FileUploadData
{
    UBYTE Type;
    UBYTE CompressedLevel;
    UWORD RealSize;
    UWORD CompressedSize;
    /* Followed by File[CompressedSize] */
};

#endif  /* RFB_H */
