#include "system.h"
#include "filescheduler.h"
#include "filemanager.h"
#include "client.h"


/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    11-07-2017 (Seg)    Cr�ation de l'automate de gestion du t�l�chargement de fichiers
*/


/***** Prototypes */
void Fsc_InitFileScheduler(struct FileList *);
void Fsc_FlushFileSchedulerResources(struct FileList *);

LONG Fsc_ScheduleRemoteStart(struct FileList *, const char *, const char *, const struct FileItemNode *, LONG);
LONG Fsc_ScheduleRemoteNext(struct FileList *, struct Twin *Twin);
BOOL Fsc_SetDestinationPath(struct FileList *, const char *, const char *);
void Fsc_FlushDestinationPath(struct FileList *);
LONG Fsc_SchedulerDownloadStart(struct FileList *);
LONG Fsc_SchedulerDownloadFile(struct FileList *, const UBYTE *, LONG);
LONG Fsc_SchedulerDownloadStop(struct FileList *);

struct FileListNode *Fsc_GetCurrentFileList(struct FileList *, BOOL);
struct FileListNode *Fsc_PushNewFileList(struct FileList *, const char *, const char *);

void Fsc_RemoveFileList(struct FileListNode *);


/*****
    Initialisation du scheduler
*****/

void Fsc_InitFileScheduler(struct FileList *fl)
{
    NewList(&fl->FileListBase);
    /*
    fl->FileListBase.lh_Head=(struct Node *)&fl->FileListBase.lh_Tail;
    fl->FileListBase.lh_Tail=NULL;
    fl->FileListBase.lh_TailPred=(struct Node *)&fl->FileListBase.lh_Head;
    fl->FileListBase.lh_Type=0;
    */
}


/*****
    Suppression des ressources allou�es pour le scheduler
*****/

void Fsc_FlushFileSchedulerResources(struct FileList *fl)
{
    struct Node *NodePtr;

    while((NodePtr=GETHEAD(&fl->FileListBase))!=NULL)
    {
        Fsc_RemoveFileList((struct FileListNode *)NodePtr);
    }

    Fsc_FlushDestinationPath(fl);
}


/*****
    D�marrage du traitement
*****/

LONG Fsc_ScheduleRemoteStart(struct FileList *fl, const char *LocalPathBase, const char *RemotePathBase, const struct FileItemNode *fi, LONG Action)
{
    LONG Result=RESULT_NOT_ENOUGH_MEMORY;
    struct FileListNode *fln;

    /* Nettoyage de la liste en cours */
    Fsc_FlushFileSchedulerResources(fl);

    /* Cr�ation d'une nouvelle liste */
    fln=Fsc_PushNewFileList(fl,RemotePathBase,NULL);
    if(fln!=NULL)
    {
        if(Fmg_AddItemToFileList(&fln->fm,fi->Node.ln_Name,fi->Size64,fi->Time,fi->Flags))
        {
            Result=RESULT_SUCCESS;
        }
    }

    return Result;
}


/*****
    Traitement de la prochaine op�ration de t�l�chargement
*****/

LONG Fsc_ScheduleRemoteNext(struct FileList *fl, struct Twin *Twin)
{
    LONG Result=RESULT_SUCCESS;
    BOOL IsExit=FALSE;
    struct FileListNode *CurFileList;

    while((CurFileList=Fsc_GetCurrentFileList(fl,FALSE))!=NULL && Result>=0 && !IsExit)
    {
        struct FileItemNode *CurFileItem=CurFileList->CurFileItem;

        /* On prend le d�but de la liste si c'est la premi�re fois qu'on la scrute */
        if(CurFileItem==NULL) CurFileItem=(struct FileItemNode *)GETHEAD(&CurFileList->fm.FileItemList);
        else
        {
            /* Ici, on traite l'action sur l'item pr�c�demment regard� */
            if(CurFileItem->Flags!=FMNG_FLAG_DIR && fl->Action==FSC_ACTION_DELETE)
            {
                /* Suppression du r�pertoire si action=delete */
            }

            /* Maintenant, on passe � l'item � traiter */
            CurFileItem=(struct FileItemNode *)GETSUCC(CurFileItem);
        }

        if(CurFileItem!=NULL)
        {
            /* On est sur la liste courante */
            if(CurFileItem->Flags==FMNG_FLAG_FILE)
            {
                /* Fichier */
                if(fl->Action==FSC_ACTION_DOWNLOAD)
                {
                    /*
                    Result=Clt_SetFileDownloadRequest(Twin,
                        const char *LocalCurrentPath,
                        const char *RemoteCurrentPath,
                        const char *FileName,
                        const ULONG Size64[2],
                        ULONG Time);
                    */
                }

                //Sys_Printf("File: Name='%s'\n",CurFileItem->Node.ln_Name);

                IsExit=FALSE;
            }
            else
            {
                /* R�pertoire */
                struct FileListNode *fln=Fsc_PushNewFileList(fl,CurFileList->fm.CurrentPath,CurFileItem->Node.ln_Name);

                if(fln!=NULL)
                {
                    //Sys_Printf("Folder: '%s'\n",fln->fm.CurrentPath);
                    Result=Clt_SetFileListRequest(Twin,fln->fm.CurrentPath,1);
                } else Result=RESULT_NOT_ENOUGH_MEMORY;

                IsExit=TRUE;
            }

            CurFileList->CurFileItem=CurFileItem;
        }
        else
        {
            /* On supprime la liste courante puisqu'il n'y a plus de fichier � scruter */
            Fsc_RemoveFileList(CurFileList);
        }
    }

    return Result;
}


/*****
    Change le chemin du r�pertoire destination courant et cr�e le r�pertoire s'il n'existe pas
    - fl: pointe la liste cha�n�e de r�pertoires
    - PathBase: chemin de base
    - FolderName: nom de r�pertoire � ajouter au chemin de base (peut �tre null)
    Retourne TRUE si �a a march�. FALSE si le r�pertoire n'a pas �t� cr�� ou si d�faut de m�moire.
*****/

BOOL Fsc_SetDestinationPath(struct FileList *fl, const char *PathBase, const char *FolderName)
{
    BOOL IsSuccess=FALSE;
    LONG Size=Sys_StrLen(PathBase)+sizeof(char);
    char *Path;

    if(FolderName!=NULL) Size+=Sys_StrLen(FolderName);

    if((Path=(char *)Sys_AllocMem(Size))!=NULL)
    {
        BPTR l;

        Sys_StrCopy(Path,PathBase,Size);
        if(FolderName!=NULL) AddPart((STRPTR)Path,(STRPTR)FolderName,Size);

        /* Teste si le r�pertoire existe */
        l=Lock((STRPTR)Path,ACCESS_WRITE);
        if(l==NULL)
        {
            if((l=CreateDir(Path))!=NULL)
            {
                IsSuccess=TRUE;
            }
        } else IsSuccess=TRUE;

        if(l!=NULL) UnLock(l);

        Fsc_FlushDestinationPath(fl);
        fl->CurrentDstPath=Path;
    }

    return IsSuccess;
}


/*****
    Permet de nettoyer le chemin allou� dans la structure FileList
*****/

void Fsc_FlushDestinationPath(struct FileList *fl)
{
    if(fl->CurrentDstPath!=NULL)
    {
        Sys_FreeMem((void *)fl->CurrentDstPath);
        fl->CurrentDstPath=NULL;
    }
}


/*****
*****/

LONG Fsc_SchedulerDownloadStart(struct FileList *fl)
{
    return 0;
}


/*****
*****/

LONG Fsc_SchedulerDownloadFile(struct FileList *fl, const UBYTE *Data, LONG Len)
{
    return 0;
}


/*****
*****/

LONG Fsc_SchedulerDownloadStop(struct FileList *fl)
{
    return 0;
}


/*****
    Retourne un pointeur sur la liste courante de fichiers
    fl: pointeur sur une filelist
    IsFlushList: TRUE pour vider la liste avant de retourner le pointeur sur le noeud
*****/

struct FileListNode *Fsc_GetCurrentFileList(struct FileList *fl, BOOL IsFlushList)
{
    struct FileListNode *fln=(struct FileListNode *)GETTAIL(&fl->FileListBase);

    if(fln!=NULL && IsFlushList)
    {
        Fmg_FlushFileManagerResources(&fln->fm,FALSE);
    }

    return fln;
}


/*****
    Ajoute une nouvelle liste de fichier en queue
*****/

struct FileListNode *Fsc_PushNewFileList(struct FileList *fl, const char *PathBase, const char *FolderName)
{
    struct FileListNode *fln=(struct FileListNode *)Sys_AllocMem(sizeof(struct FileListNode));

    if(fln!=NULL)
    {
        BOOL IsSuccess=TRUE;

        /* On attache le nouveau noeud en fin de liste */
        AddTail(&fl->FileListBase,(struct Node *)fln);

        /* Initialisation du noeud */
        Fmg_InitFileManager(&fln->fm);

        /* On g�n�re le chemin du filemanager */
        if(!Fmg_AppendToCurrentPath(&fln->fm,PathBase,FALSE)) IsSuccess=FALSE;
        if(IsSuccess && FolderName!=NULL) IsSuccess=Fmg_AppendToCurrentPath(&fln->fm,FolderName,TRUE);

        /* On restitue la m�moire en cas d'�chec */
        if(!IsSuccess)
        {
            Fsc_RemoveFileList(fln);
            fln=NULL;
        }
    }

    return fln;
}


/*****
    Supprime une liste de fichiers
*****/

void Fsc_RemoveFileList(struct FileListNode *fln)
{
    if(fln!=NULL)
    {
        Remove((struct Node *)fln);

        /* On nettoie les ressources allou�es dans le noeud */
        Fmg_FlushFileManagerResources(&fln->fm,TRUE);

        /* On supprime l'espace allou� pour le noeud */
        Sys_FreeMem((void *)fln);
    }
}
