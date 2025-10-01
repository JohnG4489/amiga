#include "system.h"
#include "auth.h"
#include "general.h"
#include "twin.h"
#include "d3des.h"
#include "rfb.h"


/*
    23-06-2017 (Seg)    Fix authentification sans mot de passe avec Tight
    20-02-2016 (Seg)    Gestion de l'authentification Tight
    18-01-2016 (Seg)    Mise à la norme du code.
    29-04-2005 (Seg)    Amélioration de l'authentification quand le serveur n'a
                        pas de mot de passe.
    26-04-2005 (Seg)    Correction d'un pb d'authentification quand le serveur
                        n'a pas de mot de passe.
    04-04-2004 (Seg)    Gestion de l'authentification du client avec le serveur
*/


/***** Prototypes */
LONG Auth_PerformAuthentification(struct Twin *);
LONG Auth_CheckAuthentification(struct Twin *, ULONG);
LONG Auth_PerformVNCAuthentification(struct Twin *);
LONG Auth_ReadReason(struct Twin *);
LONG Auth_PerformTightAuthentification(struct Twin *);

void Auth_ReadTightCapAuth(void *, LONG, const struct TightCapability *);


/*****
    Gestion de l'authentification
*****/

LONG Auth_PerformAuthentification(struct Twin *Twin)
{
    LONG Result=RESULT_AUTHENTIFICATION_INVALID;
    ULONG Security=RFB_SECURITYTYPE_INVALID;
    struct Connect *co=&Twin->Connect;
    ULONG VerRev=(co->ServerVersion<<16)+co->ServerRevision;

    /* Gestion protocoles 3.3 à 3.6 */
    if(VerRev>=0x00030003 && VerRev<=0x00030006)
    {
        Result=Gal_WaitRead(Twin,(void *)&Security,sizeof(Security));
    }

    /* Gestion protocoles 3.7 à 3.8 */
    if(VerRev>=0x00030007 && VerRev<=0x00030008)
    {
        UBYTE Count=0;

        /* Lecture des types de sécurité supportés par le serveur */
        Result=Gal_WaitRead(Twin,(void *)&Count,sizeof(Count));
        if(Result>0)
        {
            if(Count>0)
            {
                BOOL IsAuthNone=FALSE,IsAuthVnc=FALSE,IsAuthTight=FALSE;
                ULONG i;

                /* Récupération de la liste des authentifications reconnues par le serveur */
                for(i=0;Result>0 && i<(ULONG)Count;i++)
                {
                    UBYTE Data=0;
                    Result=Gal_WaitRead(Twin,(void *)&Data,sizeof(Data));
                    if(Result>0)
                    {
                        if(Data==RFB_SECURITYTYPE_NONE) IsAuthNone=TRUE;
                        if(Data==RFB_SECURITYTYPE_VNCAUTH) IsAuthVnc=TRUE;
                        if(Data==RFB_SECURITYTYPE_TIGHT) {co->IsTight=TRUE; IsAuthTight=TRUE;}
                    }
                }

                /* Choix d'un type d'authentification */
                if(Result>0)
                {
                    UBYTE AuthUsed=RFB_SECURITYTYPE_INVALID;

                    if(IsAuthTight) AuthUsed=RFB_SECURITYTYPE_TIGHT;
                    else if(IsAuthVnc) AuthUsed=RFB_SECURITYTYPE_VNCAUTH;
                    else if(IsAuthNone) AuthUsed=RFB_SECURITYTYPE_NONE;

                    Result=Gal_WaitWrite(Twin,(void *)&AuthUsed,sizeof(AuthUsed));
                    if(Result>0) Security=(ULONG)AuthUsed;
                }
            }
        }
    }

    if(Result>0)
    {
        Result=Auth_CheckAuthentification(Twin,Security);
    }

    return Result;
}


/*****
    Vérification de l'authentification
*****/

LONG Auth_CheckAuthentification(struct Twin *Twin, ULONG Security)
{
    LONG Result=RESULT_SUCCESS;
    struct Connect *co=&Twin->Connect;
    ULONG VerRev=(co->ServerVersion<<16)+co->ServerRevision;

    co->Security=Security;

    switch(Security)
    {
        case RFB_SECURITYTYPE_NONE:
            break;

        case RFB_SECURITYTYPE_VNCAUTH:
            Result=Auth_PerformVNCAuthentification(Twin);
            break;

        case RFB_SECURITYTYPE_TIGHT:
            Result=Auth_PerformTightAuthentification(Twin);
            break;

        default:
            Result=Auth_ReadReason(Twin);
            if(Result>0) Result=RESULT_AUTHENTIFICATION_INVALID;
            break;
    }

    if(Result>0)
    {
        if(VerRev>=0x00030008 || (VerRev<=0x00030007 && Security!=RFB_SECURITYTYPE_NONE))
        {
            ULONG ReturnCode=0;

            Result=Gal_WaitRead(Twin,(void *)&ReturnCode,sizeof(ReturnCode));
            if(Result>0)
            {
                switch(ReturnCode)
                {
                    case RFB_VNCAUTH_OK:
                        break;

                    case RFB_VNCAUTH_FAILED:
                        Result=Auth_ReadReason(Twin);
                        if(Result>0) Result=RESULT_AUTHENTIFICATION_FAILED;
                        break;

                    case RFB_VNCAUTH_TOOMANY:
                        Result=Auth_ReadReason(Twin);
                        if(Result>0) Result=RESULT_AUTHENTIFICATION_TOOMANY;
                        break;

                    default:
                        Result=RESULT_AUTHENTIFICATION_FAILED;
                        break;
                }
            }
        }
    }

    return Result;
}


/*****
    Gestion de l'authentification VNC
*****/

LONG Auth_PerformVNCAuthentification(struct Twin *Twin)
{
    unsigned char Challenge[16];
    LONG Result=Gal_WaitRead(Twin,(void *)Challenge,sizeof(Challenge));

    if(Result>0)
    {
        unsigned char Key[RFB_SIZEOF_PASSWORD];
        char *Password=Twin->Password;
        ULONG i,Len=Sys_StrLen(Password);

        /* On encode le challenge avec le mot de passe comme clef de cryptage */
        for(i=0;i<sizeof(Key);i++)
        {
            Key[i]=0;
            if(i<Len) Key[i]=Password[i];
        }
        deskey(Key,EN0);
        for(i=0;i<sizeof(Challenge);i+=8) des(&Challenge[i],&Challenge[i]);

        /* On envoie le challenge crypté */
        Result=Gal_WaitWrite(Twin,(void *)Challenge,sizeof(Challenge));
    }

    return Result;
}


/*****
    Gestion de l'authentification VNC
*****/

LONG Auth_ReadReason(struct Twin *Twin)
{
    ULONG Len=0;
    LONG Result=Gal_WaitRead(Twin,(void *)&Len,sizeof(Len));

    if(Result>0)
    {
        Result=Gal_WaitFixedRead(Twin,Twin->Connect.Reason,sizeof(Twin->Connect.Reason)-1,Len);
        if(Result>0) Twin->Connect.Reason[Result]=0;
    }

    return Result;
}


/*****
    Gestion de l'authentification Tight
*****/

LONG Auth_PerformTightAuthentification(struct Twin *Twin)
{
    LONG TunnelCount=0;
    LONG AuthCode=TIGHT_AUTH_NONE;
    LONG Result=Gal_WaitRead(Twin,(void *)&TunnelCount,sizeof(TunnelCount));

    if(Result>0 && TunnelCount>0)
    {
        /* On récupère le set d'options tunnel, mais on en prend aucun au final */
        Result=Conn_ReadTightCapabilities(Twin,TunnelCount,NULL,NULL);
        if(Result>0)
        {
            ULONG TagNoTunnel=0; /* NOTUNNEL */
            Result=Gal_WaitWrite(Twin,(void *)&TagNoTunnel,sizeof(TagNoTunnel));
        }
    }

    if(Result>0)
    {
        LONG AuthTypeCount=0;

        Result=Gal_WaitRead(Twin,(void *)&AuthTypeCount,sizeof(AuthTypeCount));
        if(Result>0 && AuthTypeCount>0)
        {
            ULONG AuthFlags=0;

            /* On récupère les types d'authentifications supportés par le serveur */
            Result=Conn_ReadTightCapabilities(Twin,AuthTypeCount,(void *)&AuthFlags,Auth_ReadTightCapAuth);
            if(Result>0)
            {
                /* On choisit l'authentification VNC si elle existe */
                if(AuthFlags&FLG_TIGHT_AUTH_VNC) AuthCode=TIGHT_AUTH_VNC;
                Result=Gal_WaitWrite(Twin,(void *)&AuthCode,sizeof(AuthCode));
            }
        }
    }

    if(Result>0 && AuthCode!=TIGHT_AUTH_NONE)
    {
        Result=Auth_PerformVNCAuthentification(Twin);
    }

    return Result;
}

/*****
    Hook pour interpréter les options d'authentification
*****/

void Auth_ReadTightCapAuth(void *UserPtr, LONG Idx, const struct TightCapability *Cap)
{
    ULONG Flags=*((ULONG *)UserPtr);
    if(Cap->Code==TIGHT_AUTH_VNC) Flags|=FLG_TIGHT_AUTH_VNC;

    //if(Cap->Code>-512) Sys_Printf("Code=%ld, Vendor=%s, Name=%s\n",Cap->Code,Cap->Vendor,Cap->Name);
    //else Sys_Printf("Code=%04lx, Vendor=%s, Name=%s\n",Cap->Code,Cap->Vendor,Cap->Name);

    *((ULONG *)UserPtr)=Flags;
}

