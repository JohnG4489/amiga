#include "system.h"
#include "historymanager.h"


/*
    03-03-2016 (Seg)    Gestionnaire de l'historique
*/


/***** Prototypes */
void Hmg_InitHistoryManager(struct HistoryManager *);
void Hmg_FlushHistoryManagerResources(struct HistoryManager *);

BOOL Hmg_AddItemToHistoryList(struct HistoryManager *, const char *);

void Hmg_RemoveItemFromHistoryList(struct HistoryItemNode *);
void Hmg_FreeHistoryItem(struct HistoryItemNode *);


/*****
    Initialisation du manager
*****/

void Hmg_InitHistoryManager(struct HistoryManager *hm)
{
    NewList(&hm->HistoryItemList);
    /*
    hm->HistoryItemList.lh_Head=(struct Node *)&hm->HistoryItemList.lh_Tail;
    hm->HistoryItemList.lh_Tail=NULL;
    hm->HistoryItemList.lh_TailPred=(struct Node *)&hm->HistoryItemList.lh_Head;
    hm->HistoryItemList.lh_Type=0;
    */
}


/*****
    Suppression des ressources allouées pour le manager
*****/

void Hmg_FlushHistoryManagerResources(struct HistoryManager *hm)
{
    struct Node *NodePtr;

    while((NodePtr=RemHead(&hm->HistoryItemList))!=NULL)
    {
        Hmg_FreeHistoryItem((struct HistoryItemNode *)NodePtr);
    }

    Hmg_InitHistoryManager(hm);
}


/*****
    Ajout d'un noeud sur la liste chaînée
*****/

BOOL Hmg_AddItemToHistoryList(struct HistoryManager *hm, const char *ItemName)
{
    BOOL IsSuccess=FALSE;
    struct HistoryItemNode *hi=(struct HistoryItemNode *)Sys_AllocMem(sizeof(struct HistoryItemNode));

    if(hi!=NULL)
    {
        LONG ItemSize=Sys_StrLen(ItemName)+sizeof(char);

        AddTail(&hm->HistoryItemList,(struct Node *)hi);
        if((hi->Node.ln_Name=(char *)Sys_AllocMem(ItemSize))!=NULL)
        {
            Sys_StrCopy(hi->Node.ln_Name,ItemName,ItemSize);
            IsSuccess=TRUE;
        }

        if(!IsSuccess) Hmg_RemoveItemFromHistoryList(hi);
    }

    return IsSuccess;
}


/*****
    Suppression d'un noeud de la liste et libération des ressources allouées
    par ce noeud.
*****/

void Hmg_RemoveItemFromHistoryList(struct HistoryItemNode *hi)
{
    if(hi!=NULL)
    {
        Remove((struct Node *)hi);
        Hmg_FreeHistoryItem(hi);
    }
}


/*****
    Libération des ressources d'un noeud
*****/

void Hmg_FreeHistoryItem(struct HistoryItemNode *hi)
{
    if(hi!=NULL)
    {
        if(hi->Node.ln_Name!=NULL) Sys_FreeMem((void *)hi->Node.ln_Name);
        Sys_FreeMem((void *)hi);
    }
}
