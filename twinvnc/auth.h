#ifndef AUTH_H
#define AUTH_H


struct TightCapability
{
    LONG Code;
    char Vendor[5];
    char Name[9];
};

/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern LONG Auth_PerformAuthentification(struct Twin *);

#endif  /* AUTH_H */
