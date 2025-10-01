#ifndef INPUT_H
#define INPUT_H

#include <intuition/intuition.h>

struct InputMsg
{
    ULONG Type;
    UWORD MouseX;
    UWORD MouseY;
    ULONG MouseMask;
    ULONG Key;
    BOOL DownFlag;
};

struct Inputs
{
    struct InputMsg Msg[32];
    LONG MaxCountOfMsg;
    LONG Offset;
    LONG Count;
    ULONG PrevMouseMask;
    ULONG PrevQualifier;
    BOOL IsMiddleButtonEmulated;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern void Inp_Init(struct Inputs *);
extern BOOL Inp_ManageInputMessages(struct Twin *, struct IntuiMessage *);
extern LONG Inp_SendInputMessages(struct Twin *);


#endif  /* INPUT_H */
