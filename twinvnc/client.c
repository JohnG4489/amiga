#include "system.h"
#include "client.h"
#include "general.h"
#include "rfb.h"
#include "util.h"


/*
    02-07-2017 (Seg)    Modif suite refonte artimétique 64 bits
    29-03-2016 (Seg)    Début ré-intégration transfert de fichier tight 1.x
    19-11-2006 (Seg)    Ajout de l'option NOREPORTMOUSE
    23-02-2004 (Seg)    Gestion du clipboard
    04-04-2004 (Seg)    Gestion des messages client
*/


/***** Prototypes */
LONG SetClientPixelFormat(struct Twin *, const struct PixelFormat *);
LONG SetEncodings(struct Twin *, struct ProtocolPrefs *);
LONG SetFrameBufferUpdateRequest(struct Twin *, BOOL);
LONG SetPointerEvent(struct Twin *, ULONG, ULONG, ULONG);
LONG SetKeyEvent(struct Twin *, ULONG, ULONG);
LONG SetScale(struct Twin *, ULONG);
LONG SetClientCutText(struct Twin *, BOOL);

LONG Clt_SetFileListRequest(struct Twin *, const char *, LONG);
LONG Clt_SetFileDownloadRequest(struct Twin *, const char *, const char *, const char *, const ULONG [2], ULONG);
LONG Clt_SetFileUploadRequest(struct Twin *, const char *, const char *, const char *, const ULONG [2], ULONG, const ULONG [2], BOOL);
LONG Clt_SetFileRemoveRequest(struct Twin *, const char *, const char *);
LONG Clt_SetFileUploadEndRequest(struct Twin *, ULONG, ULONG);

LONG Tight2_SetFileListRequest(struct Twin *, const char *, LONG);
LONG Tight2_SetFileDownloadRequest(struct Twin *, const char *);
LONG Tight2_SetFileDownloadDataRequest(struct Twin *, ULONG);
LONG Tight2_SetMkDirRequest(struct Twin *, const char *);
LONG Tight2_SetRenameRequest(struct Twin *, const char *, const char *);
LONG Tight2_SetRemoveRequest(struct Twin *, const char *);
LONG Tight2_SetFileUploadRequest(struct Twin *, const char *, const ULONG [2], BOOL);
LONG Tight2_SetFileUploadDataRequest(struct Twin *, const UBYTE *, ULONG);
LONG Tight2_SetFileUploadEndRequest(struct Twin *, ULONG, ULONG);

LONG Tight1_SetFileListRequest(struct Twin *, const char *, LONG);
LONG Tight1_SetFileDownloadRequest(struct Twin *, const char *);
LONG Tight1_SetFileUploadRequest(struct Twin *, const char *);

BOOL Clt_SetFTFullPathFileName(struct Twin *, const char *, const char *, const char *, const ULONG [2], ULONG, BOOL);
void Clt_FlushFTFullPathFileName(struct Twin *);


const struct {LONG EncoderID; char *EncoderName;} EncodingTypes[]=
{
    {RFB_ENCODING_TIGHT,    "Tight"},
    {RFB_ENCODING_ZRLE,     "ZRle"},
    {RFB_ENCODING_ZLIBRAW,  "ZLibRaw"},
    {RFB_ENCODING_ZLIBHEX,  "ZLibHex"},
    {RFB_ENCODING_HEXTILE,  "Hextile"},
    {RFB_ENCODING_CORRE,    "CoRRE"},
    {RFB_ENCODING_RRE,      "RRE"},
    {RFB_ENCODING_RAW,      "Raw"},
    {0,NULL}
};

const ULONG SubEncodingTypes[]=
{
    RFB_ENCODING_LASTRECT,
    RFB_ENCODING_NEWFBSIZE,
};

const ULONG CodecCursorId[]=
{
    RFB_ENCODING_XCURSOR,
    RFB_ENCODING_RICHCURSOR
};


/*****
    Changement du format du pixel
*****/

LONG SetClientPixelFormat(struct Twin *Twin, const struct PixelFormat *PixelFormat)
{
    struct ClientSetPixelFormat Msg;

    Msg.Type=RFB_SETPIXELFORMAT;
    Msg.Pad1=0;
    Msg.Pad2=0;
    Msg.PixelFormat=*PixelFormat;

    return Gal_WaitWrite(Twin,(void *)&Msg,sizeof(Msg));
}


/*****
    Choix de l'encodage
*****/

LONG SetEncodings(struct Twin *Twin, struct ProtocolPrefs *PPrefs)
{
    LONG Result;
    struct ClientSetEncodings Msg;
    ULONG TypeId=PPrefs->EncoderId;
    ULONG CountOfEncodingTypes=0;

    /* Compte des encoders */
    while(EncodingTypes[CountOfEncodingTypes].EncoderName!=NULL) CountOfEncodingTypes++;

    if(TypeId>=CountOfEncodingTypes) TypeId=0;

    Msg.Type=RFB_SETENCODINGS;
    Msg.Pad=0;
    Msg.CountOfEncodings=CountOfEncodingTypes+sizeof(SubEncodingTypes)/sizeof(ULONG);
    if(PPrefs->IsChangeZLib) Msg.CountOfEncodings++;
    if(PPrefs->IsUseJpeg) Msg.CountOfEncodings++;
    if(!PPrefs->IsNoLocalCursor) Msg.CountOfEncodings+=sizeof(CodecCursorId)/sizeof(ULONG);
    if(!PPrefs->IsNoCopyRect) Msg.CountOfEncodings++;
    if(PPrefs->IsReportMouse) Msg.CountOfEncodings++;

    Result=Gal_WaitWrite(Twin,(void *)&Msg,sizeof(Msg));
    if(Result>0)
    {
        ULONG i;

        /* Liste des codecs */
        for(i=0;i<CountOfEncodingTypes && Result>0;i++)
        {
            if(i==0) Result=Gal_WaitWrite(Twin,(void *)&EncodingTypes[TypeId].EncoderID,sizeof(ULONG));
            if(Result>0 && i!=TypeId) Result=Gal_WaitWrite(Twin,(void *)&EncodingTypes[i].EncoderID,sizeof(ULONG));
        }

        /* Sous codec CopyRect */
        if(Result>0 && !PPrefs->IsNoCopyRect)
        {
            ULONG CodecId=RFB_ENCODING_COPYRECT;
            Result=Gal_WaitWrite(Twin,(void *)&CodecId,sizeof(CodecId));
        }

        /* Sous codecs de gestion locale du curseur */
        if(Result>0 && !PPrefs->IsNoLocalCursor)
        {
            Result=Gal_WaitWrite(Twin,(void *)&CodecCursorId,sizeof(CodecCursorId));
        }

        /* Liste des autres sous-codecs */
        if(Result>0)
        {
            if(PPrefs->IsReportMouse)
            {
                ULONG CodecId=RFB_ENCODING_POINTERPOS;
                Result=Gal_WaitWrite(Twin,(void *)&CodecId,sizeof(CodecId));
            }
            if(Result>0) Result=Gal_WaitWrite(Twin,(void *)SubEncodingTypes,sizeof(SubEncodingTypes));
        }

        /* Niveau ZLib */
        if(Result>0 && PPrefs->IsChangeZLib)
        {
            ULONG Level=RFB_ENCODING_COMPRESS_LEVEL0+PPrefs->ZLibLevel;
            Result=Gal_WaitWrite(Twin,(void *)&Level,sizeof(Level));
        }

        /* Niveau Jpeg */
        if(Result>0 && PPrefs->IsUseJpeg)
        {
            ULONG Quality=RFB_ENCODING_QUALITY_LEVEL0+PPrefs->JpegQuality;
            Result=Gal_WaitWrite(Twin,(void *)&Quality,sizeof(Quality));
        }
    }

    return Result;
}


/*****
    Demande le rafraîchissement du FrameBuffer
*****/

LONG SetFrameBufferUpdateRequest(struct Twin *Twin, BOOL Incremental)
{
    struct ClientFrameBufferUpdateRequest Msg;
    struct FrameBuffer *fb=&Twin->Display.FrameBuffer;

    Msg.Type=RFB_FRAMEBUFFERUPDATEREQUEST;
    Msg.Incremental=Incremental;
    Msg.Rect.x=0;
    Msg.Rect.y=0;
    Msg.Rect.w=(UWORD)fb->Width;
    Msg.Rect.h=(UWORD)fb->Height;

    return Gal_WaitWrite(Twin,(void *)&Msg,sizeof(Msg));
}


/*****
    Evénement souris
*****/

LONG SetPointerEvent(struct Twin *Twin, ULONG x, ULONG y, ULONG Mask)
{
    struct ClientPointerEvent Msg;

    Msg.Type=RFB_POINTEREVENT;
    Msg.Mask=(UBYTE)Mask;
    Msg.x=(UWORD)x;
    Msg.y=(UWORD)y;

    return Gal_WaitWrite(Twin,(void *)&Msg,sizeof(Msg));
}


/*****
    Evénement clavier
*****/

LONG SetKeyEvent(struct Twin *Twin, ULONG Key, ULONG State)
{
    struct ClientKeyEvent Msg;

    Msg.Type=RFB_KEYEVENT;
    Msg.DownFlag=State;
    Msg.Pad=0;
    Msg.Key=Key;

    return Gal_WaitWrite(Twin,(void *)&Msg,sizeof(Msg));
}


/*****
    Scale d'écran de la part du serveur
*****/

LONG SetScale(struct Twin *Twin, ULONG Scale)
{
    struct ClientSetScale Msg;

    Msg.Type=RFB_SETSCALE;
    Msg.Scale=Scale;
    Msg.Pad=0;

    return Gal_WaitWrite(Twin,(void *)&Msg,sizeof(Msg));
}


/*****
    Envoi des données du clipboard
*****/

LONG SetClientCutText(struct Twin *Twin, BOOL IsNoClipboard)
{
    LONG Result=RESULT_SUCCESS;

    if(!IsNoClipboard)
    {
        ULONG Len=0;
        char *String=Sys_ReadClipboard(&Len);

        if(String!=NULL)
        {
            struct ClientCutText Msg;

            Msg.Type=RFB_CLIENTCUTTEXT;
            Msg.Pad1=0;
            Msg.Pad2=0;
            Msg.Length=Len;
            Result=Gal_WaitWrite(Twin,(void *)&Msg,sizeof(Msg));

            if(Result>0) Result=Gal_WaitWrite(Twin,(void *)String,Len);

            Sys_FreeMem((void *)String);
        }
    }

    return Result;
}


/*****
    Permet d'obtenir la liste des fichiers indépendament du protocol
*****/

LONG Clt_SetFileListRequest(struct Twin *Twin, const char *DirBase, LONG Flags)
{
    LONG Result=RESULT_SUCCESS;

    if((Twin->Connect.TightCapClient&FLG_TIGHT2_FILE_LIST_REQUEST))
    {
        Result=Tight2_SetFileListRequest(Twin,DirBase,Flags);
    }
    else if((Twin->Connect.TightCapClient&FLG_TIGHT1_FILE_LIST_REQUEST))
    {
        Result=Tight1_SetFileListRequest(Twin,DirBase,Flags);
    }

    return Result;
}


/*****
    Permet de lancer un download indépendament du protocole
*****/

LONG Clt_SetFileDownloadRequest(struct Twin *Twin, const char *LocalCurrentPath, const char *RemoteCurrentPath, const char *FileName, const ULONG Size64[2], ULONG Time)
{
    LONG Result=Clt_SetFTFullPathFileName(Twin,
        LocalCurrentPath,
        RemoteCurrentPath,
        FileName,
        Size64,
        Time,
        Twin->Connect.TightCapClient&FLG_TIGHT1_FILE_LIST_REQUEST?TRUE:FALSE);

    if(Result>0)
    {
        if((Twin->Connect.TightCapClient&FLG_TIGHT2_DOWNLOAD_START_REQUEST))
        {
            Result=Tight2_SetFileDownloadRequest(Twin,Twin->RemoteFullPathFileName);
        }
        else if((Twin->Connect.TightCapClient&FLG_TIGHT1_FILE_DOWNLOAD_REQUEST))
        {
            Result=Tight1_SetFileDownloadRequest(Twin,Twin->RemoteFullPathFileName);
        }
    }

    return Result;
}


/*****
    Envoi d'une requête d'upload
*****/

LONG Clt_SetFileUploadRequest(struct Twin *Twin, const char *LocalCurrentPath, const char *RemoteCurrentPath, const char *FileName, const ULONG Size64[2], ULONG Time, const ULONG Offset64[2], BOOL IsOverwrite)
{
    LONG Result=Clt_SetFTFullPathFileName(Twin,
        LocalCurrentPath,
        RemoteCurrentPath,
        FileName,
        Size64,
        Time,
        Twin->Connect.TightCapClient&FLG_TIGHT1_FILE_UPLOAD_REQUEST?TRUE:FALSE);

    if(Result>0)
    {
        if((Twin->Connect.TightCapClient&FLG_TIGHT2_UPLOAD_START_REQUEST))
        {
            Result=Tight2_SetFileUploadRequest(Twin,Twin->RemoteFullPathFileName,Offset64,IsOverwrite);
            //Sys_Printf("Tight2_SetFileUploadRequest: %ld\n",Result);
        }
        else if((Twin->Connect.TightCapClient&FLG_TIGHT1_FILE_UPLOAD_REQUEST))
        {
            Result=Tight1_SetFileUploadRequest(Twin,Twin->RemoteFullPathFileName);
            //Sys_Printf("Tight1_SetFileUploadRequest: %ld\n",Result);
        }
    }

    return Result;
}


/*****
    Envoi d'une requête de suppression de fichier
*****/

LONG Clt_SetFileRemoveRequest(struct Twin *Twin, const char *RemoteCurrentPath, const char *FileName)
{
    ULONG Size64[2]={0,0};
    LONG Result=Clt_SetFTFullPathFileName(Twin,
        "",
        RemoteCurrentPath,
        FileName,
        Size64,
        0,
        FALSE);

    if(Result>0)
    {
        if((Twin->Connect.TightCapClient&FLG_TIGHT2_REMOVE_REQUEST))
        {
            Result=Tight2_SetRemoveRequest(Twin,Twin->RemoteFullPathFileName);
        }
    }

    return Result;
}


/*****
    Envoi d'une requête de fin d'upload
*****/

LONG Clt_SetFileUploadEndRequest(struct Twin *Twin, ULONG Flags, ULONG ModTime)
{
    return Tight2_SetFileUploadEndRequest(Twin,Flags,ModTime);
}


/***************************************************************/
/***************************************************************/

/*****
    Envoi d'une requête pour obtenir une liste de fichiers
*****/

LONG Tight2_SetFileListRequest(struct Twin *Twin, const char *DirBase, LONG Flags)
{
    LONG Result;
    UBYTE Buffer[4+1];

    *((ULONG *)Buffer)=TIGHT2_FILE_LIST_REQUEST;
    Buffer[4]=(UBYTE)Flags;

    if((Result=Gal_WaitWrite(Twin,(void *)Buffer,sizeof(Buffer)))>0)
    {
        Result=Util_WaitWriteStringUTF8(Twin,DirBase);
    }

    return Result;
}


/*****
    Envoi d'une requête pour télécharger un fichier
*****/

LONG Tight2_SetFileDownloadRequest(struct Twin *Twin, const char *PathFileName)
{
    LONG Result;
    ULONG Code=TIGHT2_DOWNLOAD_START_REQUEST;

    if((Result=Gal_WaitWrite(Twin,(void *)&Code,sizeof(Code)))>0)
    {
        Result=Util_WaitWriteStringUTF8(Twin,PathFileName);
        if(Result>0)
        {
            ULONG Buffer[2]={0,0}; /* Offset 64 bits */
            Result=Gal_WaitWrite(Twin,Buffer,sizeof(Buffer));
        }
    }

    return Result;
}


/*****
    Envoi d'une requête pour réceptionner les données d'un fichier
*****/

LONG Tight2_SetFileDownloadDataRequest(struct Twin *Twin, ULONG Size)
{
    UBYTE Buffer[4+1+4+3]; /* long + byte + long + 3 pad */

    ((ULONG *)Buffer)[0]=TIGHT2_DOWNLOAD_DATA_REQUEST;
    Buffer[4]=0; /* pas de compression */
    ((ULONG *)Buffer)[2]=Size;
    Sys_MemCopy(&Buffer[5],&Buffer[8],sizeof(ULONG));

    return Gal_WaitWrite(Twin,(void *)&Buffer,9*sizeof(UBYTE));
}


/*****
    Envoi d'une requête de création de répertoire
*****/

LONG Tight2_SetMkDirRequest(struct Twin *Twin, const char *PathFileName)
{
    LONG Result;
    ULONG Code=TIGHT2_MKDIR_REQUEST;

    if((Result=Gal_WaitWrite(Twin,(void *)&Code,sizeof(Code)))>0)
    {
        Result=Util_WaitWriteStringUTF8(Twin,PathFileName);
    }

    return Result;
}


/*****
    Envoi d'une requête de renommage
*****/

LONG Tight2_SetRenameRequest(struct Twin *Twin, const char *OldPathFileName, const char *NewPathFileName)
{
    LONG Result;
    ULONG Code=TIGHT2_RENAME_REQUEST;

    if((Result=Gal_WaitWrite(Twin,(void *)&Code,sizeof(Code)))>0)
    {
        if((Result=Util_WaitWriteStringUTF8(Twin,OldPathFileName))>0)
        {
            Result=Util_WaitWriteStringUTF8(Twin,NewPathFileName);
        }
    }

    return Result;
}


/*****
    Envoi d'une requête de suppression de fichier
*****/

LONG Tight2_SetRemoveRequest(struct Twin *Twin, const char *PathFileName)
{
    LONG Result;
    ULONG Code=TIGHT2_REMOVE_REQUEST;

    if((Result=Gal_WaitWrite(Twin,(void *)&Code,sizeof(Code)))>0)
    {
        Result=Util_WaitWriteStringUTF8(Twin,PathFileName);
    }

    return Result;
}


/*****
    Envoi d'une requête d'upload
*****/

LONG Tight2_SetFileUploadRequest(struct Twin *Twin, const char *PathFileName, const ULONG Offset64[2], BOOL IsOverwrite)
{
    LONG Result;
    ULONG Code=TIGHT2_UPLOAD_START_REQUEST;

    if((Result=Gal_WaitWrite(Twin,(void *)&Code,sizeof(Code)))>0)
    {
        UBYTE Buffer[3+1+8];

        if((Result=Util_WaitWriteStringUTF8(Twin,PathFileName))>0)
        {
            Buffer[3]=IsOverwrite?1:0;
            ((ULONG *)Buffer)[1]=Offset64[0];
            ((ULONG *)Buffer)[2]=Offset64[1];
            Result=Gal_WaitWrite(Twin,(void *)&Buffer[3],9*sizeof(UBYTE));
        }
    }

    return Result;
}


/*****
    Envoi d'une requête d'upload de données
*****/

LONG Tight2_SetFileUploadDataRequest(struct Twin *Twin, const UBYTE *Data, ULONG Size)
{
    LONG Result;
    ULONG Code=TIGHT2_UPLOAD_DATA_REQUEST;

    if((Result=Gal_WaitWrite(Twin,(void *)&Code,sizeof(Code)))>0)
    {
        UBYTE Buffer[3+1+8];

        Buffer[3]=0; /* pas de compression */
        ((ULONG *)Buffer)[1]=Size;
        ((ULONG *)Buffer)[2]=Size;
        if((Result=Gal_WaitWrite(Twin,(void *)&Buffer[3],9*sizeof(UBYTE)))>0)
        {
            Result=Gal_WaitWrite(Twin,(void *)Data,Size);
        }
    }

    return Result;
}


/*****
    Envoi d'une requête de fin d'upload
*****/

LONG Tight2_SetFileUploadEndRequest(struct Twin *Twin, ULONG Flags, ULONG ModTime)
{
    UBYTE Buffer[4+2+8];
    ULONG ModTime64[2];

    Util_AmigaTimeToTime64(ModTime,ModTime64);

    /* Convertir ModTime pour Tight */
    //Sys_Printf("Tight2_SetFileUploadEndRequest\n");
    ((ULONG *)Buffer)[0]=TIGHT2_UPLOAD_END_REQUEST;
    *((UWORD *)&Buffer[4])=(UWORD)Flags;
    *((ULONG *)&Buffer[6])=ModTime64[0];
    *((ULONG *)&Buffer[10])=ModTime64[1];

    return Gal_WaitWrite(Twin,(void *)Buffer,sizeof(Buffer));
}


/***************************************************************/
/***************************************************************/

/*****
    Envoi d'une requete pour obtenir une liste de fichiers (protocole Tight 1)
*****/

LONG Tight1_SetFileListRequest(struct Twin *Twin, const char *DirBase, LONG Flags)
{
    LONG Result;
    struct ClientTight1FileListRequest Msg;

    Msg.Type=TIGHT1_FILE_LIST_REQUEST;
    Msg.Flags=(UBYTE)Flags;
    Msg.DirNameSize=Sys_StrLen(DirBase);

    if((Result=Gal_WaitWrite(Twin,(void *)&Msg,sizeof(Msg)))>0)
    {
        Result=Gal_WaitWrite(Twin,(void *)DirBase,Msg.DirNameSize);
    }

    return Result;
}


/*****
    Envoi d'une requête pour télécharger un fichier (protocole Tight 1)
*****/

LONG Tight1_SetFileDownloadRequest(struct Twin *Twin, const char *PathFileName)
{
    LONG Result;
    struct ClientTight1FileDownloadRequest Msg;
    LONG Len=Sys_StrLen(PathFileName);

    Msg.Type=TIGHT1_FILE_DOWNLOAD_REQUEST;
    Msg.CompressedLevel=0;
    Msg.FileNameSize=Len;
    Msg.Position=0;

    if((Result=Gal_WaitWrite(Twin,(void *)&Msg,sizeof(Msg)))>0)
    {
        Result=Gal_WaitWrite(Twin,(void *)PathFileName,Len*sizeof(char));
    }

    return Result;
}


/*****
    Envoi d'une requête pour envoyer un fichier (protocole Tight 1)
*****/

LONG Tight1_SetFileUploadRequest(struct Twin *Twin, const char *PathFileName)
{
    LONG Result=RESULT_SUCCESS;
    FILE *h=fopen(Twin->LocalFullPathFileName,"rb");

    if(h!=NULL)
    {
        struct ClientTight1FileUploadRequest Msg;
        LONG Len=Sys_StrLen(PathFileName);

        Msg.Type=TIGHT1_FILE_UPLOAD_REQUEST;
        Msg.CompressedLevel=0;
        Msg.FileNameSize=Len;
        Msg.Position=0;

        if((Result=Gal_WaitWrite(Twin,(void *)&Msg,sizeof(Msg)))>0)
        {
            if((Result=Gal_WaitWrite(Twin,(void *)PathFileName,Len*sizeof(char)))>0)
            {
                LONG Size;
                struct ClientTight1FileUploadData Msg;

                Msg.Type=TIGHT1_FILE_UPLOAD_DATA;
                Msg.CompressedLevel=0;

                do
                {
                    ULONG N64[2],R64[2],Val64[2],i;

                    Size=fread(Twin->Tmp,sizeof(UBYTE),sizeof(Twin->Tmp),h);

                    Msg.RealSize=Size;
                    Msg.CompressedSize=Size;

                    Result=Gal_WaitWrite(Twin,(void *)&Msg,sizeof(struct ClientTight1FileUploadData));
                    if(Result>0 && Size>0) Result=Gal_WaitWrite(Twin,(void *)Twin->Tmp,Size);

                    Val64[0]=0;
                    Val64[1]=Size;
                    Util_Add64(Val64,Twin->FTCurrentSize64);

                    N64[0]=Twin->FTCurrentSize64[0];
                    N64[1]=Twin->FTCurrentSize64[1];
                    for(i=0;i<16;i++) Util_ShiftL64(N64,FALSE);
                    Util_DivU64(N64,Twin->FTFinalSize64,R64);
                    Win_SetGaugeProgress(&Twin->WindowFileTransfer,(N64[1]*1000)>>16);
                } while(Result>0 && Size>0);

                if(Result>0)
                {
                    ULONG Time=0;
                    Result=Gal_WaitWrite(Twin,(void *)&Time,sizeof(Time));
                }
            }
        }

        fclose(h);
    }

    return Result;
}


/***************************************************************/
/***************************************************************/

/*****
    Permet d'allouer un buffer contenant le nom complet du fichier qui doit être téléchargé ou envoyé
*****/

BOOL Clt_SetFTFullPathFileName(struct Twin *Twin, const char *LocalPath, const char *RemotePath, const char *FileName, const ULONG Size64[2], ULONG ModTime, BOOL IsRemotePathTight1)
{
    LONG LocalPathLen=Sys_StrLen(LocalPath);
    LONG RemotePathLen=Sys_StrLen(RemotePath);
    LONG FileLen=Sys_StrLen(FileName);

    Clt_FlushFTFullPathFileName(Twin);
    Twin->LocalFullPathFileName=Sys_AllocMem(LocalPathLen+FileLen+sizeof(char)*2);
    Twin->RemoteFullPathFileName=Sys_AllocMem(RemotePathLen+FileLen+sizeof(char)*2);
    if(Twin->LocalFullPathFileName!=NULL && Twin->RemoteFullPathFileName!=NULL)
    {
        Twin->FTFinalSize64[0]=Size64[0];
        Twin->FTFinalSize64[1]=Size64[1];
        Twin->FTModTime=ModTime;
        Sys_SPrintf(Twin->LocalFullPathFileName,"%s%s",LocalPath,FileName);

        if(IsRemotePathTight1) Sys_SPrintf(Twin->RemoteFullPathFileName,"%s%s",RemotePath,FileName);
        else Sys_SPrintf(Twin->RemoteFullPathFileName,"%s%s/",RemotePath,FileName);

        //Sys_Printf("Local:%s\n",Twin->LocalFullPathFileName);
        //Sys_Printf("Remote:%s\n",Twin->RemoteFullPathFileName);
        return TRUE;
    }

    Clt_FlushFTFullPathFileName(Twin);

    return FALSE;
}


/*****
    Nettoyage du buffer alloué par Gal_SetDownloadFullPathFileName()
*****/

void Clt_FlushFTFullPathFileName(struct Twin *Twin)
{
    if(Twin->LocalFullPathFileName!=NULL) Sys_FreeMem((void *)Twin->LocalFullPathFileName);
    if(Twin->RemoteFullPathFileName!=NULL) Sys_FreeMem((void *)Twin->RemoteFullPathFileName);
    Twin->LocalFullPathFileName=NULL;
    Twin->RemoteFullPathFileName=NULL;
}
