#ifndef CONNECT_H
#define CONNECT_H

#ifndef SOCKET_H
#include "socket.h"
#endif

#ifndef RFB_H
#include "rfb.h"
#endif

#ifndef WINDOWSTATUS_H
#include "windowstatus.h"
#endif


struct Connect
{
    struct Socket *Socket;
    SOCKETFDSET FDSSignal;
    ULONG ServerVersion;
    ULONG ServerRevision;
    ULONG ServerWidth;
    ULONG ServerHeight;
    ULONG Security;
    BOOL IsTight;
    ULONG TightCapClient;
    ULONG TightCapServer;
    struct PixelFormat ServerPixelFormat;
    char ServerName[128];
    char Reason[1024];
};


/*****  VARIABLES ET FONCTIONS *******/
/*****  UTILISABLES PAR D'AUTRES *****/
/*****  BLOCKS DU PROJET *************/

extern LONG Conn_ContactServer(struct Twin *, void (*)(struct WindowStatus *, const char *, const char *));
extern LONG Conn_ReadTightCapabilities(struct Twin *, LONG, void *, void (*)(void *, LONG, const struct TightCapability *));

#endif  /* CONNECT_H */
