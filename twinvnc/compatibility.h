#ifndef COMPATIBILITY_H
#define COMPATIBILITY_H


#define GETHEAD(l) ((l)->lh_Head!=NULL && (l)->lh_Head->ln_Succ!=NULL?(l)->lh_Head:NULL)
#define GETTAIL(l) ((l)->lh_TailPred!=NULL && (l)->lh_TailPred->ln_Pred!=NULL?(l)->lh_TailPred:NULL)
#define GETSUCC(ln) (((struct Node *)(ln))->ln_Succ!=NULL && ((struct Node *)(ln))->ln_Succ->ln_Succ!=NULL?((struct Node *)(ln))->ln_Succ:NULL)


/**********************************************/
/* Gestion de la molette souris avec AmigaOS4 */
/**********************************************/

#ifdef __SASC
#define IDCMP_EXTENDEDMOUSE   0x08000000

struct IntuiWheelData
{
    UWORD Version;  /* version of this structure (see below) */
    UWORD Reserved; /* always 0, reserved for future use     */
    WORD  WheelX;   /* horizontal wheel movement delta       */
    WORD  WheelY;   /* vertical wheel movement delta         */
};

#define INTUIWHEELDATA_VERSION 1 /* current version of the structure above */

enum
{
    IMSGCODE_INTUIWHEELDATA  = (1<<15),
    IMSGCODE_INTUIRAWKEYDATA = (1<<14)
};
#endif


/***********************************************************************/
/* Redefinition de quelques differences entre OS pour la compatibilite */
/***********************************************************************/

#if defined(__amigaos4__)
#define Flush FFlush
#endif

#if !defined(__amigaos4__) && !defined(__MORPHOS__)
#define SRCFMT_RGB16 SRCFMT_RGB16PC
#endif

#ifdef __MORPHOS__
#define UnLockVLayer UnlockVLayer
#endif

#if defined(__amigaos4__) || defined(__MORPHOS__)
#include <stdio.h>
#define Sys_Printf printf
#define Sys_SPrintf sprintf
#endif

#ifndef TCP_NODELAY
#define TCP_NODELAY 1
#endif

#ifdef __amigaos4__
#define TVNC_SetWindowTitles(win,windowtitle,screentitle) SetWindowTitles(win,(CONST_STRPTR)(windowtitle),(CONST_STRPTR)(screentitle))
#else
#define TVNC_SetWindowTitles(win,windowtitle,screentitle) SetWindowTitles(win,(UBYTE *)(windowtitle),(UBYTE *)(screentitle))
#endif

#ifdef __MORPHOS__
#define ICOSTR STRPTR
#else
#define ICOSTR char *
#endif


/***************************************************/
/* Gestion des Hooks pour MorphOS, OS4 et SASC/68k */
/***************************************************/

#ifdef __MORPHOS__
#define M_HOOK_PROTO(FuncName,x,y,z) \
    ULONG FuncName##_GATE(void); \
    ULONG FuncName##_GATE2(x,y,z); \
    struct EmulLibEntry FuncName={TRAP_LIB,0,(void (*)(void))FuncName##_GATE }; \
    ULONG FuncName##_GATE(void) {return (FuncName##_GATE2((void *)REG_A0,(void *)REG_A2,(void *)REG_A1));} \
    struct Hook FuncName##_hook={{0},0,(void *)&FuncName};
#define M_HOOK_FUNC(FuncName,x,y,z) ULONG FuncName##_GATE2(x,y,z)
#endif

#ifdef __amigaos4__
#define M_HOOK_PROTO(FuncName,x,y,z) ULONG ASM SAVEDS FuncName(REG(a0,x),REG(a2,y),REG(a1,z))
#define M_HOOK_FUNC(FuncName,x,y,z) ULONG ASM SAVEDS FuncName(REG(a0,x),REG(a2,y),REG(a1,z))
#endif

#if !defined(__amigaos4__) && !defined(__MORPHOS__)
#define M_HOOK_PROTO(FuncName,x,y,z) ULONG __saveds __asm FuncName(REGISTER __a0 x,REGISTER __a2 y,REGISTER __a1 z)
#define M_HOOK_FUNC(FuncName,x,y,z) ULONG __saveds __asm FuncName(REGISTER __a0 x,REGISTER __a2 y,REGISTER __a1 z)
#endif


#endif  /* COMPATIBILITY_H */
