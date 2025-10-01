#ifndef DECODERCURSOR_H
#define DECODERCURSOR_H


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL OpenDecoderCursor(struct Twin *);
extern void CloseDecoderCursor(struct Twin *);
extern LONG DecoderRichCursor(struct Twin *, LONG, LONG, LONG, LONG);
extern LONG DecoderXCursor(struct Twin *, LONG, LONG, LONG, LONG);

#endif  /* DECODERCURSOR_H */
