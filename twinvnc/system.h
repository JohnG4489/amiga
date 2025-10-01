#ifndef SYSTEM_H
#define SYSTEM_H

#define PLATFORM_AMIGA 1

#ifndef PLATFORM_PC
#include <exec/exec.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/gadtools.h>
#include <proto/keymap.h>
#include <proto/utility.h>
#include <proto/asl.h>
#include <proto/icon.h>
#include <proto/locale.h>
#include <proto/wb.h>
#include <proto/diskfont.h>
#include <proto/codesets.h>
#ifdef __amigaos4__ /* sur GCC/OS4 */
#include <cybergraphics/cgxvideo.h>
#include <proto/cgxvideo.h>
#endif
#ifdef __MORPHOS__  /* sur GCC/MorphOS */
#include <cybergraphx/cgxvideo.h>
#include <proto/cgxvideo.h>
#endif
#if !defined(__amigaos4__) && !defined(__MORPHOS__) /* sur SASC/68k */
#include <pragmas/cgxvideo_pragmas.h>
#include <cybergraphx/cgxvideo.h>
#include <clib/cgxvideo_protos.h>
#endif
#include <devices/clipboard.h>
#endif
#include "compatibility.h"


#if !defined(__amigaos4__) && !defined(__MORPHOS__)
#define OSID_AMIGAOS3   0
#define OSID_AMIGAOS4   1
#define OSID_MORPHOS    2
#endif


/* quelques defines definis pour un portage sur une autre plateforme */
#ifdef PLATFORM_PC
typedef long            LONG;       /* signed 32-bit quantity */
typedef unsigned long   ULONG;      /* unsigned 32-bit quantity */
typedef short           WORD;       /* signed 16-bit quantity */
typedef unsigned short  UWORD;      /* unsigned 16-bit quantity */
typedef char            BYTE;       /* signed 8-bit quantity */
typedef unsigned char   UBYTE;      /* unsigned 8-bit quantity */
typedef char           *STRPTR;     /* string pointer (NULL terminated) */
typedef void            VOID;
typedef float           FLOAT;
typedef double          DOUBLE;
typedef long            BOOL;
#endif
#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif

/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

struct Twin;

#if !defined(__amigaos4__) && !defined(__MORPHOS__)
extern ULONG OperatingSystemID;
#endif

#ifndef PLATFORM_PC
/* declarations necessaires pour amigaos4 */
#ifdef __amigaos4__
extern struct Library *CyberGfxBase;        /* ouvert par le projet */
extern struct Library *GadtoolsBase;        /* ouvert par le projet */
extern struct Library *KeymapBase;          /* ouvert par le projet */
extern struct Library *CGXVideoBase;        /* ouvert par le projet */
extern struct Catalog *CatalogBase;
#endif

/* declarations necessaires pour OS3.1 et MorphOS */
#if !defined(__amigaos4__) || defined(__MORPHOS__)
extern struct ExecBase *SysBase;
extern struct IntuitionBase *IntuitionBase; /* ouvert par SAS/C */
extern struct DosLibrary *DOSBase;          /* ouvert par SAS/C */
extern struct GfxBase *GfxBase;             /* ouvert par le projet */
extern struct Library *UtilityBase;         /* ouvert par le projet */
extern struct Library *AslBase;             /* ouvert par le projet */
extern struct Library *IconBase;            /* ouvert par le projet */
extern struct Library *LocaleBase;          /* ouvert par le projet */
extern struct Library *WorkbenchBase;       /* ouvert par le projet */
extern struct Library *CyberGfxBase;        /* ouvert par le projet */
extern struct Library *GadToolsBase;        /* ouvert par le projet */
extern struct Library *KeymapBase;          /* ouvert par le projet */
extern struct Library *CGXVideoBase;        /* ouvert par le projet */
extern struct Library *CodesetsBase;        /* ouvert par le projet */
extern struct Catalog *CatalogBase;
#endif
#endif


extern BOOL Sys_OpenAllLibs(void);
extern void Sys_CloseAllLibs(void);
extern void *Sys_AllocMem(ULONG);
extern void Sys_FreeMem(void *);
extern UBYTE *Sys_AllocFile(STRPTR,LONG *);
extern void Sys_FreeFile(UBYTE *);
extern const char *Sys_GetString(LONG, const char *);

#if !defined(__amigaos4__) && !defined(__MORPHOS__)
extern LONG Sys_Printf(char *,...);
extern void Sys_SPrintf(char *, const char *,...);
#endif

extern void Sys_FlushOutput(void);
extern ULONG Sys_GetClock(void);
extern void Sys_MemCopy(void *, const void *, LONG);
extern void Sys_StrCopy(char *, const char *, LONG);
extern LONG Sys_StrLen(const char *);
extern LONG Sys_StrCmp(const char *, const char *);
extern LONG Sys_StrCmpNoCase(const char *, const char *);

extern BOOL Sys_OpenClipboard(HOOKFUNC, void *);
extern void Sys_CloseClipboard(void);
extern char *Sys_ReadClipboard(ULONG *);
extern void Sys_WriteClipboard(char *, ULONG);


#endif  /* SYSTEM_H */
