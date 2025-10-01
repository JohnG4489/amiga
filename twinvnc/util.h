#ifndef UTIL_H
#define UTIL_H

#ifndef TWIN_H
#include "twin.h"
#endif


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL Util_PrepareDecodeBuffers(struct Twin *, ULONG, ULONG);
extern void Util_FlushDecodeBuffers(struct Twin *, BOOL);
extern int Util_ZLibUnCompress(z_stream *, void *, ULONG, void *, ULONG);
extern int Util_ZLibInit(z_stream *);
extern int Util_ZLibEnd(z_stream *);

extern LONG Util_ReadTwinColor(struct Twin *, LONG, BOOL, ULONG *);
extern LONG Util_WaitReadStringUTF8(struct Twin *, char *, LONG);
extern LONG Util_WaitWriteStringUTF8(struct Twin *, const char *);

extern ULONG Util_ReadBuffer8(UBYTE **, UBYTE *);
extern ULONG Util_ReadBuffer16(UBYTE **, UBYTE *);
extern ULONG Util_ReadBuffer32(UBYTE **, UBYTE *);
extern void Util_ReadBufferStringUTF8(UBYTE **, UBYTE *, char *, LONG);
extern void *Util_ReadBufferColor(LONG, BOOL, UBYTE *, ULONG *);
extern void *Util_ReadBufferColorBE(LONG, UBYTE *, ULONG *);
extern void *Util_ReadBufferData(UBYTE *, UBYTE *, LONG);
extern void Util_ConvertLittleEndianPixelToBigEndian(void *, ULONG, LONG);

extern void Util_ANSItoUTF8(const char *, char *, LONG);
extern void Util_UTF8toANSI(const char *, char *, LONG);

extern ULONG Util_Time64ToAmigaTime(const ULONG [2]);
extern void Util_AmigaTimeToTime64(ULONG, ULONG [2]);

extern void Util_ShiftL64(ULONG [2], BOOL *);
extern void Util_ShiftR64(ULONG [2], BOOL *);
extern void Util_DivU64(ULONG [2], const ULONG [2], ULONG [2]);
extern void Util_Add64(const ULONG [2], ULONG [2]);
extern void Util_MulU64(const ULONG [2], ULONG [2]);

#endif  /* UTIL_H */
