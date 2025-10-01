#include "system.h"
#include "input.h"
#include "twin.h"
#include "display.h"
#include "keysymdef.h"
#include "client.h"


/*
    25-06-2017 (Seg)    Fix gestion des accents avec un serveur Tight 2.x.
                        Les accents ne fonctionnent maintenant plus avec un serveur Tight 1.3.
    17-01-2016 (Seg)    Normalisation du code en pseudo classe C
    16-01-2016 (Seg)    Modification suite à la refonte de display.c
    04-03-2006 (Seg)    Amélioration de la gestion du clavier quand on utilise Terminal server
    20-02-2006 (Seg)    Remaniement de code pour restructurer la gestion des événements dans TwinVNC
    02-07-2005 (Seg)    Suppression de la gestion des touches de contrôle.
    23-02-2005 (Seg)    Meilleure gestion des touches accentuées pour les espagnols.
    15-01-2005 (Seg)    Gestion des touches F11 et F12 + meilleure gestion des touches
                        Begin/End, Insert, et ajout des touches Scroll Lock, Pause, etc...
                        sur clavier PC (valable pour MorphOS et peut-être AOS4).
    18-11-2004 (Seg)    Gestion des entrées clavier et souris
*/


/* Définitions pour la gestion de la cache des inputs */
#define INPUT_MSG_TYPE_MOUSE    1
#define INPUT_MSG_TYPE_KEYBOARD 2

/* Code Raw de certaines touches Amiga */
#define AKEY_CAPS_ON            98

/* Définitions pour la gestion des conversions de données clavier */
struct Convert
{
    char AKey;
    UWORD VKey;
    UWORD Accent;
};

#define KEY_MAIN_COUNT          25
static const struct Convert TableMain[KEY_MAIN_COUNT]=
{
    {8,XK_BackSpace,0},         {9,XK_Tab,0},
    {27,XK_Escape,0},           {13,XK_Return,0},
    {127,XK_Delete,0},
    {'â',XK_acircumflex,0},     {'Â',XK_Acircumflex,0},
    {'ê',XK_ecircumflex,0},     {'Ê',XK_Ecircumflex,0},
    {'î',XK_icircumflex,0},     {'Î',XK_Icircumflex,0},
    {'ô',XK_ocircumflex,0},     {'Ô',XK_Ocircumflex,0},
    {'û',XK_ucircumflex,0},     {'Û',XK_Ucircumflex,0},

    {'ä',XK_adiaeresis,0},      {'Ä',XK_Adiaeresis,0},
    {'ë',XK_ediaeresis,0},      {'Ë',XK_Ediaeresis,0},
    {'ï',XK_idiaeresis,0},      {'Ï',XK_Idiaeresis,0},
    {'ö',XK_odiaeresis,0},      {'Ö',XK_Odiaeresis,0},
    {'ü',XK_udiaeresis,0},      {'Ü',XK_Udiaeresis,0},
};

#define KEY_MAIN_TIGHT_COUNT    50
static const struct Convert TableMainTight[KEY_MAIN_TIGHT_COUNT]=
{
/*  //Table de conversion fonctionnelle avec un serveur Tight 1.3.10
    //Non fonctionnel en 2.x
    {8,XK_BackSpace,0},         {9,XK_Tab,0},
    {27,XK_Escape,0},           {13,XK_Return,0},
    {127,XK_Delete,0},
    {'â','a',XK_asciicircum},   {'Â','A',XK_asciicircum},
    {'ê','e',XK_asciicircum},   {'Ê','E',XK_asciicircum},
    {'î','i',XK_asciicircum},   {'Î','I',XK_asciicircum},
    {'ô','o',XK_asciicircum},   {'Ô','O',XK_asciicircum},
    {'û','u',XK_asciicircum},   {'Û','U',XK_asciicircum},

    {'ä','a',XK_diaeresis},     {'Ä','A',XK_diaeresis},
    {'ë','e',XK_diaeresis},     {'Ë','E',XK_diaeresis},
    {'ï','i',XK_diaeresis},     {'Ï','I',XK_diaeresis},
    {'ö','o',XK_diaeresis},     {'Ö','O',XK_diaeresis},
    {'ü','u',XK_diaeresis},     {'Ü','U',XK_diaeresis},
    {'ÿ','y',XK_diaeresis},

    {'á','a',XK_acute},         {'Á','A',XK_acute},
    {'é',XK_eacute,0},          {'É',XK_Eacute,0},
    {'í','i',XK_acute},         {'Í','I',XK_acute},
    {'ó','o',XK_acute},         {'Ó','O',XK_acute},
    {'ú','u',XK_acute},         {'Ú','U',XK_acute},
    {'ý','y',XK_acute},         {'Ý','Y',XK_acute},

    {'à',XK_agrave,0},          {'À',XK_Agrave,0},
    {'è',XK_egrave,0},          {'È',XK_Egrave,0},
    {'ì','i',XK_grave},         {'Ì','I',XK_grave},
    {'ò','o',XK_grave},         {'Ò','O',XK_grave},
    {'ù',XK_ugrave,0},          {'Ù',XK_Ugrave,0},

    {'ñ',XK_ntilde,0},          {'Ñ',XK_Ntilde,0}
*/
    //Table de conversion fonctionnelle à partir de Tight 2.x
    {8,XK_BackSpace,0},         {9,XK_Tab,0},
    {27,XK_Escape,0},           {13,XK_Return,0},
    {127,XK_Delete,0},

    {'â',XK_acircumflex,0},     {'Â',XK_Acircumflex,0},
    {'ê',XK_ecircumflex,0},     {'Ê',XK_Ecircumflex,0},
    {'î',XK_icircumflex,0},     {'Î',XK_Icircumflex,0},
    {'ô',XK_ocircumflex,0},     {'Ô',XK_Ocircumflex,0},
    {'û',XK_ucircumflex,0},     {'Û',XK_Ucircumflex,0},

    {'ä',XK_adiaeresis,0},      {'Ä',XK_Adiaeresis,0},
    {'ë',XK_ediaeresis,0},      {'Ë',XK_Ediaeresis,0},
    {'ï',XK_idiaeresis,0},      {'Ï',XK_Idiaeresis,0},
    {'ö',XK_odiaeresis,0},      {'Ö',XK_Odiaeresis,0},
    {'ü',XK_udiaeresis,0},      {'Ü',XK_Udiaeresis,0},
    {'ÿ',XK_ydiaeresis,0},

    {'á',XK_aacute,0},          {'Á',XK_Aacute,0},
    {'é',XK_eacute,0},          {'É',XK_Eacute,0},
    {'í',XK_iacute,0},          {'Í',XK_Iacute,0},
    {'ó',XK_oacute,0},          {'Ó',XK_Oacute,0},
    {'ú',XK_uacute,0},          {'Ú',XK_Uacute,0},
    {'ý',XK_yacute,0},          {'Ý',XK_Yacute,0},

    {'à',XK_agrave,0},          {'À',XK_Agrave,0},
    {'è',XK_egrave,0},          {'È',XK_Egrave,0},
    {'ì',XK_igrave,0},          {'Ì',XK_igrave,0},
    {'ò',XK_ograve,0},          {'Ò',XK_ograve,0},
    {'ù',XK_ugrave,0},          {'Ù',XK_Ugrave,0},

    {'ñ',XK_ntilde,0},          {'Ñ',XK_Ntilde,0}
};

#define KEY_ALT_NUMPAD_COUNT    14
static const struct Convert TableAltNumPad[KEY_ALT_NUMPAD_COUNT]=
{
    {'[',XK_Num_Lock,0},        {']',XK_Scroll_Lock,0},
    {'/',XK_Sys_Req,0},         {'*',XK_Print,0},
    {'7',XK_Home,0},            {'8',XK_Up,0},
    {'9',XK_Page_Up,0},         {'4',XK_Left,0},
    {'6',XK_Right,0},           {'1',XK_End,0},
    {'2',XK_Down,0},            {'3',XK_Page_Down,0},
    {'0',XK_Insert,0},          {'.',XK_Delete,0}
};

#define KEY_NUMPAD_COUNT        16
static const struct Convert TableNumPad[KEY_NUMPAD_COUNT]=
{
    {'0',XK_KP_0,0},            {'1',XK_KP_1,0},
    {'2',XK_KP_2,0},            {'3',XK_KP_3,0},
    {'4',XK_KP_4,0},            {'5',XK_KP_5,0},
    {'6',XK_KP_6,0},            {'7',XK_KP_7,0},
    {'8',XK_KP_8,0},            {'9',XK_KP_9,0},
    {'/',XK_KP_Divide,0},       {'*',XK_KP_Multiply,0},
    {'-',XK_KP_Subtract,0},     {'+',XK_KP_Add,0},
    {'.',XK_KP_Decimal,0},      {13,XK_KP_Enter,0}
};


/***** Prototypes */
void Inp_Init(struct Inputs *);
BOOL Inp_ManageInputMessages(struct Twin *, struct IntuiMessage *);
LONG Inp_SendInputMessages(struct Twin *);

void Inp_ManageKeyboardInput(struct Twin *, struct IntuiMessage *);
struct InputMsg *Inp_GetInputMsg(struct Inputs *);
struct InputMsg *Inp_AddInput(struct Inputs *);
BOOL Inp_AddMouseInput(struct Inputs *, ULONG, ULONG, ULONG);
BOOL Inp_AddKeyboardInput(struct Inputs *, ULONG, BOOL);
BOOL Inp_AddMouseWheel(struct Inputs *, ULONG, ULONG, ULONG, ULONG);
ULONG Inp_ConvertAmigaKey(const struct Convert *, ULONG, ULONG, ULONG *);


/*****
    Initialisation de la structure d'événements
*****/

void Inp_Init(struct Inputs *inp)
{
    inp->Offset=0;
    inp->Count=0;
    inp->PrevMouseMask=0;
    inp->PrevQualifier=0;
    inp->IsMiddleButtonEmulated=FALSE;
    inp->MaxCountOfMsg=sizeof(inp->Msg)/sizeof(struct InputMsg);
}


/*****
    Gestions des messages de la fenêtre de visualisation
*****/

BOOL Inp_ManageInputMessages(struct Twin *Twin, struct IntuiMessage *msg)
{
    struct Inputs *inp=&Twin->Inputs;

    switch(msg->Class)
    {
#ifndef __MORPHOS__
        case IDCMP_EXTENDEDMOUSE:
            if(msg->Code==IMSGCODE_INTUIWHEELDATA)
            {
                WORD WheelY=((struct IntuiWheelData *)msg->IAddress)->WheelY;

                /* Test deplacement vers le haut et le bas*/
                if(WheelY!=0) Inp_AddMouseWheel(inp,(ULONG)msg->MouseX,(ULONG)msg->MouseY,inp->PrevMouseMask,(WheelY<0?8:16));
            }
            break;
#endif
        case IDCMP_RAWKEY:
            switch(msg->Code)
            {
#ifndef __amigaos4__
                case 0x7a:
                    Inp_AddMouseWheel(inp,(ULONG)msg->MouseX,(ULONG)msg->MouseY,inp->PrevMouseMask,8);
                    break;

                case 0x7b:
                    Inp_AddMouseWheel(inp,(ULONG)msg->MouseX,(ULONG)msg->MouseY,inp->PrevMouseMask,16);
                    break;
#endif
                default:
                    Inp_ManageKeyboardInput(Twin,msg);
                    break;
            }
            break;

        case IDCMP_MOUSEBUTTONS:
            switch(msg->Code)
            {
                case MENUDOWN:   inp->PrevMouseMask|=4; break;
                case MENUUP:     inp->PrevMouseMask&=~4; break;
                case MIDDLEDOWN: inp->PrevMouseMask|=2; break;
                case MIDDLEUP:   inp->PrevMouseMask&=~2; break;
                case SELECTDOWN:
                    if(msg->Qualifier&IEQUALIFIER_RCOMMAND)
                    {
                        inp->PrevMouseMask|=2;
                        inp->IsMiddleButtonEmulated=TRUE;
                    } else inp->PrevMouseMask|=1;
                    break;

                case SELECTUP:
                    inp->PrevMouseMask&=~1;
                    if(inp->IsMiddleButtonEmulated)
                    {
                        inp->PrevMouseMask&=~2;
                        inp->IsMiddleButtonEmulated=FALSE;
                    }
                    break;
            }
            Inp_AddMouseInput(inp,(ULONG)msg->MouseX,(ULONG)msg->MouseY,inp->PrevMouseMask);
            break;

        case IDCMP_MOUSEMOVE:
            Inp_AddMouseInput(inp,(ULONG)msg->MouseX,(ULONG)msg->MouseY,inp->PrevMouseMask);
            break;
    }

    if(inp->Count>0) return TRUE;

    return FALSE;
}


/*****
    Conversion des coordonnées relatives à la fenêtre
*****/

LONG Inp_SendInputMessages(struct Twin *Twin)
{
    LONG Result=RESULT_SUCCESS,x=0,y=0;
    struct InputMsg *InputMsg;
    BOOL IsMouseMove=FALSE;
    struct GlobalPrefs *gp=&Twin->CurrentGlobalPrefs;

    while(Result>0 && (InputMsg=Inp_GetInputMsg(&Twin->Inputs))!=NULL)
    {
        switch(InputMsg->Type)
        {
            case INPUT_MSG_TYPE_MOUSE:
                {
                    ULONG CursorType=VIEWCURSOR_NORMAL;

                    if(gp->IsViewOnly) CursorType=VIEWCURSOR_VIEWONLY;
                    else if(!gp->IsCursor) CursorType=VIEWCURSOR_HIDDEN;
                    Disp_SetAmigaCursor(&Twin->Display,CursorType,(LONG)InputMsg->MouseX,(LONG)InputMsg->MouseY,FALSE);

                    if(VInf_GetRealMouseXY(&Twin->Display.ViewInfo,(LONG)InputMsg->MouseX,(LONG)InputMsg->MouseY,&x,&y))
                    {
                        if(!gp->IsViewOnly)
                        {
                            Result=SetPointerEvent(Twin,x,y,InputMsg->MouseMask);
                            IsMouseMove=TRUE;
                        }
                    }
                }
                break;

            case INPUT_MSG_TYPE_KEYBOARD:
               if(!gp->IsViewOnly) Result=SetKeyEvent(Twin,InputMsg->Key,(ULONG)InputMsg->DownFlag);
                break;
        }
    }

    if(IsMouseMove && gp->DisplayOption!=DISPLAY_OPTION_NODISPLAY) Disp_MoveCursor(&Twin->Display,x,y);

    return Result;
}


/*****
    Décodage des événements clavier
*****/

void Inp_ManageKeyboardInput(struct Twin *Twin, struct IntuiMessage *msg)
{
    UWORD RawCode=msg->Code;
    BOOL DownFlag=FALSE;
    struct GlobalPrefs *gb=&Twin->CurrentGlobalPrefs;

    /* Test si une touche est appuyée (sauf si c'est la touche CapsLock) */
    if(RawCode<=127) DownFlag=TRUE; else RawCode&=0x7f;

    if(RawCode!=AKEY_CAPS_ON)
    {
        ULONG Qualifier=(ULONG)(UWORD)msg->Qualifier;
        struct InputEvent ie;
        UBYTE Buffer[16];
        ULONG Key=0,Accent=0,Count;
        BOOL IsEmulPageMove=FALSE;
        struct Inputs *inp=&Twin->Inputs;

        /* On remap le code Raw pour savoir quelle touche a été appuyée */
        ie.ie_Class=IECLASS_RAWKEY;
        ie.ie_SubClass=0;
        ie.ie_Code=RawCode;
        ie.ie_Qualifier=Qualifier&(~(IEQUALIFIER_REPEAT|IEQUALIFIER_CONTROL|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND));
        ie.ie_EventAddress=(APTR *)*((ULONG *)msg->IAddress);

        /* Emulation de la touche "square" */
        if(RawCode==0 && Qualifier&IEQUALIFIER_RCOMMAND)
        {
            Key=XK_twosuperior;
            Qualifier&=~(IEQUALIFIER_RCOMMAND);

        }
        else
        {
            Count=MapRawKey(&ie,(STRPTR)Buffer,sizeof(Buffer),NULL);
            if(Count==1)
            {
                if(Buffer[0]<32 || Buffer[0]>127)
                {
                    ie.ie_Qualifier=Qualifier&(~(IEQUALIFIER_REPEAT|IEQUALIFIER_CONTROL|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND|IEQUALIFIER_LALT|IEQUALIFIER_RALT));
                    Count=MapRawKey(&ie,(STRPTR)Buffer,sizeof(Buffer),NULL);
                }
            }

            if(Count==0)
            {
                switch(RawCode)
                {
                    case 71: Key=XK_Insert; break;
                    case 72: Key=XK_Page_Up; break;
                    case 73: Key=XK_Page_Down; break;
                    case 107: Key=XK_Scroll_Lock; break;
                    case 112: Key=XK_Home; break;
                    case 113: Key=XK_End; break;
                    default:
                        Key=0;
                        ie.ie_Qualifier=Qualifier&(~(IEQUALIFIER_REPEAT|IEQUALIFIER_CONTROL|IEQUALIFIER_LCOMMAND|IEQUALIFIER_RCOMMAND|IEQUALIFIER_LALT|IEQUALIFIER_RALT));
                        Count=MapRawKey(&ie,(STRPTR)Buffer,sizeof(Buffer),NULL);
                        break;
                }
            }

            switch(Count)
            {
                case 1:
                    {
                        LONG Size=KEY_MAIN_COUNT;
                        const struct Convert *Table=TableMain;

                        if(Twin->Connect.IsTight) {Size=KEY_MAIN_TIGHT_COUNT; Table=TableMainTight;}
                        Key=Inp_ConvertAmigaKey(Table,Size,(ULONG)(UBYTE)Buffer[0],&Accent);
                        if(Qualifier&IEQUALIFIER_RCOMMAND)
                        {
                            switch(Key)
                            {
                                case XK_Delete: /* Pour gérer le CTRL+ALT+SUPPR (sous UAE) */
                                    Inp_AddKeyboardInput(inp,XK_Control_L,TRUE);
                                    Inp_AddKeyboardInput(inp,XK_Alt_L,TRUE);
                                    Inp_AddKeyboardInput(inp,XK_Delete,TRUE);
                                    Inp_AddKeyboardInput(inp,XK_Delete,FALSE);
                                    Inp_AddKeyboardInput(inp,XK_Alt_L,FALSE);
                                    Inp_AddKeyboardInput(inp,XK_Control_L,FALSE);
                                    Key=0;
                                    break;

                                default:
                                    if(Qualifier&IEQUALIFIER_NUMERICPAD)
                                    {
                                        Key=Inp_ConvertAmigaKey(TableAltNumPad,KEY_ALT_NUMPAD_COUNT,Key,&Accent);
                                    }
                                    break;
                            }
                        }
                        else
                        {
                            /* Test special dans le cas d'une touche du pavé numérique */
                            if(Qualifier&IEQUALIFIER_NUMERICPAD)
                            {
                                Key=Inp_ConvertAmigaKey(TableNumPad,KEY_NUMPAD_COUNT,Key,&Accent);
                            }
                        }
                    }
                    break;

                case 2:
                    /* Gestion des touches flêchées */
                    switch(Buffer[1])
                    {
                        case 65: Key=XK_Up; break;
                        case 66: Key=XK_Down; break;
                        case 67: Key=XK_Right; break;
                        case 68: Key=XK_Left; break;
                        case 84: Key=XK_Up; if(Qualifier&IEQUALIFIER_RSHIFT && gb->IsKeyboardEmul) {IsEmulPageMove=TRUE; Key=XK_Page_Up;} break;
                        case 83: Key=XK_Down; if(Qualifier&IEQUALIFIER_RSHIFT && gb->IsKeyboardEmul) {IsEmulPageMove=TRUE; Key=XK_Page_Down;} break;
                        case 90: Key=XK_Tab; break;
                    }
                    break;

                case 3:
                    /* Gestion des touches de fonction F1->F20 sans Shift */
                    if(Buffer[1]>=48 && Buffer[1]<=57)
                    {
                        Key=(Qualifier&IEQUALIFIER_RCOMMAND?XK_F11:XK_F1)+(ULONG)Buffer[1]-48;
                    }
                    if(Buffer[2]==65)
                    {
                        Key=XK_Left;
                        if(Qualifier&IEQUALIFIER_RSHIFT && gb->IsKeyboardEmul)
                        {
                            IsEmulPageMove=TRUE;
                            Key=XK_Home;
                        }
                    }
                    if(Buffer[2]==64)
                    {
                        Key=XK_Right;
                        if(Qualifier&IEQUALIFIER_RSHIFT && gb->IsKeyboardEmul)
                        {
                            IsEmulPageMove=TRUE;
                            Key=XK_End;
                        }
                    }
                    break;

                case 4:
                    if(Buffer[0]==0x9b && (Buffer[1]==0x34 || Buffer[1]==0x35) && Buffer[3]==0x7e)
                    {
                        switch(Buffer[2])
                        {
                            case 0x30: Key=XK_Insert; break;
                            case 0x31: Key=XK_Page_Up; break;
                            case 0x32: Key=XK_Page_Down; break;
                            case 0x33: Key=XK_Pause; break;
                            case 0x34: Key=XK_Home; break;
                            case 0x35: Key=XK_End; break;
                            case 0x36: Key=XK_Print; break;
                        }
                    }
                    else if(Buffer[0]==0x9b && Buffer[1]==0x32 && Buffer[3]==0x7e)
                    {
                        switch(Buffer[2])
                        {
                            case 0x30: Key=XK_F11; break;
                            case 0x31: Key=XK_F12; break;
                        }
                    }
                    /* Gestion des touches de fonction F1->F20 avec Shift */
                    else if(Buffer[2]>=48 && Buffer[2]<=57)
                    {
                        Key=(Qualifier&IEQUALIFIER_RCOMMAND?XK_F11:XK_F1)+(ULONG)Buffer[2]-48;
                    }
                    break;
            }
        }

        /* Test pour pouvoir gérer correctement les touches @ et # et éviter le switch Qwerty sous Windows */
        if((Qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_LALT))==(IEQUALIFIER_LSHIFT|IEQUALIFIER_LALT))
        {
            Qualifier&=~(IEQUALIFIER_LSHIFT|IEQUALIFIER_LALT);
        }

        /* Emulation des touches page down/up et begin end via la touche shift droite */
        if(IsEmulPageMove)
        {
            if(inp->PrevQualifier&IEQUALIFIER_RSHIFT) Inp_AddKeyboardInput(inp,XK_Shift_R,FALSE);
            inp->PrevQualifier&=~IEQUALIFIER_RSHIFT;
            Qualifier&=~IEQUALIFIER_RSHIFT;
        }

        if(!(inp->PrevQualifier&IEQUALIFIER_LCOMMAND) && (Qualifier&IEQUALIFIER_LCOMMAND) && !gb->IsNoLAmigaKey)
        {
            Inp_AddKeyboardInput(inp,XK_Control_L,TRUE);
            Inp_AddKeyboardInput(inp,XK_Escape,TRUE);
        }
        if(!(inp->PrevQualifier&IEQUALIFIER_CONTROL) && (Qualifier&IEQUALIFIER_CONTROL)) Inp_AddKeyboardInput(inp,XK_Control_L,TRUE);
        if(!(inp->PrevQualifier&IEQUALIFIER_LALT) && (Qualifier&IEQUALIFIER_LALT)) Inp_AddKeyboardInput(inp,XK_Alt_L,TRUE);
        if(!(inp->PrevQualifier&IEQUALIFIER_RALT) && (Qualifier&IEQUALIFIER_RALT)) Inp_AddKeyboardInput(inp,XK_Alt_R,TRUE);
        if(!(inp->PrevQualifier&IEQUALIFIER_LSHIFT) && (Qualifier&IEQUALIFIER_LSHIFT)) Inp_AddKeyboardInput(inp,XK_Shift_L,TRUE);
        if(!(inp->PrevQualifier&IEQUALIFIER_RSHIFT) && (Qualifier&IEQUALIFIER_RSHIFT)) Inp_AddKeyboardInput(inp,XK_Shift_R,TRUE);

        if(Key>0)
        {
            if(Key>=32 && Key<=127)
            {
                if(Qualifier&IEQUALIFIER_LALT) Inp_AddKeyboardInput(inp,XK_Alt_L,FALSE);
                if(Qualifier&IEQUALIFIER_RALT) Inp_AddKeyboardInput(inp,XK_Alt_R,FALSE);
            }

            if(Accent>0) Inp_AddKeyboardInput(inp,Accent,TRUE);
            Inp_AddKeyboardInput(inp,Key,DownFlag);
            if(Accent>0) Inp_AddKeyboardInput(inp,Accent,FALSE);

            if(Key>=32 && Key<=127)
            {
                if(Qualifier&IEQUALIFIER_LALT) Inp_AddKeyboardInput(inp,XK_Alt_L,TRUE);
                if(Qualifier&IEQUALIFIER_RALT) Inp_AddKeyboardInput(inp,XK_Alt_R,TRUE);
            }
        }

        if((inp->PrevQualifier&IEQUALIFIER_RSHIFT) && !(Qualifier&IEQUALIFIER_RSHIFT)) Inp_AddKeyboardInput(inp,XK_Shift_R,FALSE);
        if((inp->PrevQualifier&IEQUALIFIER_LSHIFT) && !(Qualifier&IEQUALIFIER_LSHIFT)) Inp_AddKeyboardInput(inp,XK_Shift_L,FALSE);
        if((inp->PrevQualifier&IEQUALIFIER_RALT) && !(Qualifier&IEQUALIFIER_RALT)) Inp_AddKeyboardInput(inp,XK_Alt_R,FALSE);
        if((inp->PrevQualifier&IEQUALIFIER_LALT) && !(Qualifier&IEQUALIFIER_LALT)) Inp_AddKeyboardInput(inp,XK_Alt_L,FALSE);
        if((inp->PrevQualifier&IEQUALIFIER_CONTROL) && !(Qualifier&IEQUALIFIER_CONTROL)) Inp_AddKeyboardInput(inp,XK_Control_L,FALSE);
        if((inp->PrevQualifier&IEQUALIFIER_LCOMMAND) && !(Qualifier&IEQUALIFIER_LCOMMAND) && !gb->IsNoLAmigaKey)
        {
            Inp_AddKeyboardInput(inp,XK_Escape,FALSE);
            Inp_AddKeyboardInput(inp,XK_Control_L,FALSE);
        }

        inp->PrevQualifier=Qualifier;
    }
}


/*****
    Récupère un événement dans le buffer circulaire d'événements
*****/

struct InputMsg *Inp_GetInputMsg(struct Inputs *inp)
{
    struct InputMsg *msg=NULL;

    if(inp->Count>0)
    {
        if(inp->Offset>=inp->MaxCountOfMsg) inp->Offset=0;
        msg=&inp->Msg[inp->Offset++];
        inp->Count--;
    }

    return msg;
}


/*****
    Ajout d'un événement
*****/

struct InputMsg *Inp_AddInput(struct Inputs *inp)
{
    struct InputMsg *Msg=NULL;

    if(inp->Count<inp->MaxCountOfMsg)
    {
        ULONG NewPos=inp->Offset+inp->Count++;

        if(NewPos>=inp->MaxCountOfMsg) NewPos-=inp->MaxCountOfMsg;
        Msg=&inp->Msg[NewPos];
    }

    return Msg;
}


/*****
    Ajout d'un événement souris
*****/

BOOL Inp_AddMouseInput(struct Inputs *inp, ULONG MouseX, ULONG MouseY, ULONG Mask)
{
    struct InputMsg *Msg=Inp_AddInput(inp);

    if(Msg!=NULL)
    {
        Msg->Type=INPUT_MSG_TYPE_MOUSE;
        Msg->MouseX=(UWORD)MouseX;
        Msg->MouseY=(UWORD)MouseY;
        Msg->MouseMask=Mask;
        return TRUE;
    }

    return FALSE;
}


/*****
    Ajout d'un événement clavier
*****/

BOOL Inp_AddKeyboardInput(struct Inputs *inp, ULONG Key, BOOL DownFlag)
{
    struct InputMsg *Msg=Inp_AddInput(inp);

    if(Msg!=NULL)
    {
        Msg->Type=INPUT_MSG_TYPE_KEYBOARD;
        Msg->Key=Key;
        Msg->DownFlag=DownFlag;
        return TRUE;
    }

    return FALSE;
}


/*****
    Ajout d'une impulsion molette
*****/

BOOL Inp_AddMouseWheel(struct Inputs *inp, ULONG MouseX, ULONG MouseY, ULONG CurrentMask, ULONG WheelMask)
{
    BOOL IsBufferNotFull=TRUE;

    CurrentMask|=WheelMask;
    if(!Inp_AddMouseInput(inp,MouseX,MouseY,CurrentMask)) IsBufferNotFull=FALSE;
    CurrentMask&=~WheelMask;
    if(!Inp_AddMouseInput(inp,MouseX,MouseY,CurrentMask)) IsBufferNotFull=FALSE;

    return IsBufferNotFull;
}


/*****
    Conversion d'une clef clavier ANSI en code VNC
*****/

ULONG Inp_ConvertAmigaKey(const struct Convert *Table, ULONG Count, ULONG AKey, ULONG *Accent)
{
    ULONG i, VKey=AKey;

    *Accent=0;
    for(i=0;i<Count;i++)
    {
        if(Table[i].AKey==(char)AKey)
        {
            VKey=Table[i].VKey;
            *Accent=Table[i].Accent;
            break;
        }
    }

    return VKey;
}
