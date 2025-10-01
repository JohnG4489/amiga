#include "global.h"
#include "system.h"
#include "windowconnection.h"
#include "twin.h"
#include "general.h"
#include "gui.h"
#include "twinvnc_const.h"
#include <intuition/sghooks.h>
#include <intuition/gadgetclass.h>


/*
    25-02-2007 (Seg)    Remaniements mineurs suite à l'ajout de la sauvegarde de la config
    19-02-2007 (Seg)    Reprototypage de quelques fonctions
    07-03-2006 (Seg)    Gestion des proportions de l'interface en fonction du ratio de l'écran
    05-03-2006 (Seg)    Gestion du "Hide Char" pour le mot de passe
    16-01-2006 (Seg)    Gestion de l'historique des hosts
    09-05-2005 (Seg)    Gestion de la boîte de connexion
*/


/***** Prototypes */
BOOL Win_OpenWindowConnection(struct WindowConnection *, struct Window *, char *, ULONG *, char *, struct GlobalPrefs *, ULONG, struct FontDef *, struct MsgPort *);
void Win_CloseWindowConnection(struct WindowConnection *);
LONG Win_ManageWindowConnectionMessages(struct WindowConnection *, struct IntuiMessage *);
char *Win_GetWindowConnectionHost(struct WindowConnection *, ULONG *);
char *Win_GetWindowConnectionPassword(struct WindowConnection *);
void Win_SaveHostsHistory(struct WindowConnection *);

void InitWindowConnectionGadgets(struct FontDef *, ULONG);
void LoadHostStringInfoConfig(struct HistoryStringInfo *);
void LoadPasswordStringInfoConfig(struct PasswordStringInfo *);
void InitHostAndPortAndPassword(struct WindowConnection *, char *, ULONG, ULONG, char *);
void AddUniqueStringToBuffer(char *, LONG, char *);
LONG GetCountOfLines(char *);
BOOL GetStringFromBuffer(char *, LONG, char *, LONG *);

M_HOOK_PROTO(StringHostHook, struct Hook *, struct SGWork *, ULONG *);
M_HOOK_PROTO(StringPasswordHook, struct Hook *, struct SGWork *, ULONG *);


/***** Defines locaux */
#define WIN_WIDTH               400
#define WIN_HEIGHT              96
#define STRING_HOST_LEFT        50
#define STRING_HOST_TOP         16
#define STRING_HOST_WIDTH       (WIN_WIDTH-STRING_HOST_LEFT-20)
#define STRING_PASSWORD_LEFT    (STRING_HOST_LEFT)
#define STRING_PASSWORD_TOP     (STRING_HOST_TOP+26)
#define STRING_PASSWORD_WIDTH   (150)
#define BUTTON_WIDTH            90
#define BUTTON_HEIGHT           16
#define BUTTON_SPACE            16
#define BUTTON_CONNECT_LEFT     ((WIN_WIDTH-(BUTTON_WIDTH*3+BUTTON_SPACE*2))/2)
#define BUTTON_OPTIONS_LEFT     (BUTTON_CONNECT_LEFT+BUTTON_WIDTH+BUTTON_SPACE)
#define BUTTON_QUIT_LEFT        (BUTTON_OPTIONS_LEFT+BUTTON_WIDTH+BUTTON_SPACE)

#define VAR_HISTORYBUFFER       "TwinVNCHosts"
#define VAR_HIDECHAR            "TwinVNCHideChar"


/***** Variables globales */
struct TagItem ConnectionNoTag[]=
{
    {TAG_DONE,0UL}
};

struct TagItem ConnectionButtonTag[]=
{
    {GT_Underscore,'_'},
    {TAG_DONE,0UL}
};

struct TagItem ConnectionHostTag[]=
{
    {GTST_EditHook,(ULONG)NULL},
    {GTST_MaxChars,HOST_LENGTH},
    {TAG_DONE,0UL}
};

struct TagItem ConnectionPasswordTag[]=
{
    {GTST_EditHook,(ULONG)NULL},
    {GTST_MaxChars,PASSWORD_LENGTH},
    {TAG_DONE,0UL}
};

struct TagItem ConnectionCheckTag[]=
{
    {GTCB_Scaled,TRUE},
    {GT_Underscore,'_'},
    {TAG_DONE,0UL}
};


/***** Paramètres des gadgets */
struct TextAttr ConnectStdTextAttr={NULL,0,0,0};
struct TextAttr ConnectBoldTextAttr={NULL,0,0,0};
struct CustGadgetAttr HostGadgetAttr    ={LOC_CONNECTION_HOST,      "Host",     &ConnectStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr PasswordGadgetAttr={LOC_CONNECTION_PASSWORD,  "Password", &ConnectStdTextAttr,PLACETEXT_LEFT};
struct CustGadgetAttr ViewOnlyGadgetAttr={LOC_CONNECTION_VIEWONLY,  "_View only",&ConnectStdTextAttr,PLACETEXT_RIGHT};
struct CustGadgetAttr ConnectGadgetAttr ={LOC_CONNECTION_CONNECT,   "Connect",  &ConnectBoldTextAttr,PLACETEXT_IN};
struct CustGadgetAttr OptionsGadgetAttr ={LOC_CONNECTION_OPTIONS,   "_Options", &ConnectStdTextAttr,PLACETEXT_IN};
struct CustGadgetAttr QuitGadgetAttr    ={LOC_CONNECTION_QUIT,      "Quit",     &ConnectStdTextAttr,PLACETEXT_IN};


/***** Liste des gadgets */
enum
{
    TAGID_HOST,
    TAGID_PASSWORD,
    TAGID_VIEWONLY,
    TAGID_CONNECT,
    TAGID_OPTIONS,
    TAGID_QUIT,
};

struct CustomGadget ConnectionGadgetList[]=
{
    {STRING_KIND,   TAGID_HOST,     ConnectionHostTag,      STRING_HOST_LEFT,STRING_HOST_TOP,STRING_HOST_WIDTH,16,0,0,0,0,                  (APTR)&HostGadgetAttr,0,NULL,NULL},
    {STRING_KIND,   TAGID_PASSWORD, ConnectionPasswordTag,  STRING_PASSWORD_LEFT,STRING_PASSWORD_TOP,STRING_PASSWORD_WIDTH,16,0,0,0,0,      (APTR)&PasswordGadgetAttr,0,NULL,NULL},
    {CHECKBOX_KIND, TAGID_VIEWONLY, ConnectionCheckTag,     STRING_PASSWORD_LEFT+STRING_PASSWORD_WIDTH+10,STRING_PASSWORD_TOP,20,16,0,0,0,0,(APTR)&ViewOnlyGadgetAttr,0,NULL,NULL},
    {BUTTON_KIND,   TAGID_CONNECT,  ConnectionButtonTag,    BUTTON_CONNECT_LEFT,WIN_HEIGHT-20,BUTTON_WIDTH,BUTTON_HEIGHT,0,0,0,0,           (APTR)&ConnectGadgetAttr,0,NULL,NULL},
    {BUTTON_KIND,   TAGID_OPTIONS,  ConnectionButtonTag,    BUTTON_OPTIONS_LEFT,WIN_HEIGHT-20,BUTTON_WIDTH,BUTTON_HEIGHT,0,0,0,0,           (APTR)&OptionsGadgetAttr,0,NULL,NULL},
    {BUTTON_KIND,   TAGID_QUIT,     ConnectionButtonTag,    BUTTON_QUIT_LEFT,WIN_HEIGHT-20,BUTTON_WIDTH,BUTTON_HEIGHT,0,0,0,0,              (APTR)&QuitGadgetAttr,0,NULL,NULL},
    {KIND_DONE}
};

struct CustomMotif ConnectionMotifList[]=
{
    {MOTIF_RECTALT_COLOR2,0,0,WIN_WIDTH,WIN_HEIGHT,0,0,0,0,NULL},
    {MOTIF_RECTFILL_COLOR0,6,5,WIN_WIDTH-10,WIN_HEIGHT-30,0,0,0,0,NULL},
    {MOTIF_BEVELBOX_RECESSED,6,5,WIN_WIDTH-10,WIN_HEIGHT-30,0,0,0,0,NULL},
    {MOTIF_DONE}
};


/*****
    Ouverture d'une fenêtre de connexion
*****/

BOOL Win_OpenWindowConnection(struct WindowConnection *wc, struct Window *ParentWindow, char *Host, ULONG *Port, char *Password, struct GlobalPrefs *gp, ULONG DefaultPort, struct FontDef *fd, struct MsgPort *MPort)
{
    BOOL IsSuccess=FALSE,IsLocked;
    ULONG VerticalRatio=0;
    struct Screen *Scr=Gal_GetBestPublicScreen(ParentWindow,&IsLocked,&VerticalRatio);

    if(Scr!=NULL)
    {
        LONG WinWidth=WIN_WIDTH,WinHeight=WIN_HEIGHT;

        wc->GPrefs=gp;

        /* On initialise le Hook du host */
        wc->HostData.NewLine=wc->TmpHostPort;
        wc->HostData.NewLineMaxLen=HOST_LENGTH;
        LoadHostStringInfoConfig(&wc->HostData);
        wc->HostHook.h_Entry=(HOOKFUNC)&StringHostHook;
        wc->HostHook.h_SubEntry=NULL;
        wc->HostHook.h_Data=(APTR)&wc->HostData;
        ConnectionHostTag[0].ti_Data=(ULONG)&wc->HostHook;

        /* On initialise le Hook du mot de passe */
        LoadPasswordStringInfoConfig(&wc->PasswordData);
        wc->PasswordHook.h_Entry=(HOOKFUNC)&StringPasswordHook;
        wc->PasswordHook.h_SubEntry=NULL;
        wc->PasswordHook.h_Data=(APTR)&wc->PasswordData;
        ConnectionPasswordTag[0].ti_Data=(ULONG)&wc->PasswordHook;

        /* Initialisation des objets */
        Gui_GetBestWindowSize(&WinWidth,&WinHeight,VerticalRatio);
        InitWindowConnectionGadgets(fd,VerticalRatio);

        IsSuccess=Gui_OpenCompleteWindow(
                &wc->Window,Scr,MPort,
                __APPNAME__,Sys_GetString(LOC_CONNECTION_TITLE,"Connection"),
                -1,-1,WinWidth,WinHeight,
                ConnectionGadgetList,ConnectionMotifList,NULL);

        if(IsSuccess)
        {
            struct Gadget *Ptr=Gui_GetGadgetPtr(ConnectionGadgetList,TAGID_VIEWONLY);

            GT_SetGadgetAttrs(Ptr,NULL,NULL,
                    GTCB_Checked,(ULONG)gp->IsViewOnly,
                    TAG_DONE);

            InitHostAndPortAndPassword(wc,Host,*Port,DefaultPort,Password);

            RefreshGadgets(wc->Window.GadgetsBase->FirstGadget,wc->Window.WindowBase,NULL);
        }

        if(IsLocked) UnlockPubScreen(NULL,Scr);
    }

    return IsSuccess;
}


/*****
    Fermeture de la fenêtre de connexion
*****/

void Win_CloseWindowConnection(struct WindowConnection *wc)
{
    Gui_CloseCompleteWindow(&wc->Window);
}


/*****
    Traitement des événements de la fenêtre de connexion
*****/

LONG Win_ManageWindowConnectionMessages(struct WindowConnection *wc, struct IntuiMessage *msg)
{
    LONG Result=WIN_CONNECTION_RESULT_NONE;
    struct Window *win=wc->Window.WindowBase;
    struct Gadget *GadgetPtr;
    ULONG Value=0,Port;

    GT_RefreshWindow(win,NULL);

    switch(msg->Class)
    {
        case IDCMP_VANILLAKEY:
            switch(msg->Code)
            {
                case 13: /* Touche Entrée */
                    Result=WIN_CONNECTION_RESULT_CONNECT;
                    break;

                case 27: /* Touche Escape */
                    Result=WIN_CONNECTION_RESULT_QUIT;
                    break;

                case 'o':
                case 'O':
                    Result=WIN_CONNECTION_RESULT_OPTIONS;
                    break;

                case 'v':
                case 'V':
                    GadgetPtr=Gui_GetGadgetPtr(ConnectionGadgetList,TAGID_VIEWONLY);
                    Value=wc->GPrefs->IsViewOnly?FALSE:TRUE;
                    GT_SetGadgetAttrs(GadgetPtr,win,NULL,GTCB_Checked,Value,TAG_DONE);
                    wc->GPrefs->IsViewOnly=Value;
                    break;

                default:
                    ActivateGadget(wc->ActiveString,win,NULL);
                    break;
            }
            break;

        case IDCMP_GADGETUP:
            GadgetPtr=(struct Gadget *)msg->IAddress;

            switch(GadgetPtr->GadgetID)
            {
                case TAGID_CONNECT:
                    Result=WIN_CONNECTION_RESULT_CONNECT;
                    break;

                case TAGID_OPTIONS:
                    Result=WIN_CONNECTION_RESULT_OPTIONS;
                    break;

                case TAGID_QUIT:
                    Result=WIN_CONNECTION_RESULT_QUIT;
                    break;

                case TAGID_HOST:
                    wc->ActiveString=(struct Gadget *)msg->IAddress;
                    if(msg->Code!=27) ActivateGadget(Gui_GetGadgetPtr(ConnectionGadgetList,TAGID_PASSWORD),win,NULL);
                    break;

                case TAGID_PASSWORD:
                    wc->ActiveString=(struct Gadget *)msg->IAddress;
                    if(msg->Code!=27)
                    {
                        if(msg->Code!=9 && Sys_StrLen(Win_GetWindowConnectionHost(wc,&Port))!=0) Result=WIN_CONNECTION_RESULT_CONNECT;
                        else ActivateGadget(Gui_GetGadgetPtr(ConnectionGadgetList,TAGID_HOST),win,NULL);
                    }
                    break;

                case TAGID_VIEWONLY:
                    GT_GetGadgetAttrs(GadgetPtr,NULL,NULL,GTCB_Checked,&Value,TAG_DONE);
                    wc->GPrefs->IsViewOnly=Value?TRUE:FALSE;
                    break;
            }
            break;

        case IDCMP_GADGETHELP:
            /*
            {
                struct Gadget *GadgetPtr=(struct Gadget *)msg->IAddress;
                char *Title=wc->Window.WindowTitle;

                if(GadgetPtr!=NULL && GadgetPtr->UserData!=NULL) Title=(char *)GadgetPtr->UserData;
                TVNC_SetWindowTitles(win,Title,NULL);
            }
            */
            break;

        case IDCMP_CLOSEWINDOW:
            Result=WIN_CONNECTION_RESULT_QUIT;
            break;
    }

    return Result;
}


/*****
    Permet d'obtenir un pointeur sur le Host
*****/

char *Win_GetWindowConnectionHost(struct WindowConnection *wc, ULONG *Port)
{
    ULONG i,j=0,State=0;
    struct StringInfo *Ptr=((struct StringInfo *)Gui_GetGadgetPtr(ConnectionGadgetList,TAGID_HOST)->SpecialInfo);
    char *Host=(char *)Ptr->Buffer;

    /* Automate d'état pour décortiquer la chaîne */
    for(i=0;i<Ptr->NumChars;i++)
    {
        char Char=Host[i];

        switch(State)
        {
            case 0: /* On cherche le ":" */
                if(Char==':') State=2;
                else if(j<sizeof(wc->TmpHostPort)-1) wc->TmpHostPort[j++]=Host[i];
                     else State=1;
                break;

            case 1: /* On cherche le ":" */
                if(Char==':') State=2;
                break;

            case 2: /* On cherche le numéro de port */
                if(Char>='0' && Char<='9') {State=3; *Port=(ULONG)(Char-'0');}
                break;

            case 3: /* On scan tant qu'on a des numéros */
                if(Char>='0' && Char<='9') *Port=*Port*10+(ULONG)(Char-'0'); else State=3;
                break;
        }
    }
    wc->TmpHostPort[j]=0;

    return wc->TmpHostPort;
}


/*****
    Permet d'obtenir un pointeur sur le mot de passe
*****/

char *Win_GetWindowConnectionPassword(struct WindowConnection *wc)
{
    return wc->PasswordData.Password;
}


/*****
    Permet de sauvegarder le nouvel historique de Hosts dans la variable
    d'environnement.
*****/

void Win_SaveHostsHistory(struct WindowConnection *wc)
{
    struct Gadget *GadgetPtr=Gui_GetGadgetPtr(ConnectionGadgetList,TAGID_HOST);

    if(GadgetPtr!=NULL)
    {
        char *Host=(char *)((struct StringInfo *)GadgetPtr->SpecialInfo)->Buffer;

        AddUniqueStringToBuffer(wc->HostData.Buffer,sizeof(wc->HostData.Buffer),Host);
        SetVar(VAR_HISTORYBUFFER,wc->HostData.Buffer,-1,GVF_GLOBAL_ONLY|GVF_SAVE_VAR);
    }
}


/*****
    Initialisation des proportions des gadgets en fonctons des ressources
*****/

void InitWindowConnectionGadgets(struct FontDef *fd, ULONG VerticalRatio)
{
    static const ULONG CenterGadgetIDList[]={TAGID_CONNECT,TAGID_OPTIONS,TAGID_QUIT,(ULONG)-1};
    static const ULONG ShiftGadgetIDList[]={TAGID_HOST,TAGID_PASSWORD,TAGID_VIEWONLY,(ULONG)-1};

    Gui_CopyObjectPosition(ConnectionGadgetList,ConnectionMotifList,VerticalRatio);

    Gui_InitTextAttr(fd,&ConnectStdTextAttr,FS_NORMAL);
    Gui_InitTextAttr(fd,&ConnectBoldTextAttr,FSF_BOLD);

    /* Centrage de quelques gadgets... */
    Gui_DisposeGadgetsHorizontaly(0,WIN_WIDTH,ConnectionGadgetList,CenterGadgetIDList,TRUE);

    /* Décalage de quelques gadgets... */
    Gui_ShiftGadgetsHorizontaly(15,WIN_WIDTH-30,ConnectionGadgetList,ShiftGadgetIDList);
}


/*****
    Charge l'historique de hosts pour la zone de saisie du host
*****/

void LoadHostStringInfoConfig(struct HistoryStringInfo *Ptr)
{
    if(GetVar(VAR_HISTORYBUFFER,Ptr->Buffer,sizeof(Ptr->Buffer),GVF_GLOBAL_ONLY|GVF_BINARY_VAR)<=0)
    {
        Sys_StrCopy(Ptr->Buffer,"",sizeof(Ptr->Buffer));
    }
    Ptr->CountOfLines=GetCountOfLines(Ptr->Buffer);
    Ptr->LineNumber=0;
}


/*****
    Charge le caractère utilsé pour cacher le mot de passe
*****/

void LoadPasswordStringInfoConfig(struct PasswordStringInfo *Ptr)
{
    if(GetVar(VAR_HIDECHAR,&Ptr->HideChar,sizeof(Ptr->HideChar),GVF_DONT_NULL_TERM|GVF_GLOBAL_ONLY|GVF_BINARY_VAR)<=0)
    {
        Ptr->HideChar='*';
    }
}


/*****
    Permet de définir une chaîne par défaut dans le gadget Host
*****/

void InitHostAndPortAndPassword(struct WindowConnection *wc, char *Host, ULONG Port, ULONG DefaultPort, char *Password)
{
    wc->ActiveString=Gui_GetGadgetPtr(ConnectionGadgetList,TAGID_HOST);

    /* Initialisation de la zone de Host */
    wc->HostData.LineNumber=0;
    if(Host!=NULL && Sys_StrLen(Host)>0)
    {
        if(Port==DefaultPort) Sys_StrCopy(wc->TmpHostPort,Host,sizeof(wc->TmpHostPort));
        else Sys_SPrintf(wc->TmpHostPort,"%s:%ld",Host,Port);
        wc->ActiveString=Gui_GetGadgetPtr(ConnectionGadgetList,TAGID_PASSWORD);
    }
    else
    {
        LONG Len=sizeof(wc->TmpHostPort)-1;

        if(GetStringFromBuffer(wc->HostData.Buffer,0,wc->TmpHostPort,&Len)) wc->HostData.LineNumber=1;
        wc->TmpHostPort[Len]=0;
    }

    GT_SetGadgetAttrs(Gui_GetGadgetPtr(ConnectionGadgetList,TAGID_HOST),wc->Window.WindowBase,NULL,
            GTST_String,(ULONG)wc->TmpHostPort,
            TAG_DONE);

    /* Initialisation de la zone de password */
    if(Password!=NULL)
    {
        char Tmp[PASSWORD_LENGTH+1];
        LONG i,Len=Sys_StrLen(Password);

        for(i=0;i<Len && i<sizeof(Tmp)-1;i++)
        {
            Tmp[i]=wc->PasswordData.HideChar;
            wc->PasswordData.Password[i]=Password[i];
        }
        Tmp[i]=0;
        wc->PasswordData.Password[i]=0;

        GT_SetGadgetAttrs(Gui_GetGadgetPtr(ConnectionGadgetList,TAGID_PASSWORD),wc->Window.WindowBase,NULL,
                GTST_String,(ULONG)Tmp,
                TAG_DONE);
    }

    Sys_StrCopy(wc->HostData.NewLine,"",wc->HostData.NewLineMaxLen);

    /* Activation de la zone de saisie à remplir */
    ActivateGadget(wc->ActiveString,wc->Window.WindowBase,NULL);
}


/*****
    Ajoute une ligne de host en tête de l'historique.
    Cette fonction efface aussi les éventuels doublons qu'il peut y avoir.
*****/

void AddUniqueStringToBuffer(char *Buffer, LONG SizeOfBuffer, char *String)
{
    LONG StringLen=Sys_StrLen(String);

    if(StringLen>0)
    {
        BOOL IsEqual=FALSE;
        LONG BufferLen=Sys_StrLen(Buffer);
        LONG Pos=0;

        SizeOfBuffer--;
        if(StringLen>SizeOfBuffer) StringLen=SizeOfBuffer;

        /* On recherche un host identique dans la liste pour le supprimer */
        while(Pos<BufferLen && IsEqual==FALSE)
        {
            LONG i,LineLen=0;

            /* Calcule la longueur de la ligne */
            while(Pos+LineLen<BufferLen && Buffer[Pos+LineLen]!='\n') LineLen++;

            if(LineLen==StringLen)
            {
                /* On scan toute la ligne pour savoir si elle est identique */
                IsEqual=TRUE;
                for(i=0;i<LineLen && IsEqual;i++)
                {
                    if(Buffer[Pos+i]!=String[i]) IsEqual=FALSE;
                }
            }

            if(!IsEqual) Pos+=LineLen+sizeof(char);
        }

        /* Suppression de la String doublon */
        if(IsEqual)
        {
            /* On supprime le retour chariot ou le zéro de terminaison en prime */
            LONG Pos2=Pos+StringLen+1;
            while(Pos2<BufferLen+1) Buffer[Pos++]=Buffer[Pos2++];
            BufferLen-=Pos2-Pos;

            /* On réajuste la taille dans le cas où il n'y avait pas de retour chariot */
            if(BufferLen<0) BufferLen=0;
        }

        /* Décalage pour pouvoir insérer la nouvelle String */
        Pos=BufferLen;
        if(Pos+StringLen+1>SizeOfBuffer)
        {
            /* On raccourcit le buffer pour ne garder que les lignes complètes */
            for(Pos=SizeOfBuffer-(StringLen+1);Pos>=0 && Buffer[Pos]!='\n';Pos--);
        }
        BufferLen=StringLen;
        if(Pos>0)
        {
            BufferLen+=Pos+1;

            /* On décale si on a quelque chose à décaler */
            while(--Pos>=0) Buffer[StringLen+Pos+1]=Buffer[Pos];
        }

        /* Insertion du host en début de buffer */
        for(Pos=0;Pos<StringLen && Pos<SizeOfBuffer;Pos++) Buffer[Pos]=String[Pos];
        Buffer[Pos]='\n';

        /* On écrase la terminaison pour valider le buffer */
        Buffer[BufferLen]=0;
    }
}


/*****
    Permet de connaître le nombre de lignes présentes dans le buffer
*****/

LONG GetCountOfLines(char *Buffer)
{
    LONG Count=0,Pos=0;
    char CurChar;

    while((CurChar=Buffer[Pos])!=0)
    {
        if(CurChar=='\n') Count++;
        Pos++;
    }
    if(Pos>0) Count++;

    return Count;
}


/*****
    Permet d'extraire une ligne du buffer
*****/

BOOL GetStringFromBuffer(char *Buffer, LONG Line, char *String, LONG *MaxStringLen)
{
    BOOL IsLineExists=TRUE;
    LONG Pos=0,CurLine=0,MaxLen=*MaxStringLen;

    while(CurLine!=Line && IsLineExists)
    {
        /* On recherche la fin de la ligne */
        while(Buffer[Pos]!='\n' && Buffer[Pos]!=0) Pos++;

        if(Buffer[Pos++]==0) IsLineExists=FALSE;
        CurLine++;
    }

    *MaxStringLen=0;
    if(IsLineExists)
    {
        while(--MaxLen>0 && Buffer[Pos]!='\n' && Buffer[Pos]!=0)
        {
            String[(*MaxStringLen)++]=Buffer[Pos++];
        }
    }

    return IsLineExists;
}


/*****
    Hook pour la gestion des strings historique de saisie
*****/

M_HOOK_FUNC(StringHostHook, struct Hook *hook, struct SGWork *sgw, ULONG *msg)
{
    ULONG Result=~0;

    if(*msg==SGH_KEY)
    {
        struct HistoryStringInfo *sh=(struct HistoryStringInfo *)hook->h_Data;

        switch(sgw->EditOp)
        {
            case EO_NOOP:
                switch(sgw->IEvent->ie_Code)
                {
                    case 76: /* Flèche vers le haut */
                        if(sh->LineNumber>0)
                        {
                            LONG Len=sh->NewLineMaxLen;

                            sh->LineNumber--;
                            if(sh->LineNumber>0) GetStringFromBuffer(sh->Buffer,sh->LineNumber-1,sgw->WorkBuffer,&Len);
                            else Sys_StrCopy(sgw->WorkBuffer,sh->NewLine,Len);
                            sgw->WorkBuffer[Len]=0;
                            sgw->NumChars=Len;
                            sgw->BufferPos=Len;
                            sgw->Actions|=SGA_REDISPLAY;
                        }
                        break;

                    case 77: /* Flèche vers le bas */
                        if(sh->LineNumber<sh->CountOfLines)
                        {
                            LONG Len=sh->NewLineMaxLen;

                            if(sh->LineNumber==0)
                            {
                                LONG i;
                                for(i=0;i<sgw->NumChars && i<Len; i++) sh->NewLine[i]=sgw->WorkBuffer[i];
                                sh->NewLine[i]=0;
                            }

                            sh->LineNumber++;
                            GetStringFromBuffer(sh->Buffer,sh->LineNumber-1,sgw->WorkBuffer,&Len);
                            sgw->WorkBuffer[Len]=0;
                            sgw->NumChars=Len;
                            sgw->BufferPos=Len;
                            sgw->Actions|=SGA_REDISPLAY;
                        }
                        break;

                    case 69:
                        sgw->Actions|=SGA_END;
                        break;
                }
                break;

            case EO_MOVECURSOR:
                break;

            default:
                sh->LineNumber=0;
                break;
        }

        Result=0;
    }

    return Result;
}


/*****
    Hook pour la gestion des strings avec saisie de mot de passe
*****/

M_HOOK_FUNC(StringPasswordHook, struct Hook *hook, struct SGWork *sgw, ULONG *msg)
{
    ULONG Result=~0;

    if(*msg==SGH_KEY)
    {
        char *Buffer=((struct PasswordStringInfo *)hook->h_Data)->Password;
        char HideChar=((struct PasswordStringInfo *)hook->h_Data)->HideChar;
        LONG i,PrevPos=(sgw->StringInfo)->BufferPos;

        switch(sgw->EditOp)
        {
            case EO_NOOP:
                /* Test de la touche Esc */
                if(sgw->IEvent->ie_Code==69) sgw->Actions|=SGA_END;
                break;

            case EO_DELBACKWARD:
                for(i=sgw->BufferPos;PrevPos>0 && i<sgw->NumChars;i++) Buffer[i]=Buffer[i+1];
                Buffer[sgw->NumChars]=0;
                break;

            case EO_DELFORWARD:
                for(i=sgw->BufferPos;i<sgw->NumChars;i++) Buffer[i]=Buffer[i+1];
                Buffer[sgw->NumChars]=0;
                break;

            case EO_REPLACECHAR:
                Buffer[sgw->BufferPos-1]=sgw->Code;
                sgw->WorkBuffer[sgw->BufferPos-1]=HideChar;
                sgw->Actions|=SGA_REDISPLAY;
                break;

            case EO_BIGCHANGE:
            case EO_INSERTCHAR:
                {
                    LONG Len=sgw->BufferPos-PrevPos;

                    for(i=sgw->NumChars-1;i>=PrevPos;i--) Buffer[i]=Buffer[i-Len];
                    for(i=PrevPos;i<PrevPos+Len;i++)
                    {
                        Buffer[i]=sgw->WorkBuffer[i];
                        sgw->WorkBuffer[i]=HideChar;
                    }
                    Buffer[sgw->NumChars]=0;
                    sgw->Actions|=SGA_REDISPLAY;
                }
                break;

            case EO_CLEAR:
                Buffer[0]=0;
                break;

            case EO_MOVECURSOR:
                break;

            default:
                sgw->Actions&=~SGA_USE;
                break;
        }

        Result=0;
    }

    return Result;
}
