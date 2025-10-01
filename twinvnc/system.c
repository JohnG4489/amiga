#include <stdarg.h>
#include <time.h>
#include "system.h"

/*
    26-06-2017 (Seg)    Ajout de Sys_StrCmpNoCase()
    16-01-2016 (Seg)    Modification suite à la refonte de display.c
    03-09-2000 (Seg)    Utilisation des fonctions systeme du 3dengine
    19-05-2004 (Seg)    Ajout des fonctions Sys_MemCopy() et Sys_StrCopy()
*/


#if !defined(__amigaos4__) && !defined(__MORPHOS__)
ULONG OperatingSystemID=OSID_AMIGAOS3;
#endif

#ifdef __amigaos4__
struct Library *CyberGfxBase=NULL;
struct Library *GadToolsBase=NULL;
struct Library *KeymapBase=NULL;
struct Library *CGXVideoBase=NULL;
struct Catalog *CatalogBase=NULL;

struct CyberGfxIFace *ICyberGfx=NULL;
struct CGXVideoIFace *ICGXVideo=NULL;
struct GadToolsIFace *IGadTools=NULL;
struct KeymapIFace *IKeymap=NULL;
struct DiskfontIFace *IDiskfont=NULL;
#endif

#ifndef __amigaos4__
struct GfxBase *GfxBase=NULL;
struct Library *UtilityBase=NULL;
struct Library *AslBase=NULL;
struct Library *IconBase=NULL;
struct Library *WorkbenchBase=NULL;
struct Library *LocaleBase=NULL;
struct Library *CyberGfxBase=NULL;
struct Library *GadToolsBase=NULL;
struct Library *KeymapBase=NULL;
struct Library *DiskfontBase=NULL;
struct Library *CGXVideoBase=NULL;
struct Library *CodesetsBase=NULL;
struct Catalog *CatalogBase=NULL;
#endif

struct IOClipReq *IOClipReqPtr=NULL;
struct Hook IOClipHook;


/***** Prototypes */
BOOL Sys_OpenAllLibs(void);
void Sys_CloseAllLibs(void);
void *Sys_AllocMem(ULONG);
void Sys_FreeMem(void *);
UBYTE *Sys_AllocFile(STRPTR,LONG *);
void Sys_FreeFile(UBYTE *);
const char *Sys_GetString(LONG, const char *);

#if !defined(__amigaos4__) && !defined(__MORPHOS__)
LONG Sys_Printf(char *,...);
void Sys_SPrintf(char *, const char *,...);
#endif

void Sys_FlushOutput(void);
ULONG Sys_GetClock(void);
void Sys_MemCopy(void *, const void *, LONG);
void Sys_StrCopy(char *, const char *, LONG);
LONG Sys_StrLen(const char *);
LONG Sys_StrCmp(const char *, const char *);
LONG Sys_StrCmpNoCase(const char *, const char *);

BOOL Sys_OpenClipboard(HOOKFUNC, void *);
void Sys_CloseClipboard(void);
char *Sys_ReadClipboard(ULONG *);
void Sys_WriteClipboard(char *, ULONG);

BOOL CreateClipboardHook(HOOKFUNC, void *);
void RemoveClipboardHook(void);


/*****
    Ouverture des librairies system.
*****/


BOOL Sys_OpenAllLibs(void)
{
#ifndef __amigaos4__
    /* Libs below are auto opened by os4 libauto.a */
    GfxBase=(struct GfxBase *) OpenLibrary("graphics.library",40L);
    UtilityBase=OpenLibrary("utility.library",39L);
    AslBase=OpenLibrary("asl.library",39L);
    IconBase=OpenLibrary("icon.library",39L);
    WorkbenchBase=OpenLibrary("workbench.library",39L);
    LocaleBase=OpenLibrary("locale.library",38L);
#endif

    CyberGfxBase=OpenLibrary(CYBERGFXNAME,CYBERGFX_INCLUDE_VERSION);
    CGXVideoBase=OpenLibrary("cgxvideo.library",0L);
    GadToolsBase=OpenLibrary("gadtools.library",39L);
    KeymapBase=OpenLibrary("keymap.library",39L);
    DiskfontBase=OpenLibrary("diskfont.library",39L);
    CodesetsBase=OpenLibrary("codesets.library",6L);

#ifdef __amigaos4__
    /* Interfaces which we have to open for corresponding libs, because they are not opened by libauto.a */
    ICyberGfx=(struct CyberGfxIFace *)GetInterface(CyberGfxBase,"main",1L,NULL);
    ICGXVideo=(struct CGXVideoIFace *)GetInterface(CGXVideoBase,"main",1L,NULL);
    IGadTools=(struct GadToolsIFace *)GetInterface(GadToolsBase,"main",1L,NULL);
    IKeymap=(struct KeymapIFace *)GetInterface(KeymapBase,"main",1L,NULL);
    IDiskfont=(struct DiskfontIFace *)GetInterface(DiskfontBase,"main",1L,NULL);
    ICodesets=(struct CodesetsIFace *)GetInterface(CodesetsBase,"main",1L,NULL);
#endif

    /* Rq: les librairies Cybergraphics et cgxvideo sont facultatives */
    if(GfxBase && GadToolsBase && KeymapBase && UtilityBase &&
       AslBase && IconBase && WorkbenchBase && LocaleBase)
    {
        CatalogBase=OpenCatalog(NULL,"twinvnc.catalog",TAG_DONE);

#if !defined(__amigaos4__) && !defined(__MORPHOS__)
        if(FindResident("MorphOS")!=NULL)
        {
            //Sys_Printf("System: MorphOS\n");
            OperatingSystemID=OSID_MORPHOS;
        }
        else if(SysBase->LibNode.lib_Version>=50)
        {
            //Sys_Printf("System: AmigaOS4\n");
            OperatingSystemID=OSID_AMIGAOS4;
        }
        else
        {
            //Sys_Printf("System: AmigaOS3.x\n");
            OperatingSystemID=OSID_AMIGAOS3;
        }
#endif

        return TRUE; /* Succes */
    }

    Sys_CloseAllLibs();
    return FALSE;
}


/*****
    Fermeture des librairies system.
*****/

void Sys_CloseAllLibs(void)
{
    if(LocaleBase) CloseCatalog(CatalogBase);

#ifdef __amigaos4__
    /* Interfaces which we have to open for corresponding libs, because they are not opened by libauto.a */
    DropInterface((struct Interface *)ICyberGfx);
    DropInterface((struct Interface *)ICGXVideo);
    DropInterface((struct Interface *)IGadTools);
    DropInterface((struct Interface *)IKeymap);
    DropInterface((struct Interface *)IDiskfont);
    DropInterface((struct Interface *)ICodesets);
#endif

    CloseLibrary(CyberGfxBase);
    CloseLibrary(CGXVideoBase);
    CloseLibrary(GadToolsBase);
    CloseLibrary(KeymapBase);
    CloseLibrary(DiskfontBase);
    CloseLibrary(CodesetsBase);

#ifndef __amigaos4__
    /* Libs below are auto closed by os4 libauto.a */
    CloseLibrary((struct Library *)GfxBase); /* NULL Autorisé (OS V36+) */
    CloseLibrary(UtilityBase);
    CloseLibrary(AslBase);
    CloseLibrary(IconBase);
    CloseLibrary(WorkbenchBase);
    CloseLibrary(LocaleBase);
#endif
}


/*****
    Allocation mémoire
    - la mémoire allouée est initialisée à zéro.
    Retourne NULL en cas d'echec
*****/

void *Sys_AllocMem(ULONG Size)
{
    return AllocVec(Size,MEMF_ANY|MEMF_CLEAR);
}


/*****
    Désallocation de la mémoire allouée par Sys_AllocMem()
    Précondition: Ptr peut être NULL
*****/

void Sys_FreeMem(void *Ptr)
{
    FreeVec(Ptr);
}


/*****
    Chargement d'un fichier dans un buffer.
    - Name: nom du fichier
    - FileSize: adresse d'un LONG ou stocker la taille du fichier (peut être NULL)
      Si FileSize est NULL ou si le LONG situé à cette adresse est égal à 0, alors
      le fichier sera chargé tout entier et FileSize sera initialisé à la taille
      du fichier si FileSize est non NULL.
      Si à l'initialisation, la taille de FileSize est égale à n (n>0), alors
      AllocFile() tentera de charger les n premiers octets du fichier.
*****/

UBYTE *Sys_AllocFile(STRPTR Name,LONG *FileSize)
{
    BPTR h;
    UBYTE *Ptr=NULL;

    if((h=Open(Name,MODE_OLDFILE))!=0)
    {
        LONG Size;
        Seek(h,0,OFFSET_END);
        Size=Seek(h,0,OFFSET_BEGINNING);
        if(FileSize!=NULL)
        {
            if(*FileSize==0L) *FileSize=Size; else Size=*FileSize;
        }
        if((Ptr=(UBYTE *)Sys_AllocMem((ULONG)Size))!=NULL) Read(h,Ptr,Size);
        Close(h);
    }
    return Ptr;
}


/*****
    Libération du buffer alloué par AllocFile().
*****/

void Sys_FreeFile(UBYTE *Ptr)
{
    Sys_FreeMem((void *)Ptr); /* NULL Autorisé */
}


/*****
    Gestion des ressources
*****/

const char *Sys_GetString(LONG StringID, const char *DefaultString)
{
    if(DefaultString!=NULL && StringID>=0) return (const char *)GetCatalogStr(CatalogBase,StringID,(STRPTR)DefaultString);

    return DefaultString;
}


/*****
    Ecriture d'une chaîne de caractères dans la sortie standard.
*****/
#if !defined(__amigaos4__) && !defined(__MORPHOS__)
LONG Sys_Printf(char *String,...)
{
    LONG Error;
    va_list argptr;

    va_start(argptr,String);
    Error=VPrintf(String,argptr);
    va_end(argptr);

    return Error;      /*  count/error */
}
#endif


/*****
    Formatage d'une chaîne de caractères.
*****/
#if !defined(__amigaos4__) && !defined(__MORPHOS__)
void Sys_SPrintf(char *Buffer, const char *String,...)
{
    va_list argptr;

    va_start(argptr,String);
    RawDoFmt((STRPTR)String,argptr,(void (*))"\x16\xc0\x4e\x75",Buffer);
    /*sprintf(Buffer,String,argptr);*/
    va_end(argptr);
}
#endif


/*****
    Flush de la sortie courante
*****/

void Sys_FlushOutput(void)
{
    Flush(Output());
}


/*****
    Pour obtenir le nombre de secondes courant
*****/

ULONG Sys_GetClock(void)
{
    return (ULONG)clock();
}


/*****
    Equivalent de memcpy()
*****/

void Sys_MemCopy(void *Dst, const void *Src, LONG Size)
{
    UBYTE *pdst=(UBYTE *)Dst, *psrc=(UBYTE *)Src;
    while(--Size>=0) *(pdst++)=*(psrc++);
}


/*****
    Equivalent de strcpy()
*****/

void Sys_StrCopy(char *Dst, const char *Src, LONG SizeOfDst)
{
    while(--SizeOfDst>0 && *Src!=0) *(Dst++)=*(Src++);
    *Dst=0;
}


/*****
    Permet d'obtenir le nombre de caractères du chaîne.
*****/

LONG Sys_StrLen(const char *String)
{
    LONG Count=0;

    while(String[Count]!=0) Count++;

    return Count;
}


/*****
    Comparaison de deux chaînes
*****/

LONG Sys_StrCmp(const char *Str1, const char *Str2)
{
    while(*Str1!=0 || *Str2!=0)
    {
        if(*Str1<*Str2) return -1;
        else if(*Str1>*Str2) return 1;
        Str1++;
        Str2++;
    }

    return 0;
}


/*****
    Comparaison de deux chaînes insensitivement
*****/

LONG Sys_StrCmpNoCase(const char *Str1, const char *Str2)
{
    while(*Str1!=0 || *Str2!=0)
    {
        char Char1=*Str1>='A' && *Str1<='Z'?*Str1|32:*Str1;
        char Char2=*Str2>='A' && *Str2<='Z'?*Str2|32:*Str2;
        if(Char1<Char2) return -1;
        else if(Char1>Char2) return 1;
        Str1++;
        Str2++;
    }

    return 0;
}

/*****
    Ouverture du clipboard.device pour la gestion des copier/coller
*****/

BOOL Sys_OpenClipboard(HOOKFUNC FuncPtr, void *Data)
{
    struct MsgPort *mp=CreateMsgPort();

    if(mp!=NULL)
    {
        IOClipReqPtr=(struct IOClipReq *)CreateIORequest(mp,sizeof(struct IOClipReq));

        if(IOClipReqPtr!=NULL)
        {
            if(!(OpenDevice("clipboard.device",0L,(struct IORequest *)IOClipReqPtr,0L)))
            {
                if(CreateClipboardHook(FuncPtr,Data)) return TRUE;
                else
                {
                    Sys_CloseClipboard();
                    return FALSE;
                }
            }

            DeleteIORequest((struct IORequest *)IOClipReqPtr);
        }

        DeleteMsgPort(mp);
    }

    return FALSE;
}


/*****
    Fermeture du clipboard.device ouvert par OpenClipboard()
*****/

void Sys_CloseClipboard(void)
{
    if(IOClipReqPtr!=NULL)
    {
        struct MsgPort *mp=IOClipReqPtr->io_Message.mn_ReplyPort;

        RemoveClipboardHook();
        CloseDevice((struct IORequest *)IOClipReqPtr);
        DeleteIORequest((struct IORequest *)IOClipReqPtr);
        DeleteMsgPort(mp);
    }
}


/*****
    Lecture d'une chaîne dans le Clipboard
    Note: Il faut désallouer la chaîne avec Sys_FreeMem()
*****/

char *Sys_ReadClipboard(ULONG *TotalLen)
{
    char *String=NULL;

    *TotalLen=0;
    if(IOClipReqPtr!=NULL)
    {
        ULONG Data[3];

        /* initial set-up for Offset, Error, and ClipID */
        IOClipReqPtr->io_Offset=0;
        IOClipReqPtr->io_Error=0;
        IOClipReqPtr->io_ClipID=0;

        /* Look for "FORM[size]FTXT" */
        IOClipReqPtr->io_Command=CMD_READ;
        IOClipReqPtr->io_Data=(STRPTR)Data;
        IOClipReqPtr->io_Length=3*sizeof(ULONG);
        DoIO((struct IORequest *)IOClipReqPtr);

        /* Check to see if we have at least 12 bytes */
        if(IOClipReqPtr->io_Actual==3*sizeof(ULONG) && !IOClipReqPtr->io_Error)
        {
            /* Check to see if it starts with "FORM" and "FTXT" */
            if(Data[0]==*((ULONG *)"FORM") && Data[2]==*((ULONG *)"FTXT"))
            {
                while(!IOClipReqPtr->io_Error && IOClipReqPtr->io_Actual>=IOClipReqPtr->io_Length)
                {
                    IOClipReqPtr->io_Command=CMD_READ;
                    IOClipReqPtr->io_Data=(STRPTR)Data;
                    IOClipReqPtr->io_Length=2*sizeof(ULONG);
                    DoIO((struct IORequest *)IOClipReqPtr);

                    if(IOClipReqPtr->io_Actual==2*sizeof(ULONG) && !IOClipReqPtr->io_Error)
                    {
                        ULONG Offset=IOClipReqPtr->io_Offset;
                        ULONG Len=Data[1];

                        if(Data[0]==*((ULONG *)"CHRS"))
                        {
                            char *NewString=Sys_AllocMem(*TotalLen+Len);

                            if(NewString!=NULL)
                            {
                                if(String!=NULL)
                                {
                                    LONG i;
                                    for(i=0;i<*TotalLen;i++) NewString[i]=String[i];
                                    Sys_FreeMem((void *)String);
                                }

                                IOClipReqPtr->io_Command=CMD_READ;
                                IOClipReqPtr->io_Data=(STRPTR)&NewString[*TotalLen];
                                IOClipReqPtr->io_Length=Len;
                                DoIO((struct IORequest *)IOClipReqPtr);

                                String=NewString;
                                *TotalLen+=Len;
                            }
                        }

                        IOClipReqPtr->io_Offset=Offset+Len+(Len&1);
                    }
                }
            }
        }

        /* falls through immediately if io_Actual==0 */
        IOClipReqPtr->io_Command=CMD_READ;
        IOClipReqPtr->io_Data=(STRPTR)Data;
        IOClipReqPtr->io_Length=sizeof(Data);
        while(IOClipReqPtr->io_Actual>0 && !IOClipReqPtr->io_Error)
        {
            DoIO((struct IORequest *)IOClipReqPtr);
        }
    }

    return String;
}


/*****
    Ecriture d'une chaîne dans le clipboard
*****/

void Sys_WriteClipboard(char *String, ULONG Len)
{
    if(IOClipReqPtr!=NULL)
    {
        ULONG pad=Len&1;
        ULONG Data[5];

        RemoveClipboardHook();

        /* initial set-up for Offset, Error, and ClipID */
        IOClipReqPtr->io_Offset=0;
        IOClipReqPtr->io_Error=0;
        IOClipReqPtr->io_ClipID=0;

        /* Create the IFF header information */
        Data[0]=*((ULONG *)"FORM");
        Data[1]=12+Len+pad;
        Data[2]=*((ULONG *)"FTXT");
        Data[3]=*((ULONG *)"CHRS");
        Data[4]=Len;

        /* Write Header */
        IOClipReqPtr->io_Data=(STRPTR)Data;
        IOClipReqPtr->io_Length=sizeof(Data);
        IOClipReqPtr->io_Command=CMD_WRITE;
        DoIO((struct IORequest *)IOClipReqPtr);

        /* Write string */
        IOClipReqPtr->io_Data=(STRPTR)String;
        IOClipReqPtr->io_Length=Len;
        IOClipReqPtr->io_Command=CMD_WRITE;
        DoIO((struct IORequest *)IOClipReqPtr);

        /* Write Pad if needed */
        if(pad>0)
        {
            IOClipReqPtr->io_Data=(STRPTR)"\0";
            IOClipReqPtr->io_Length=1L;
            IOClipReqPtr->io_Command=CMD_WRITE;
            DoIO((struct IORequest *)IOClipReqPtr);
        }

        /* Tell the clipboard we are done writing */
        IOClipReqPtr->io_Command=CMD_UPDATE;
        DoIO((struct IORequest *)IOClipReqPtr);

        CreateClipboardHook(IOClipHook.h_Entry,IOClipHook.h_Data);
    }
}


/*****
    Permet de définir un hook pour les arrivées de données dans le clipboard
*****/

BOOL CreateClipboardHook(HOOKFUNC FuncPtr, void *Data)
{
    if(FuncPtr!=NULL)
    {
        IOClipHook.h_Entry=FuncPtr;
        IOClipHook.h_SubEntry=NULL;
        IOClipHook.h_Data=Data;

        IOClipReqPtr->io_Command=CBD_CHANGEHOOK;
        IOClipReqPtr->io_Data=(STRPTR)&IOClipHook;
        IOClipReqPtr->io_Length=1;
        IOClipReqPtr->io_Offset=0;
        IOClipReqPtr->io_Error=0;
        IOClipReqPtr->io_ClipID=0;
        if(DoIO((struct IORequest *)IOClipReqPtr)!=0) return FALSE;
    }

    return TRUE;
}


/*****
    Permet de retirer le Hook definis par CreateClipboardHook()
*****/

void RemoveClipboardHook(void)
{
    IOClipReqPtr->io_Command=CBD_CHANGEHOOK;
    IOClipReqPtr->io_Data=(STRPTR)&IOClipHook;
    IOClipReqPtr->io_Length=0;
    DoIO((struct IORequest *)IOClipReqPtr);
}
