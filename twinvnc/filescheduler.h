#ifndef FILESCHEDULER_H
#define FILESCHEDULER_H

#ifndef FILEMANAGER_H
#include "filemanager.h"
#endif

#define FSC_ACTION_DOWNLOAD     0
#define FSC_ACTION_DELETE       1


struct FileListNode
{
    struct Node Node;
    struct FileManager fm;
    struct FileItemNode *CurFileItem;
};

struct FileList
{
    LONG Action;
    struct List FileListBase;
    char *DstFileName;
    char *CurrentDstPath;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern void Fsc_InitFileScheduler(struct FileList *);
extern void Fsc_FlushFileSchedulerResources(struct FileList *);

extern LONG Fsc_ScheduleRemoteStart(struct FileList *, const char *, const char *, const struct FileItemNode *, LONG);
extern LONG Fsc_ScheduleRemoteNext(struct FileList *, struct Twin *Twin);
extern BOOL Fsc_SetDestinationPath(struct FileList *, const char *, const char *);
extern void Fsc_FlushDestinationPath(struct FileList *);
extern LONG Fsc_SchedulerDownloadStart(struct FileList *);
extern LONG Fsc_SchedulerDownloadFile(struct FileList *, const UBYTE *, LONG);
extern LONG Fsc_SchedulerDownloadStop(struct FileList *);

extern struct FileListNode *Fsc_GetCurrentFileList(struct FileList *, BOOL);
extern struct FileListNode *Fsc_PushNewFileList(struct FileList *, const char *, const char *);


#endif  /* FILESCHEDULER_H */

