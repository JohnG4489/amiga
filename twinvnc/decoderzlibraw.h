#ifndef DECODERZLIBRAW_H
#define DECODERZLIBRAW_H


struct ZLibRawData
{
    z_stream StreamZLibRaw;
    BOOL IsInitStreamZLibRawOk;
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern BOOL OpenDecoderZLibRaw(struct Twin *);
extern void CloseDecoderZLibRaw(struct Twin *);
extern LONG DecoderZLibRaw(struct Twin *, LONG, LONG, LONG, LONG);

#endif  /* DECODERZLIBRAW_H */
