#include "system.h"
#include "filemanager.h"


/*
    02-07-2017 (Seg)    Modif suite refonte artimétique 64 bits
    02-03-2016 (Seg)    Quelques améliorations
    09-05-2006 (Seg)    Gestion des fichiers
*/


/***** Prototypes */
void Fmg_InitFileManager(struct FileManager *);
void Fmg_FlushFileManagerResources(struct FileManager *, BOOL);

BOOL Fmg_AppendToCurrentPath(struct FileManager *, const char *, BOOL);
BOOL Fmg_RootCurrentPath(struct FileManager *, BOOL);
void Fmg_ParentCurrentPath(struct FileManager *, BOOL);

struct FileItemNode *Fmg_GetFileItemNodeFromIndex(struct FileManager *, ULONG);
BOOL Fmg_AddItemToFileList(struct FileManager *, const char *, const ULONG [2], ULONG, ULONG);
void Fmg_SortFileItemList(struct FileManager *);

BOOL Fmg_CompareFileItemNode(struct FileItemNode *, struct FileItemNode *);
void Fmg_RemoveItemFromFileList(struct FileManager *, struct FileItemNode *);
void Fmg_FreeFileItem(struct FileItemNode *);


/*****
    Initialisation du manager
*****/

void Fmg_InitFileManager(struct FileManager *fm)
{
    NewList(&fm->FileItemList);
    /*
    fm->FileItemList.lh_Head=(struct Node *)&fm->FileItemList.lh_Tail;
    fm->FileItemList.lh_Tail=NULL;
    fm->FileItemList.lh_TailPred=(struct Node *)&fm->FileItemList.lh_Head;
    fm->FileItemList.lh_Type=0;
    */
    fm->CurrentPath=NULL;
}


/*****
    Suppression des ressources allouées pour le manager
    IsFlushCurrentPath: si TRUE, supprime également la chaîne allouée pour le CurrentPath
*****/

void Fmg_FlushFileManagerResources(struct FileManager *fm, BOOL IsFlushCurrentPath)
{
    struct Node *NodePtr;

    while((NodePtr=RemHead(&fm->FileItemList))!=NULL)
    {
        Fmg_FreeFileItem((struct FileItemNode *)NodePtr);
    }

    if(IsFlushCurrentPath && fm->CurrentPath!=NULL)
    {
        Sys_FreeMem((void *)fm->CurrentPath);
        fm->CurrentPath=NULL;
    }
}


/*****
    Ajoute un répertoire au chemin courant
*****/

BOOL Fmg_AppendToCurrentPath(struct FileManager *fm, const char *DirName, BOOL IsRemotePath)
{
    BOOL IsSuccess=FALSE;
    LONG Len=Sys_StrLen(DirName)+2*sizeof(char);
    char *Ptr;

    if(fm->CurrentPath!=NULL) Len+=Sys_StrLen(fm->CurrentPath);

    if((Ptr=(char *)Sys_AllocMem(Len))!=NULL)
    {
        if(fm->CurrentPath!=NULL)
        {
            if(IsRemotePath || Sys_StrCmp(fm->CurrentPath,"")!=0) Sys_SPrintf(Ptr,"%s%s/",fm->CurrentPath,DirName);
            else Sys_SPrintf(Ptr,"%s",DirName);

            Sys_FreeMem((void *)fm->CurrentPath);
        }
        else
        {
            if(IsRemotePath) Sys_SPrintf(Ptr,"/%s",DirName);
            else Sys_SPrintf(Ptr,"%s",DirName);
        }

        //Sys_Printf("Dir:%s\n",Ptr);
        fm->CurrentPath=Ptr;

        IsSuccess=TRUE;
    }

    return IsSuccess;
}


/*****
    Revient à la racine
*****/

BOOL Fmg_RootCurrentPath(struct FileManager *fm, BOOL IsRemotePath)
{
    if(fm->CurrentPath!=NULL) fm->CurrentPath[0]=0;

    return Fmg_AppendToCurrentPath(fm,"",IsRemotePath);
}


/*****
    Revient au répertoire précédent
*****/

void Fmg_ParentCurrentPath(struct FileManager *fm, BOOL IsRemotePath)
{
    if(fm->CurrentPath!=NULL)
    {
        LONG Pos=Sys_StrLen(fm->CurrentPath)-2;

        if(IsRemotePath) while(Pos>0 && fm->CurrentPath[Pos]!='/') fm->CurrentPath[Pos--]=0;
        else while(Pos>=0 && fm->CurrentPath[Pos]!='/' && fm->CurrentPath[Pos]!=':') fm->CurrentPath[Pos--]=0;
        //Sys_Printf("Parent Path:%s\n",fm->CurrentPath);
    }
}


/*****
    Récupération d'un noeud d'index "Index" sur la liste
*****/

struct FileItemNode *Fmg_GetFileItemNodeFromIndex(struct FileManager *fm, ULONG Index)
{
    struct Node *NodePtr=fm->FileItemList.lh_Head;
    ULONG i;

    for(i=0;NodePtr->ln_Succ!=NULL;i++)
    {
        if(i==Index) return (struct FileItemNode *)NodePtr;
        NodePtr=NodePtr->ln_Succ;
    }

    return NULL;
}


/*****
    Ajout d'un noeud sur la liste chaînée
*****/

BOOL Fmg_AddItemToFileList(struct FileManager *fm, const char *ItemName, const ULONG Size64[2], ULONG Time, ULONG Flags)
{
    BOOL IsSuccess=FALSE;
    struct FileItemNode *fi=(struct FileItemNode *)Sys_AllocMem(sizeof(struct FileItemNode));

    if(fi!=NULL)
    {
        LONG ItemSize=Sys_StrLen(ItemName)+sizeof(char);

        AddTail(&fm->FileItemList,(struct Node *)fi);
        if((fi->Node.ln_Name=(char *)Sys_AllocMem(ItemSize))!=NULL)
        {
            ULONG CurrentSecs,CurrentMicros;
            CurrentTime(&CurrentSecs,&CurrentMicros);
            Sys_StrCopy(fi->Node.ln_Name,ItemName,ItemSize);
            fi->Node.ln_Type=NT_USER;
            fi->Node.ln_Pri=0;
            fi->Size64[0]=Size64[0];
            fi->Size64[1]=Size64[1];
            fi->Time=Time;
            fi->Flags=Flags;
            fi->IsSelected=FALSE;

            /*{
                struct ClockData cd;
                char Tmp[100]="";
                Amiga2Date(Time,&cd);
                if((Flags&1)==0) Sys_SPrintf(Tmp," [%08lx: %02ld/%02ld/%04ld %02ld:%02ld:%02ld]",Time,cd.mday,cd.month,cd.year,cd.hour,cd.min,cd.sec);
                Sys_Printf("%s:%s %s\n",Flags?"DIR":"FILE",Tmp,ItemName);
            }*/

            IsSuccess=TRUE;
        }

        if(!IsSuccess)
        {
            Fmg_RemoveItemFromFileList(fm,fi);
        }
    }

    return IsSuccess;
}


/*****
    Routine de tri d'une liste chaînée en fonction des noms des items
*****/

void Fmg_SortFileItemList(struct FileManager *fm)
{
    struct Node *CursorNodePtr=fm->FileItemList.lh_Head;

    /* On regarde si on est sur le dernier noeud */
    while(CursorNodePtr->ln_Succ!=NULL)
    {
        struct Node *NodePtr=CursorNodePtr;

        /* On récup le noeud suivant pour la comparaison */
        while(NodePtr!=fm->FileItemList.lh_TailPred)
        {
            struct Node *NextNodePtr=NodePtr->ln_Succ;

            if(Fmg_CompareFileItemNode((struct FileItemNode *)CursorNodePtr,(struct FileItemNode *)NextNodePtr))
            {
                struct Node *Tmp=CursorNodePtr->ln_Pred;

                Remove(CursorNodePtr);
                Insert(&fm->FileItemList,CursorNodePtr,NextNodePtr);

                Remove(NextNodePtr);
                Insert(&fm->FileItemList,NextNodePtr,Tmp);

                Tmp=CursorNodePtr;
                CursorNodePtr=NextNodePtr;
                NextNodePtr=Tmp;
            }

            NodePtr=NextNodePtr;
        }

        CursorNodePtr=CursorNodePtr->ln_Succ;
    }
}


/*****
    Routine de comparaison de noeud pour classer les items de la liste
    dans l'ordre alphabétique en respectant la priorité des répertoire.
*****/

BOOL Fmg_CompareFileItemNode(struct FileItemNode *NodeAPtr, struct FileItemNode *NodeBPtr)
{
    if(NodeAPtr->Flags<NodeBPtr->Flags || (NodeAPtr->Flags==NodeBPtr->Flags &&
       Stricmp(((struct Node *)NodeAPtr)->ln_Name,((struct Node *)NodeBPtr)->ln_Name)>0))
    {
        return TRUE;
    }

    return FALSE;
}


/*****
    Suppression d'un noeud de la liste et libération des ressources allouées
    par ce noeud.
*****/

void Fmg_RemoveItemFromFileList(struct FileManager *fm, struct FileItemNode *fi)
{
    if(fi!=NULL)
    {
        Remove((struct Node *)fi);
        Fmg_FreeFileItem(fi);
    }
}


/*****
    Libération des ressources d'un noeud
*****/

void Fmg_FreeFileItem(struct FileItemNode *fi)
{
    if(fi!=NULL)
    {
        if(fi->Node.ln_Name!=NULL) Sys_FreeMem((void *)fi->Node.ln_Name);
        Sys_FreeMem((void *)fi);
    }
}
