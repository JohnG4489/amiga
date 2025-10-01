#ifndef GLOBAL_H
#define GLOBAL_H

//#ifdef __SASC
//#define __CURRENTDATE__ __AMIGADATE__
//#else
#define __CURRENTDATE__ "06.12.22"
//#endif

#define __APPNAME__ "TwinVNC"
#define __CURRENTVERSION__ "0.91betatesters"
#define __COPYRIGHT__ "(c) 2004-2022"

#ifdef __SASC
#define __OSNAME__ "AmigaOS/060"
#define __DEFAULT_STACK_SIZE__ 8192
#endif
#ifdef __MORPHOS__
#define __OSNAME__ "MorphOS/PPC"
#define __DEFAULT_STACK_SIZE__ 16384
#endif
#ifdef __amigaos4__
#define __OSNAME__ "AmigaOS4/PPC"
#define __DEFAULT_STACK_SIZE__ 16384
#endif

#endif /* GLOBAL_H */
