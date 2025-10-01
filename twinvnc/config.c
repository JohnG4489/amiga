#include "global.h"
#include "system.h"
#include "config.h"
#include "client.h"
#include "d3des.h"
#include "twinvnc_const.h"
#include "rfb.h"


/*
    26-06-2017 (Seg)    Refonte de la gestion du CTRLKEY
    29-01-2016 (Seg)    Ajout de l'option CTRLKEY
    18-01-2016 (Seg)    Mise aux normes du code
    10-03-2007 (Seg)    Correction sur la gestion du chemin de l'outil par defaut dans l'icône projet
    07-03-2007 (Seg)    Amélioration du code de sauvegarde de la config
    03-02-2007 (Seg)    Sauvegarde de la config
    28-12-2006 (Seg)    Ajout de l'argument SMOOTH
    06-03-2006 (Seg)    Ajout des arguments FONTNAME et FONTSIZE
    27-02-2006 (Seg)    Ajout de la fonction CleanConfig() + d'autres modifications
    29-08-2005 (Seg)    Gestion de la config
*/


enum TVNC_ARGS
{
    ARG_HOST=0,
    ARG_PORT,
    ARG_PASS,
    ARG_SPASS,

    ARG_ENCODERID,
    ARG_ASKMODEID,
    ARG_MODEID,
    ARG_MODEDEPTH,
    ARG_DEPTH,

    ARG_SERVERSCALE,
    ARG_ZLIB,
    ARG_JPEG,
    ARG_CURSOR,

    ARG_NOLOCAL,
    ARG_NOCOPYRECT,
    ARG_NOREPORTMOUSE,
    ARG_NOCLIPBOARD,

    ARG_NODISPLAY,
    ARG_WINDOW,
    ARG_TOOLBAR,
    ARG_SCREENBAR,

    ARG_KBEMUL,
    ARG_NOLAMIGAKEY,
    ARG_NOSHARED,
    ARG_VIEWONLY,

    ARG_OVERLAY,
    ARG_SCALE,
    ARG_SMOOTH,
    ARG_ICONIFY,

    ARG_LEFT,
    ARG_TOP,
    ARG_WIDTH,
    ARG_HEIGHT,

    ARG_FONTNAME,
    ARG_FONTSIZE,
    ARG_CTRLKEY,
    ARG_CRYPT,
    ARG_FLAG,
    COUNTOF_ARGS
};


const char *ToolTypes[COUNTOF_ARGS]=
{
    "HOST",         "PORT",         "PASS",         "SPASS",
    "ENCODERID",    "ASKMODEID",    "MODEID",       "MODEDEPTH",
    "DEPTH",        "SERVERSCALE",  "ZLIB",         "JPEG",
    "CURSOR",       "NOLOCAL",      "NOCOPYRECT",   "NOREPORTMOUSE",
    "NOCLIPBOARD",  "NODISPLAY",    "WINDOW",       "TOOLBAR",
    "SCREENBAR",    "KBEMUL",       "NOLAMIGAKEY",  "NOSHARED",
    "VIEWONLY",     "OVERLAY",      "SCALE",        "SMOOTH",
    "ICONIFY",      "LEFT",         "TOP",          "WIDTH",
    "HEIGHT",       "FONTNAME",     "FONTSIZE",     "CTRLKEY",
    NULL,           "FLAG"
};

const char *Arguments=
    "HOST,PORT/N,PASS,SPASS,ENCODERID/N,ASKMODEID/S,MODEID/N,MODEDEPTH/N,DEPTH/N,"
    "SERVERSCALE/N,ZLIB/N,JPEG/N,AMIGACURSOR=CURSOR/S,NOLOCALCURSOR=NOLOCAL/S,"
    "NOCOPYRECT/S,NOREPORTMOUSE/S,NOCLIPBOARD/S,NODISPLAY/S,WINDOW/S,TOOLBAR/S,"
    "SCREENBAR/S,KBEMUL/S,NOLAMIGAKEY/S,NOSHARED/S,VIEWONLY/S,OVERLAY/N,SCALE/S,"
    "SMOOTH/S,ICONIFY/S,LEFT/N,TOP/N,WIDTH/N,HEIGHT/N,FONTNAME,FONTSIZE/N,CTRLKEY,"
    "CRYPT,FLAG/S";

extern struct DiskObject AppIconDObj;
static unsigned char FixedKey[8]={23,82,107,6,35,78,88,7};
static unsigned char PassTable[17]="56e7c328bf9d1a04";//"@#()&$*/!?:[]{}%";

static const ULONG QualList[]=
{
    IEQUALIFIER_LSHIFT,
    IEQUALIFIER_RSHIFT,
    IEQUALIFIER_CONTROL,
    IEQUALIFIER_LALT,
    IEQUALIFIER_RALT,
    IEQUALIFIER_LCOMMAND,
    IEQUALIFIER_RCOMMAND,
};

static const char *QualName[]=
{
    "LSHIFT",
    "RSHIFT",
    "CTRL",
    "LALT",
    "RALT",
    "LAMIGA",
    "RAMIGA"
};


/***** Prototypes */
void Cfg_Init(struct GlobalPrefs *, struct ProtocolPrefs *, char *, ULONG *, char *);
void Cfg_InitDefaultProtocolPrefs(struct ProtocolPrefs *);
LONG Cfg_ReadConfig(struct GlobalPrefs *, struct ProtocolPrefs *, void *, char **, char *, ULONG *, char *, BOOL *, BOOL *);
BOOL Cfg_WriteConfig(const struct GlobalPrefs *, const struct ProtocolPrefs *, const char *, const char *, ULONG, const char *, BOOL, const char *);

LONG Cfg_ReadConfigCli(struct GlobalPrefs *, struct ProtocolPrefs *, char **, char **, char *, ULONG *, char *, BOOL *, BOOL *);
LONG Cfg_ReadConfigIcon(struct GlobalPrefs *, struct ProtocolPrefs *, struct WBStartup *, char **, char *, ULONG *, char *, BOOL *, BOOL *);

ULONG Cfg_ParseControlKeyOption(const char *);
ULONG Cfg_GetControlKeyOptionText(ULONG, char *);

BOOL Cfg_SetLongFromString(const char *, LONG *);
char *Cfg_SetToolTypeString(char **, const char *, const char *);
char *Cfg_SetToolTypeLong(char **, const char *, LONG);
char *Cfg_SetToolTypeULong(char **, const char *, ULONG);
char *Cfg_GetProgramPathFromCli(void);
char *Cfg_GetProgramPathFromIcon(BPTR, const char *);
void Cfg_CryptPass(const char *, char *);
void Cfg_UnCryptPass(const char *, char *);


/*****
    Initialisation des structures de configuration.
    Remplissage par défaut des structures GlobalPrefs et ProtocolPrefs.
    Initilisation de Host, Port et Password passés en paramètres.
*****/

void Cfg_Init(struct GlobalPrefs *gp, struct ProtocolPrefs *pp, char *Host, ULONG *Port, char *Password)
{
    Cfg_InitDefaultProtocolPrefs(pp);
    pp->EncoderId=0;            /* On force l'encoder Tight dans les prefs twinvnc par défaut */
    pp->IsNoLocalCursor=FALSE;  /* On force la gestion locale du curseur */
    pp->IsNoCopyRect=FALSE;     /* On force l'encodeur copyrect */

    Sys_StrCopy(Host,"",SIZEOF_HOST_BUFFER);
    Sys_StrCopy(Password,"",SIZEOF_PASSWORD_BUFFER);
    *Port=RFB_PORT;

    gp->ScrModeSelectType=SCRMODE_SELECT_AUTO;
    gp->ScrModeID=INVALID_ID;
    gp->ScrDepth=0;
    gp->IsCursor=FALSE;
    gp->IsNoClipboard=FALSE;

    gp->ViewOption=VIEW_OPTION_NORMAL;
    gp->DisplayOption=DISPLAY_OPTION_FULLSCREEN;
    gp->IsShowToolBar=FALSE;
    gp->IsScreenBar=FALSE;
    gp->IsKeyboardEmul=FALSE;
    gp->IsNoLAmigaKey=FALSE;
    gp->IsViewOnly=FALSE;

    gp->IsOverlay=FALSE;
    gp->OverlayThreshold=100;

    gp->IsSmoothed=FALSE;
    gp->IsStartIconify=FALSE;
    gp->Left=-1;
    gp->Top=-1;
    gp->Width=-1;
    gp->Height=-1;
    gp->ControlKey=DEFAULT_CONTROLKEY;
    gp->Flag=FALSE;

    gp->TimeOut=60;
    gp->IsAltIconifyIcon=TRUE;

    Sys_StrCopy(gp->FontName,"topaz.font",sizeof(gp->FontName));
    gp->FontSize=8;
}


/*****
    Définition des paramètres VNC par défaut pour le démarrage de la connexion
*****/

void Cfg_InitDefaultProtocolPrefs(struct ProtocolPrefs *pp)
{
    pp->IsShared=TRUE;          /* Connexion en mode partage */
    pp->EncoderId=7;            /* Codec Raw par défaut du serveur */
    pp->Depth=0;                /* PixelFormat du serveur */
    pp->ServerScale=1;          /* Scale par défaut du serveur */
    pp->IsChangeZLib=FALSE;     /* Prend le taux de compression du serveur */
    pp->ZLibLevel=9;            /* Facultatif puisque IsChangeZLib=FALSE */
    pp->IsUseJpeg=FALSE;        /* Désactivation par defaut de l'option Jpeg du codec Tight */
    pp->JpegQuality=5;          /* Facultatif puisque IsUseJpeg=FALSE */
    pp->IsNoLocalCursor=TRUE;   /* Pas de gestion locale du curseur */
    pp->IsNoCopyRect=TRUE;      /* Pas de copyrect */
    pp->IsReportMouse=TRUE;     /* Report des mouvements de la souris du serveur */
}


/*****
    Lecture des informations provenant du cli ou de l'icêne
*****/

LONG Cfg_ReadConfig(struct GlobalPrefs *gp, struct ProtocolPrefs *pp, void *Arg, char **AppPath, char *Host, ULONG *Port, char *Password, BOOL *IsHost, BOOL *IsPassword)
{
    LONG Error;

    /* Initialisation de la config */
    Cfg_Init(gp,pp,Host,Port,Password);

    /* Lecture de la config */
    *AppPath=NULL;
    if(Cli()!=NULL)
    {
        Error=Cfg_ReadConfigCli(gp,pp,(char **)Arg,AppPath,Host,Port,Password,IsHost,IsPassword);
    }
    else
    {
        Error=Cfg_ReadConfigIcon(gp,pp,(struct WBStartup *)Arg,AppPath,Host,Port,Password,IsHost,IsPassword);
    }

    /* Quelques tests de cohérence */
    if(pp->Depth<3) pp->Depth=0;
    else if(pp->Depth==3) pp->Depth=3;
    else if(pp->Depth<=6) pp->Depth=6;
    else if(pp->Depth<=8) pp->Depth=8;
    else if(pp->Depth<=16) pp->Depth=16;
    else pp->Depth=32;

    if(pp->ZLibLevel>9) pp->ZLibLevel=9;
    if(pp->JpegQuality>9) pp->JpegQuality=9;

    if(gp->FontSize<1) gp->FontSize=1;

    return Error;
}


/*****
    Ecriture de la config sur le disque
*****/

BOOL Cfg_WriteConfig(const struct GlobalPrefs *gp, const struct ProtocolPrefs *pp, const char *AppPath, const char *Host, ULONG Port, const char *Password, BOOL IsForcePassword, const char *FileName)
{
    BOOL IsSuccess=FALSE;
    static const LONG ArgList[]=
    {
        ARG_HOST,       ARG_PORT,       ARG_SPASS,      ARG_ENCODERID,
        ARG_MODEID,     ARG_MODEDEPTH,  ARG_DEPTH,      ARG_SERVERSCALE,
        ARG_ZLIB,       ARG_JPEG,       ARG_OVERLAY,    ARG_LEFT,
        ARG_TOP,        ARG_WIDTH,      ARG_HEIGHT,     ARG_FONTNAME,
        ARG_FONTSIZE,   ARG_CTRLKEY
    };
    struct DiskObject *icon=(gp->AltIconifyIcon!=NULL)?gp->AltIconifyIcon:&AppIconDObj;
    ICOSTR *CurTTPtr=icon->do_ToolTypes;
    LONG Count=0,BufLen,i;
    UBYTE *Buffer;

    /* On compte le nombre de tooltypes actuellement présents dans l'icône courant */
    if(CurTTPtr!=NULL)
    {
        for(Count=0;CurTTPtr[Count]!=NULL;Count++);
    }

    /* On compte la longueur de chaque argument */
    BufLen=(2*Count+COUNTOF_ARGS+2)*sizeof(char *) +
           SIZEOF_HOST_BUFFER +
           (14*16+20+(MAXFONTNAME+2))*sizeof(char); /* 14 entiers + SPASS + FONTNAME */
    for(i=0;i<sizeof(ArgList)/sizeof(LONG);i++)
    {
        BufLen+=Sys_StrLen(ToolTypes[ArgList[i]]);
    }

    /* Allocation d'un buffer pour manipuler les tooltypes */
    if((Buffer=(UBYTE *)Sys_AllocMem(BufLen))!=NULL)
    {
        ICOSTR *NewTTTable=(ICOSTR *)Buffer;
        ICOSTR *CurTTTable=&NewTTTable[Count+COUNTOF_ARGS+1];
        ICOSTR BufCursorPtr=(char *)&CurTTTable[Count+1];
        ICOSTR *Ptr=NewTTTable;
        static const ICOSTR FakeTT="";
        BOOL IsKnownTooltype=TRUE;

        /* Elimination des Tooltypes connus dans l'icône */
        for(i=0;i<Count;i++) CurTTTable[i]=CurTTPtr[i];
        CurTTTable[i]=NULL;
        while(IsKnownTooltype)
        {
            /* Scan de tous les arguments supportés par TwinVNC */
            IsKnownTooltype=FALSE;
            for(i=0;i<COUNTOF_ARGS;i++)
            {
                ICOSTR TTValue=FindToolType(CurTTTable,(ICOSTR)ToolTypes[i]);

                if(TTValue!=NULL)
                {
                    LONG j;

                    /* Identification du tooltype dans la table et élimination de celui-ci */
                    for(j=0;j<Count;j++)
                    {
                        ICOSTR TTKey=CurTTTable[j];
                        if(TTValue>=TTKey && TTValue<=&TTKey[Sys_StrLen((char *)TTKey)])
                        {
                            IsKnownTooltype=TRUE;
                            CurTTTable[j]=(ICOSTR)FakeTT;
                            break;
                        }
                    }
                }
            }
        }

        /* HOST=s */
        if(Sys_StrLen(Host)>0)
        {
            *(Ptr++)=Cfg_SetToolTypeString((char **)&BufCursorPtr,ToolTypes[ARG_HOST],Host);
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_PORT],(LONG)(UWORD)Port);
        }

        /* SPASS=s */
        if(Sys_StrLen(Password)>0 || IsForcePassword)
        {
            char EncryptedPass[20];
            Cfg_CryptPass(Password,EncryptedPass);
            *(Ptr++)=Cfg_SetToolTypeString((char **)&BufCursorPtr,ToolTypes[ARG_SPASS],EncryptedPass);
        }

        /* ENCODERID=n */
        if(pp->EncoderId>0 && pp->EncoderId<=99)
        {
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_ENCODERID],(LONG)(pp->EncoderId&63));
        }

        /* ASKMODEID */
        if(gp->ScrModeSelectType==SCRMODE_SELECT_ASK) *(Ptr++)=(ICOSTR)ToolTypes[ARG_ASKMODEID];

        /* MODEID=n */
        if(gp->ScrModeID!=INVALID_ID && gp->ScrModeSelectType==SCRMODE_SELECT_CURRENT)
        {
            *(Ptr++)=Cfg_SetToolTypeULong((char **)&BufCursorPtr,ToolTypes[ARG_MODEID],(ULONG)gp->ScrModeID);
        }

        /* MODEDEPTH=n */
        if(gp->ScrDepth!=0)
        {
            *(Ptr++)=Cfg_SetToolTypeULong((char **)&BufCursorPtr,ToolTypes[ARG_MODEDEPTH],(ULONG)gp->ScrDepth);
        }

        /* DEPTH=n */
        if(pp->Depth>0 && pp->Depth<=32)
        {
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_DEPTH],(LONG)(pp->Depth&63));
        }

        /* SERVERSCALE=n */
        if(pp->ServerScale>1 && pp->ServerScale<=99)
        {
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_SERVERSCALE],(LONG)(UWORD)pp->ServerScale);
        }

        /* ZLIB=n */
        if(pp->IsChangeZLib && pp->ZLibLevel>=0 && pp->ZLibLevel<=99)
        {
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_ZLIB],(LONG)(pp->ZLibLevel&63));
        }

        /* JPEG=n */
        if(pp->IsUseJpeg && pp->JpegQuality>=0 && pp->JpegQuality<=99)
        {
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_JPEG],(LONG)(pp->JpegQuality&63));
        }

        /* CURSOR */
        if(gp->IsCursor) *(Ptr++)=(ICOSTR)ToolTypes[ARG_CURSOR];

        /* NOLOCAL */
        if(pp->IsNoLocalCursor) *(Ptr++)=(ICOSTR)ToolTypes[ARG_NOLOCAL];

        /* NOCOPYRECT */
        if(pp->IsNoCopyRect) *(Ptr++)=(ICOSTR)ToolTypes[ARG_NOCOPYRECT];

        /* NOCLIPBOARD */
        if(gp->IsNoClipboard) *(Ptr++)=(ICOSTR)ToolTypes[ARG_NOCLIPBOARD];

        /* NOREPORTMOUSE */
        if(!pp->IsReportMouse) *(Ptr++)=(ICOSTR)ToolTypes[ARG_NOREPORTMOUSE];

        /* NODISPLAY */
        if(gp->DisplayOption==DISPLAY_OPTION_NODISPLAY) *(Ptr++)=(ICOSTR)ToolTypes[ARG_NODISPLAY];

        /* WINDOW */
        if(gp->DisplayOption==DISPLAY_OPTION_WINDOW) *(Ptr++)=(ICOSTR)ToolTypes[ARG_WINDOW];

        /* TOOLBAR */
        if(gp->IsShowToolBar) *(Ptr++)=(ICOSTR)ToolTypes[ARG_TOOLBAR];

        /* SCREENBAR */
        if(gp->IsScreenBar) *(Ptr++)=(ICOSTR)ToolTypes[ARG_SCREENBAR];

        /* KBEMUL */
        if(gp->IsKeyboardEmul) *(Ptr++)=(ICOSTR)ToolTypes[ARG_KBEMUL];

        /* NOLAMIGAKEY */
        if(gp->IsNoLAmigaKey) *(Ptr++)=(ICOSTR)ToolTypes[ARG_NOLAMIGAKEY];

        /* NOSHARED */
        if(!pp->IsShared) *(Ptr++)=(ICOSTR)ToolTypes[ARG_NOSHARED];

        /* VIEWONLY */
        if(gp->IsViewOnly) *(Ptr++)=(ICOSTR)ToolTypes[ARG_VIEWONLY];

        /* OVERLAY=n */
        if(gp->IsOverlay && gp->OverlayThreshold>0 && gp->OverlayThreshold<=9999)
        {
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_OVERLAY],(LONG)(UWORD)gp->OverlayThreshold);
        }

        /* SCALE */
        if(gp->ViewOption==VIEW_OPTION_SCALE) *(Ptr++)=(ICOSTR)ToolTypes[ARG_SCALE];

        /* SMOOTH */
        if(gp->IsSmoothed) *(Ptr++)=(ICOSTR)ToolTypes[ARG_SMOOTH];

        /* ICONIFY */
        if(gp->IsStartIconify) *(Ptr++)=(ICOSTR)ToolTypes[ARG_ICONIFY];

        /* LEFT */
        if(gp->Left>=0)
        {
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_LEFT],(LONG)(WORD)gp->Left);
        }

        /* TOP */
        if(gp->Top>=0)
        {
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_TOP],(LONG)(WORD)gp->Top);
        }

        /* WIDTH */
        if(gp->Width>=0)
        {
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_WIDTH],(LONG)(WORD)gp->Width);
        }

        /* HEIGHT */
        if(gp->Height>=0)
        {
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_HEIGHT],(LONG)(WORD)gp->Height);
        }

        /* FONTNAME */
        if(Sys_StrLen(gp->FontName)>0)
        {
            *(Ptr++)=Cfg_SetToolTypeString((char **)&BufCursorPtr,ToolTypes[ARG_FONTNAME],gp->FontName);
            *(Ptr++)=Cfg_SetToolTypeLong((char **)&BufCursorPtr,ToolTypes[ARG_FONTSIZE],(LONG)(UWORD)gp->FontSize);
        }

        /* CTRLKEY */
        if(gp->ControlKey!=0)
        {
            char Tmp[128];
            ULONG Qualifier=Cfg_GetControlKeyOptionText(gp->ControlKey,Tmp);
            if(Qualifier!=0) *(Ptr++)=Cfg_SetToolTypeString((char **)&BufCursorPtr,ToolTypes[ARG_CTRLKEY],Tmp);
        }

        /* FLAG */
        if(gp->Flag) *(Ptr++)=(ICOSTR)ToolTypes[ARG_FLAG];

        /* Ajout des autres tooltypes */
        for(i=0;i<Count;i++)
        {
            if(CurTTTable[i]!=FakeTT) *(Ptr++)=CurTTTable[i];
        }

        *Ptr=NULL;

        /* Enregistrement de l'icône */
        {
            UBYTE PrevType=icon->do_Type;
            ICOSTR PrevDefaultTool=icon->do_DefaultTool;
            ICOSTR *PrevToolTypes=icon->do_ToolTypes;
            LONG PrevCurrentX=icon->do_CurrentX;
            LONG PrevCurrentY=icon->do_CurrentY;
            LONG PrevStackSize=icon->do_StackSize;
            ICOSTR CurrentAppPath=(ICOSTR)AppPath;

            /* Recuperation du chemin de l'app dans l'icône courant */
            if(PrevDefaultTool!=NULL && icon->do_Type==WBPROJECT)
            {
                if(Sys_StrLen((char *)PrevDefaultTool)>0) CurrentAppPath=PrevDefaultTool;
            }

            icon->do_Type=WBPROJECT;
            icon->do_DefaultTool=CurrentAppPath;
            icon->do_ToolTypes=NewTTTable;
            icon->do_CurrentX=NO_ICON_POSITION;
            icon->do_CurrentY=NO_ICON_POSITION;
            icon->do_StackSize=__DEFAULT_STACK_SIZE__;

            IsSuccess=PutDiskObject((ICOSTR)FileName,icon);

            icon->do_Type=PrevType;
            icon->do_DefaultTool=PrevDefaultTool;
            icon->do_ToolTypes=PrevToolTypes;
            icon->do_CurrentX=PrevCurrentX;
            icon->do_CurrentY=PrevCurrentY;
            icon->do_StackSize=PrevStackSize;
        }

        Sys_FreeMem((void *)Buffer);
    }

    return IsSuccess;
}


/*****
    Lecture des informations provenant du cli
*****/

LONG Cfg_ReadConfigCli(struct GlobalPrefs *gp, struct ProtocolPrefs *pp, char **Arg, char **AppPath, char *Host, ULONG *Port, char *Password, BOOL *IsHost, BOOL *IsPassword)
{
    LONG Error=0,i,MyPars[COUNTOF_ARGS];
    struct RDArgs *RDArgs;

    *AppPath=Cfg_GetProgramPathFromCli();
    *IsHost=FALSE;
    *IsPassword=FALSE;

    /* Lecture des arguments */
    for(i=0; i<COUNTOF_ARGS; i++) MyPars[i]=0;
    if((RDArgs=ReadArgs((STRPTR)Arguments,MyPars,NULL))!=NULL)
    {
        if(MyPars[ARG_HOST])
        {
            Sys_StrCopy(Host,(char *)MyPars[ARG_HOST],SIZEOF_HOST_BUFFER);
            *IsHost=TRUE;
        }
        if(MyPars[ARG_PORT]) *Port=*((ULONG *)MyPars[ARG_PORT]);

        /* Note: on traite d'abord le mot de passe sécurisé car il est moins prioritaire que l'autre */
        if(MyPars[ARG_SPASS])
        {
            Cfg_UnCryptPass((char *)MyPars[ARG_SPASS],Password);
            *IsPassword=TRUE;
        }
        if(MyPars[ARG_PASS])
        {
            Sys_StrCopy(Password,(char *)MyPars[ARG_PASS],SIZEOF_PASSWORD_BUFFER);
            *IsPassword=TRUE;
        }

        if(MyPars[ARG_ENCODERID]) pp->EncoderId=*((ULONG *)MyPars[ARG_ENCODERID]);

        /* Note: on traite l'argument MODEID avant ASKMODEID car il est moins prioritaire que ce dernier */
        if(MyPars[ARG_MODEID])
        {
            gp->ScrModeSelectType=SCRMODE_SELECT_CURRENT;
            gp->ScrModeID=*((ULONG *)MyPars[ARG_MODEID]);
        }

        if(MyPars[ARG_MODEDEPTH]) gp->ScrDepth=*((ULONG *)MyPars[ARG_MODEDEPTH]);

        /* Note: on traite le flag ASKMODEID après l'argument MODEID pour pouvoir écraser le type */
        if(MyPars[ARG_ASKMODEID]) gp->ScrModeSelectType=SCRMODE_SELECT_ASK;

        if(MyPars[ARG_DEPTH]) pp->Depth=*((ULONG *)MyPars[ARG_DEPTH]);
        if(MyPars[ARG_SERVERSCALE]) pp->ServerScale=*((ULONG *)MyPars[ARG_SERVERSCALE]);

        if(MyPars[ARG_ZLIB])
        {
            pp->IsChangeZLib=TRUE;
            pp->ZLibLevel=*((ULONG *)MyPars[ARG_ZLIB]);
        }

        if(MyPars[ARG_JPEG])
        {
            pp->IsUseJpeg=TRUE;
            pp->JpegQuality=*((ULONG *)MyPars[ARG_JPEG]);
        }

        if(MyPars[ARG_CURSOR]) gp->IsCursor=TRUE;
        if(MyPars[ARG_NOLOCAL]) pp->IsNoLocalCursor=TRUE;
        if(MyPars[ARG_NOCOPYRECT]) pp->IsNoCopyRect=TRUE;
        if(MyPars[ARG_NOREPORTMOUSE]) pp->IsReportMouse=FALSE;
        if(MyPars[ARG_NOCLIPBOARD]) gp->IsNoClipboard=TRUE;
        if(MyPars[ARG_WINDOW]) gp->DisplayOption=DISPLAY_OPTION_WINDOW;
        if(MyPars[ARG_NODISPLAY]) gp->DisplayOption=DISPLAY_OPTION_NODISPLAY;
        if(MyPars[ARG_TOOLBAR]) gp->IsShowToolBar=TRUE;
        if(MyPars[ARG_SCREENBAR]) gp->IsScreenBar=TRUE;
        if(MyPars[ARG_KBEMUL]) gp->IsKeyboardEmul=TRUE;
        if(MyPars[ARG_NOLAMIGAKEY]) gp->IsNoLAmigaKey=TRUE;
        if(MyPars[ARG_NOSHARED]) pp->IsShared=FALSE;
        if(MyPars[ARG_VIEWONLY]) gp->IsViewOnly=TRUE;

        if(MyPars[ARG_OVERLAY])
        {
            LONG OverlayThreshold=*((LONG *)MyPars[ARG_OVERLAY]);
            if(OverlayThreshold>=0)
            {
                gp->IsOverlay=TRUE;
                gp->OverlayThreshold=OverlayThreshold;
            }
        }

        if(MyPars[ARG_SCALE]) gp->ViewOption=VIEW_OPTION_SCALE;
        if(MyPars[ARG_SMOOTH]) gp->IsSmoothed=TRUE;
        if(MyPars[ARG_ICONIFY]) gp->IsStartIconify=TRUE;
        if(MyPars[ARG_LEFT]) gp->Left=*((LONG *)MyPars[ARG_LEFT]);
        if(MyPars[ARG_TOP]) gp->Top=*((LONG *)MyPars[ARG_TOP]);
        if(MyPars[ARG_WIDTH]) gp->Width=*((LONG *)MyPars[ARG_WIDTH]);
        if(MyPars[ARG_HEIGHT]) gp->Height=*((LONG *)MyPars[ARG_HEIGHT]);

        if(MyPars[ARG_FONTNAME]) Sys_StrCopy(gp->FontName,(char *)MyPars[ARG_FONTNAME],sizeof(gp->FontName));
        if(MyPars[ARG_FONTSIZE]) gp->FontSize=*((LONG *)MyPars[ARG_FONTSIZE]);

        if(MyPars[ARG_CTRLKEY])
        {
            ULONG Qualifier=Cfg_ParseControlKeyOption((const char *)MyPars[ARG_CTRLKEY]);
            if(Qualifier!=0) gp->ControlKey=Qualifier;
        }

        if(MyPars[ARG_CRYPT])
        {
            char Tmp[24];
            Cfg_CryptPass((char *)MyPars[ARG_CRYPT],Tmp);
            Sys_Printf("%s\n",Tmp);
            Error=-1;
        }

        if(MyPars[ARG_FLAG]) gp->Flag=TRUE;

        /* On recupère, si possible, un pointeur sur l'icône qui servira pour l'iconification */
        if(gp->IsAltIconifyIcon) gp->AltIconifyIcon=GetDiskObject(Arg[0]);

        FreeArgs(RDArgs);
    } else Error=IoErr();

    return Error;
}


/*****
    Lecture des informations d'un icône
*****/

LONG Cfg_ReadConfigIcon(struct GlobalPrefs *gp, struct ProtocolPrefs *pp, struct WBStartup *Arg, char **AppPath, char *Host, ULONG *Port, char *Password, BOOL *IsHost, BOOL *IsPassword)
{
    LONG Error=0;

    if(Arg->sm_NumArgs>0)
    {
        struct WBArg *wbarg=&Arg->sm_ArgList[Arg->sm_NumArgs-1];
        BPTR oldlock=CurrentDir(wbarg->wa_Lock);
        struct DiskObject *icon;

        *AppPath=Cfg_GetProgramPathFromIcon(wbarg->wa_Lock,wbarg->wa_Name);

        if((icon=GetDiskObject(wbarg->wa_Name))!=NULL)
        {
            ICOSTR *Ptr=icon->do_ToolTypes;
            ICOSTR *TTName=(ICOSTR *)ToolTypes; /* Conversion de type pour les warnings de compile MOS vs AOS */
            ICOSTR tt;

            /* Initialisation de la config: On prend les tooltypes du dernier icône loadé */
            *IsHost=FALSE;
            *IsPassword=FALSE;
            Cfg_Init(gp,pp,Host,Port,Password);

            if((tt=FindToolType(Ptr,TTName[ARG_HOST]))!=NULL)
            {
                if(Sys_StrLen((char *)tt)>0)
                {
                    Sys_StrCopy(Host,(char *)tt,SIZEOF_HOST_BUFFER);
                    *IsHost=TRUE;
                }
            }
            Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_PORT]),(LONG *)Port);

            /* Note: on traite d'abord le mot de passe sécurisé car il est moins prioritaire que l'autre */
            if((tt=FindToolType(Ptr,TTName[ARG_SPASS]))!=NULL)
            {
                if(Sys_StrLen((char *)tt)>0) Cfg_UnCryptPass((char *)tt,Password);
                *IsPassword=TRUE;
            }
            if((tt=FindToolType(Ptr,TTName[ARG_PASS]))!=NULL)
            {
                Sys_StrCopy(Password,(char *)tt,SIZEOF_PASSWORD_BUFFER);
                *IsPassword=TRUE;
            }

            Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_ENCODERID]),(LONG *)&pp->EncoderId);

            /* Note: on traite l'argument MODEID avant ASKMODEID car il est moins prioritaire que ce dernier */
            if(Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_MODEID]),(LONG *)&gp->ScrModeID))
            {
                gp->ScrModeSelectType=SCRMODE_SELECT_CURRENT;
            }

            Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_MODEDEPTH]),(LONG *)&gp->ScrDepth);

            /* Note: on traite le flag ASKMODEID après l'argument MODEID pour pouvoir écraser le type */
            if(FindToolType(Ptr,TTName[ARG_ASKMODEID])!=NULL)
            {
                gp->ScrModeSelectType=SCRMODE_SELECT_ASK;
            }

            Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_DEPTH]),(LONG *)&pp->Depth);
            Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_SERVERSCALE]),(LONG *)&pp->ServerScale);

            if(Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_ZLIB]),(LONG *)&pp->ZLibLevel))
            {
                pp->IsChangeZLib=TRUE;
            }

            if(Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_JPEG]),(LONG *)&pp->JpegQuality))
            {
                pp->IsUseJpeg=TRUE;
            }

            if(FindToolType(Ptr,TTName[ARG_CURSOR])!=NULL) gp->IsCursor=TRUE;
            if(FindToolType(Ptr,TTName[ARG_NOLOCAL])!=NULL) pp->IsNoLocalCursor=TRUE;
            if(FindToolType(Ptr,TTName[ARG_NOCOPYRECT])!=NULL) pp->IsNoCopyRect=TRUE;
            if(FindToolType(Ptr,TTName[ARG_NOREPORTMOUSE])!=NULL) pp->IsReportMouse=FALSE;
            if(FindToolType(Ptr,TTName[ARG_NOCLIPBOARD])!=NULL) gp->IsNoClipboard=TRUE;
            if(FindToolType(Ptr,TTName[ARG_WINDOW])!=NULL) gp->DisplayOption=DISPLAY_OPTION_WINDOW;
            if(FindToolType(Ptr,TTName[ARG_NODISPLAY])!=NULL) gp->DisplayOption=DISPLAY_OPTION_NODISPLAY;
            if(FindToolType(Ptr,TTName[ARG_TOOLBAR])!=NULL) gp->IsShowToolBar=TRUE;
            if(FindToolType(Ptr,TTName[ARG_SCREENBAR])!=NULL) gp->IsScreenBar=TRUE;
            if(FindToolType(Ptr,TTName[ARG_KBEMUL])!=NULL) gp->IsKeyboardEmul=TRUE;
            if(FindToolType(Ptr,TTName[ARG_NOLAMIGAKEY])!=NULL) gp->IsNoLAmigaKey=TRUE;
            if(FindToolType(Ptr,TTName[ARG_NOSHARED])!=NULL) pp->IsShared=FALSE;
            if(FindToolType(Ptr,TTName[ARG_VIEWONLY])!=NULL) gp->IsViewOnly=TRUE;

            {
                LONG OverlayThreshold=0;
                if(Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_OVERLAY]),&OverlayThreshold))
                {
                    gp->IsOverlay=TRUE;
                    if(OverlayThreshold>=0) gp->OverlayThreshold=OverlayThreshold;
                }
            }

            if(FindToolType(Ptr,TTName[ARG_SCALE])!=NULL) gp->ViewOption=VIEW_OPTION_SCALE;
            if(FindToolType(Ptr,TTName[ARG_SMOOTH])!=NULL) gp->IsSmoothed=TRUE;
            if(FindToolType(Ptr,TTName[ARG_ICONIFY])!=NULL) gp->IsStartIconify=TRUE;

            Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_LEFT]),(LONG *)&gp->Left);
            Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_TOP]),(LONG *)&gp->Top);
            Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_WIDTH]),(LONG *)&gp->Width);
            Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_HEIGHT]),(LONG *)&gp->Height);

            if((tt=FindToolType(Ptr,TTName[ARG_FONTNAME]))!=NULL)
            {
                Sys_StrCopy(gp->FontName,(char *)tt,sizeof(gp->FontName));
            }

            Cfg_SetLongFromString((char *)FindToolType(Ptr,TTName[ARG_FONTSIZE]),(LONG *)&gp->FontSize);

            if((tt=FindToolType(Ptr,TTName[ARG_CTRLKEY]))!=NULL)
            {
                ULONG Qualifier=Cfg_ParseControlKeyOption(tt);
                if(Qualifier!=0) gp->ControlKey=Qualifier;
            }

            if(FindToolType(Ptr,TTName[ARG_FLAG])!=NULL) gp->Flag=TRUE;

            /* On recupère, si possible, un pointeur sur l'icône qui servira pour l'iconification */
            if(!gp->IsAltIconifyIcon) FreeDiskObject(icon); else gp->AltIconifyIcon=icon;
        } else Error=IoErr();

        CurrentDir(oldlock);
    }

    return Error;
}


/*****
    Permet de convertir l'option Control Key texte en valeur numérique
*****/

ULONG Cfg_ParseControlKeyOption(const char *Val)
{
    ULONG Qualifier=0;
    char Tmp[16];

    while(*Val!=0)
    {
        LONG i;
        for(i=0; i<sizeof(Tmp)/sizeof(char)-1 && *Val!=0 && *Val!='+' && *Val!=' '; i++) Tmp[i]=*(Val++);
        Tmp[i]=0;

        for(i=0; i<sizeof(QualName)/sizeof(const char *); i++)
        {
            if(Sys_StrCmpNoCase(Tmp,QualName[i])==0) Qualifier|=QualList[i];
        }

        while(*Val!=0 && (*Val==' ' || *Val=='+')) Val++;
    }

    return Qualifier;
}


/*****
    Convertit le Control Key numérique en version texte
*****/

ULONG Cfg_GetControlKeyOptionText(ULONG Qualifier, char *Buffer)
{
    ULONG QualResult=0;
    LONG i;

    Buffer[0]=0;
    for(i=0; i<sizeof(QualList)/sizeof(ULONG); i++)
    {
        if((Qualifier&QualList[i])!=0)
        {
            char *Tmp=&Buffer[Sys_StrLen(Buffer)];
            if(Tmp!=Buffer) *(Tmp++)='+';
            Sys_StrCopy(Tmp,QualName[i],100);
            QualResult|=QualList[i];
        }
    }

    return QualResult;
}


/*****
    Conversion d'une chaîne en LONG.
    Une chaîne NULL est acceptée et non convertie
*****/

BOOL Cfg_SetLongFromString(const char *Src, LONG *Dst)
{
    if(Src!=NULL)
    {
        LONG Tmp;
        if(StrToLong((STRPTR)Src,&Tmp)!=-1)
        {
            *Dst=Tmp;
            return TRUE;
        }
    }

    return FALSE;
}


/*****
    Permet d'ajouter une chaîne de type Clef/Valeur chaîne dans le buffer.
    Retourne le pointeur sur cette chaîne. Le pointeur sur le buffer est
    auto incrémenté.
*****/

char *Cfg_SetToolTypeString(char **Buffer, const char *TTName, const char *TTValue)
{
    char *Ptr=*Buffer;
    Sys_SPrintf(Ptr,"%s=%s",TTName,TTValue);
    *Buffer+=Sys_StrLen(Ptr)+sizeof(char);
    return Ptr;
}


/*****
    Permet d'ajouter une chaîne de type Clef/Valeur Long dans le buffer.
    Retourne le pointeur sur cette chaîne. Le pointeur sur le buffer est
    auto incrémenté.
*****/

char *Cfg_SetToolTypeLong(char **Buffer, const char *TTName, LONG TTValue)
{
    char *Ptr=*Buffer;
    Sys_SPrintf(Ptr,"%s=%ld",TTName,TTValue);
    *Buffer+=Sys_StrLen(Ptr)+sizeof(char);
    return Ptr;
}


/*****
    Permet d'ajouter une chaîne de type Clef/Valeur ULong dans le buffer.
    Retourne le pointeur sur cette chaîne. Le pointeur sur le buffer est
    auto incrémenté.
*****/

char *Cfg_SetToolTypeULong(char **Buffer, const char *TTName, ULONG TTValue)
{
    char *Ptr=*Buffer;
    Sys_SPrintf(Ptr,"%s=%lu",TTName,TTValue);
    *Buffer+=Sys_StrLen(Ptr)+sizeof(char);
    return Ptr;
}


/*****
    Retourne le chemin complet du programme.
    Note: Le chemin doit être désalloué par Sys_FreeMem()
*****/

char *Cfg_GetProgramPathFromCli(void)
{
    BOOL IsSuccess=FALSE;
    LONG DirLen=512,ProgLen=512;
    char *PathName=Sys_AllocMem((DirLen+ProgLen)*sizeof(char));

    if(PathName!=NULL)
    {
        BPTR LockDir=GetProgramDir();

        /* On cherche d'abord à obtenir le chemin de l'application */
        if(NameFromLock(LockDir,PathName,DirLen))
        {
            char *ProgName=&PathName[DirLen];

            /* Puis on cherche à obtenir le nom de l'application */
            if(GetProgramName(ProgName,ProgLen))
            {
                AddPart(PathName,FilePart(ProgName),DirLen+ProgLen);
                IsSuccess=TRUE;
            }
        }
    }

    if(PathName!=NULL && !IsSuccess)
    {
        Sys_FreeMem((void *)PathName);
        PathName=NULL;
    }

    return PathName;
}


/*****
    Retourne le chemin complet du programme.
    Note: Le chemin doit être désalloué par Sys_FreeMem()
*****/

char *Cfg_GetProgramPathFromIcon(BPTR lock, const char *Name)
{
    BOOL IsSuccess=FALSE;
    LONG DirLen=512,ProgLen=Sys_StrLen(Name)+sizeof(char);
    char *PathName=Sys_AllocMem((DirLen+ProgLen)*sizeof(char));

    if(PathName!=NULL)
    {
        /* On cherche d'abord à obtenir le chemin de l'application */
        if(NameFromLock(lock,PathName,DirLen))
        {
            AddPart(PathName,FilePart((STRPTR)Name),DirLen+ProgLen);
            IsSuccess=TRUE;
        }
    }

    if(PathName!=NULL && !IsSuccess)
    {
        Sys_FreeMem((void *)PathName);
        PathName=NULL;
    }

    return PathName;
}


/*****
    Cryptage du mot de passe.
    Src: buffer contenant la chaîne à crypter (seuls les 8 premiers caractères sont pris en compte
    Dst: buffer de 16+1 caractères pour contenir la chaîne cryptée
*****/

void Cfg_CryptPass(const char *Src, char *Dst)
{
    LONG Len=Sys_StrLen(Src),i;
    unsigned char Tmp[8];

    for(i=0;i<8;i++) Tmp[i]=(i<Len?Src[i]:0);
    deskey(FixedKey,EN0);
    des(Tmp,Tmp);

    for(i=0;i<8;i++)
    {
        UBYTE Value=Tmp[i];
        ULONG Low=Value&0x0f;
        ULONG Hig=(Value>>4)&0x0f;
        *(Dst++)=PassTable[Low];
        *(Dst++)=PassTable[Hig];
    }
    *Dst=0;
}


/*****
    Décryptage d'une chaîne cryptée par Cfg_CryptPass()
    Src: Chaîne de 16 caractères maxi
    Dst: Buffer de 8+1 caractères
*****/

void Cfg_UnCryptPass(const char *Src, char *Dst)
{
    LONG Len=Sys_StrLen(Src),i;
    unsigned char Tmp[16];

    for(i=0;i<16;i++) Tmp[i]=(i<Len?Src[i]:0);

    for(i=0;i<8;i++)
    {
        unsigned char Char1=Tmp[i*2];
        unsigned char Char2=Tmp[i*2+1];
        LONG Low=-1,Hig=-1,j;

        for(j=0;j<16;j++)
        {
            if(PassTable[j]==Char1) Low=j;
            if(PassTable[j]==Char2) Hig=j;
        }
        Dst[i]=(Low>=0 && Hig>=0?((Hig&0x0f)<<4)+(Low&0x0f):0);
    }

    deskey(FixedKey,DE1);
    des((unsigned char *)Dst,(unsigned char *)Dst);
    Dst[8]=0;
}
