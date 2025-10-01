#ifndef FILEMANAGER_H
#define FILEMANAGER_H


#define FMNG_FLAG_FILE  0
#define FMNG_FLAG_DIR   1

struct FileItemNode
{
    struct Node Node;
    ULONG Flags;
    ULONG Size64[2];
    ULONG Time;
    BOOL IsSelected;
};

struct FileManager
{
    char *CurrentPath;
    struct List FileItemList;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern void Fmg_InitFileManager(struct FileManager *);
extern void Fmg_FlushFileManagerResources(struct FileManager *, BOOL);
extern BOOL Fmg_AppendToCurrentPath(struct FileManager *, const char *, BOOL);
extern BOOL Fmg_RootCurrentPath(struct FileManager *, BOOL);
extern void Fmg_ParentCurrentPath(struct FileManager *, BOOL);
extern struct FileItemNode *Fmg_GetFileItemNodeFromIndex(struct FileManager *, ULONG);
extern BOOL Fmg_AddItemToFileList(struct FileManager *, const char *, const ULONG [2], ULONG, ULONG);
extern void Fmg_SortFileItemList(struct FileManager *);


#endif  /* FILEMANAGER_H */

