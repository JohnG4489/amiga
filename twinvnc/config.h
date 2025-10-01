#ifndef CONFIG_H
#define CONFIG_H

/* Options d'affichage */
#define DISPLAY_OPTION_WINDOW               0
#define DISPLAY_OPTION_FULLSCREEN           1
#define DISPLAY_OPTION_NODISPLAY            2

#define VIEW_OPTION_NORMAL                  0
#define VIEW_OPTION_SCALE                   1

#define SCRMODE_SELECT_AUTO                 0
#define SCRMODE_SELECT_ASK                  1
#define SCRMODE_SELECT_CURRENT              2

/* Autres */
#define SIZEOF_HOST_BUFFER                  1025
#define SIZEOF_PASSWORD_BUFFER              9
#define DEFAULT_CONTROLKEY                  IEQUALIFIER_RCOMMAND


struct ProtocolPrefs
{
    ULONG EncoderId;
    ULONG Depth;
    LONG ServerScale;
    BOOL IsChangeZLib;
    ULONG ZLibLevel;
    BOOL IsUseJpeg;
    ULONG JpegQuality;
    BOOL IsNoLocalCursor;
    BOOL IsNoCopyRect;
    BOOL IsReportMouse;
    BOOL IsShared;
};

struct GlobalPrefs
{
    ULONG TimeOut;
    BOOL Flag;

    ULONG ScrModeSelectType;
    ULONG ScrModeID;
    ULONG ScrDepth;
    BOOL IsCursor;
    BOOL IsNoClipboard;
    BOOL IsShowToolBar;
    BOOL IsScreenBar;
    BOOL IsKeyboardEmul;
    BOOL IsNoLAmigaKey;
    BOOL IsOverlay;
    LONG OverlayThreshold;
    ULONG DisplayOption;
    ULONG ViewOption;
    BOOL IsSmoothed;
    BOOL IsStartIconify;
    LONG Left;
    LONG Top;
    LONG Width;
    LONG Height;
    ULONG ControlKey;
    char FontName[MAXFONTNAME];
    LONG FontSize;
    BOOL IsViewOnly;
    BOOL IsAltIconifyIcon;
    struct DiskObject *AltIconifyIcon;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern void Cfg_Init(struct GlobalPrefs *, struct ProtocolPrefs *, char *, ULONG *, char *);
extern void Cfg_InitDefaultProtocolPrefs(struct ProtocolPrefs *);
extern LONG Cfg_ReadConfig(struct GlobalPrefs *, struct ProtocolPrefs *, void *, char **, char *, ULONG *, char *, BOOL *, BOOL *);
extern BOOL Cfg_WriteConfig(const struct GlobalPrefs *, const struct ProtocolPrefs *, const char *, const char *, ULONG, const char *, BOOL, const char *);

extern ULONG Cfg_GetControlKeyOptionText(ULONG, char *);


#endif  /* CONFIG_H */

