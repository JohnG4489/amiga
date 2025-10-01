#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H


struct HistoryItemNode
{
    struct Node Node;
};

struct HistoryManager
{
    struct List HistoryItemList;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern void Hmg_InitHistoryManager(struct HistoryManager *);
extern void Hmg_FlushHistoryManagerResources(struct HistoryManager *);
extern BOOL Hmg_AddItemToHistoryList(struct HistoryManager *, const char *);


#endif  /* HISTORYMANAGER_H */

