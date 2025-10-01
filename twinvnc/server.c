#include "system.h"
#include "rfb.h"
#include "server.h"
#include "client.h"
#include "twin.h"
#include "general.h"
#include "filemanager.h"
#include "filescheduler.h"
#include "util.h"

#include "decodercursor.h"
#include "decoderraw.h"
#include "decodercopyrect.h"
#include "decoderrre.h"
#include "decodercorre.h"
#include "decoderhextile.h"
#include "decoderzlibraw.h"
#include "decoderzrle.h"
#include "decodertight.h"
#include <errno.h>

/*
    20-02-2022 (Seg)    Remaniement des noms de fonctions
    02-07-2017 (Seg)    Modif suite refonte artimétique 64 bits
    29-03-2016 (Seg)    Début ré-intégration transfert de fichier tight 1.x
    02-03-2016 (Seg)    Ajout API transfert de fichier
    17-01-2016 (Seg)    Modifications suite au changement de l'API decoding
    16-01-2016 (Seg)    Modification suite à la refonte de display.c
    23-02-2005 (Seg)    Gestion du clipboard serveur
    04-04-2004 (Seg)    Gestion des messages serveur
*/


/***** Prototypes */
LONG Srv_ManageServerMessage(struct Twin *);
LONG Srv_ManageTight1Message(struct Twin *, UBYTE *);
LONG Srv_ManageTight2Message(struct Twin *, ULONG);

LONG Srv_GetFrameBufferUpdate(struct Twin *, struct ServerFrameBufferUpdate *);
LONG Srv_GetColourMapEntries(struct Twin *, struct ServerSetColourMapEntries *);
LONG Srv_GetServerCutText(struct Twin *, struct ServerCutText *);
LONG Srv_ResizeDisplay(struct Twin *, LONG, LONG);

LONG Srv_Tight2_GetFileListData(struct Twin *, struct FileManager *);
LONG Srv_Tight2_OpenFileDownload(struct Twin *, const char *);
LONG Srv_Tight2_CloseFileDownload(struct Twin *);
LONG Srv_Tight2_GetFileDownloadData(struct Twin *);
LONG Srv_Tight2_OpenFileUpload(struct Twin *, const char *);
LONG Srv_Tight2_CloseFileUpload(struct Twin *);
LONG Srv_Tight2_SetFileUploadData(struct Twin *);

LONG Srv_Tight1_GetFileListData(struct Twin *, struct FileManager *, struct ServerTight1FileListData *);
LONG Srv_Tight1_GetFileDownloadData(struct Twin *, struct ServerTight1FileDownloadData *);


/*****
    Gestion des messages venant du serveur
*****/

LONG Srv_ManageServerMessage(struct Twin *Twin)
{
    UBYTE Buffer[16];
    LONG Result=Gal_WaitRead(Twin,(void *)Buffer,1);

    if(Result>0)
    {
        switch(Buffer[0])
        {
            case RFB_FRAMEBUFFERUPDATE:
                Result=Gal_WaitRead(Twin,(void *)&Buffer[1],sizeof(struct ServerFrameBufferUpdate)-sizeof(UBYTE));
                if(Result>0) Result=Srv_GetFrameBufferUpdate(Twin,(struct ServerFrameBufferUpdate *)Buffer);
                break;

            case RFB_SETCOLOURMAPENTRIES:
                Result=Gal_WaitRead(Twin,(void *)&Buffer[1],sizeof(struct ServerSetColourMapEntries)-sizeof(UBYTE));
                if(Result>0) Result=Srv_GetColourMapEntries(Twin,(struct ServerSetColourMapEntries *)Buffer);
                break;

            case RFB_BELL:
                DisplayBeep(NULL);
                break;

            case RFB_SERVERCUTTEXT:
                Result=Gal_WaitRead(Twin,(void *)&Buffer[1],sizeof(struct ServerCutText)-sizeof(UBYTE));
                if(Result>0) Result=Srv_GetServerCutText(Twin,(struct ServerCutText *)Buffer);
                break;

            case RFB_RESIZEFRAMEBUFFER:
                Result=Gal_WaitRead(Twin,(void *)&Buffer[1],sizeof(struct ServerResizeFrameBuffer)-sizeof(UBYTE));
                if(Result>0)
                {
                    struct ServerResizeFrameBuffer *Msg=(struct ServerResizeFrameBuffer *)Buffer;
                    Result=Srv_ResizeDisplay(Twin,(LONG)Msg->Width,(LONG)Msg->Height);
                    if(Result>0) Twin->IsRefreshIncremental=FALSE;
                }
                break;

            case TIGHT1_FILE_LIST_DATA:
            case TIGHT1_FILE_DOWNLOAD_DATA:
            case TIGHT1_FILE_UPLOAD_CANCEL:
            case TIGHT1_FILE_DOWNLOAD_FAILED:
                Result=Srv_ManageTight1Message(Twin,Buffer);
                break;

            case RFB_TIGHT_CMD_ID:
                Result=Gal_WaitRead(Twin,(void *)&Buffer[1],3);
                if(Result>0) Result=Srv_ManageTight2Message(Twin,*(ULONG *)Buffer);
                break;

            default:
                //Sys_Printf("Unknown: %ld\n",(ULONG)Buffer[0]);
                break;
        }
    }

    return Result;
}


/*****
    Gestion des messages du protocole Tight 1
*****/

LONG Srv_ManageTight1Message(struct Twin *Twin, UBYTE *Msg)
{
    LONG Result=RESULT_SUCCESS;
    struct WindowFileTransfer *wft=&Twin->WindowFileTransfer;

    switch(*Msg)
    {
        case TIGHT1_FILE_LIST_DATA:
            Result=Gal_WaitRead(Twin,(void *)&Msg[1],sizeof(struct ServerTight1FileListData)-sizeof(UBYTE));
            if(Result>0)
            {
                struct FileListNode *fln=Fsc_GetCurrentFileList(&Twin->FileList,TRUE);

                if(fln!=NULL)
                {
                    Result=Srv_Tight1_GetFileListData(Twin,&fln->fm,(struct ServerTight1FileListData *)Msg);
                    if(Result>0) Result=Fsc_ScheduleRemoteNext(&Twin->FileList,Twin);
                }
                else
                {
                    struct FileManager *fm=Win_BeginRemoteFileManagerRefresh(wft);

                    Fmg_FlushFileManagerResources(fm,FALSE);
                    Result=Srv_Tight1_GetFileListData(Twin,fm,(struct ServerTight1FileListData *)Msg);
                    if(Result>0)
                    {
                        Fmg_SortFileItemList(fm);
                        Win_EndRemoteFileManagerRefresh(wft);
                    }
                }
            }
            break;

        case TIGHT1_FILE_DOWNLOAD_DATA:
            Result=Gal_WaitRead(Twin,(void *)&Msg[1],sizeof(struct ServerTight1FileDownloadData)-sizeof(UBYTE));
            if(Result>0)
            {
                Result=Srv_Tight1_GetFileDownloadData(Twin,(struct ServerTight1FileDownloadData *)Msg);
            }
            break;

        case TIGHT1_FILE_UPLOAD_CANCEL:
        case TIGHT1_FILE_DOWNLOAD_FAILED:
            break;
    }

    return Result;
}


/*****
    Gestion des messages du protocole Tight 2
*****/

LONG Srv_ManageTight2Message(struct Twin *Twin, ULONG MsgCode)
{
    LONG Result=RESULT_SUCCESS;
    struct WindowFileTransfer *wft=&Twin->WindowFileTransfer;

    switch(MsgCode)
    {
        case TIGHT2_COMPRESSION_SUPPORT_REPLY:
            break;

        case TIGHT2_FILE_LIST_REPLY:
            {
                struct FileListNode *fln=Fsc_GetCurrentFileList(&Twin->FileList,TRUE);

                if(fln!=NULL)
                {
                    Result=Srv_Tight2_GetFileListData(Twin,&fln->fm);
                    if(Result>0) Result=Fsc_ScheduleRemoteNext(&Twin->FileList,Twin);
                }
                else
                {
                    /* Récupération de la liste des fichiers du répertoire ciblé */
                    struct FileManager *fm=Win_BeginRemoteFileManagerRefresh(wft);

                    Fmg_FlushFileManagerResources(fm,FALSE);
                    Result=Srv_Tight2_GetFileListData(Twin,fm);
                    if(Result>0)
                    {
                        Fmg_SortFileItemList(fm);
                        Win_EndRemoteFileManagerRefresh(wft);
                    }
                }
            }
            break;

        case TIGHT2_UPLOAD_START_REPLY:
            /* Ouverture du fichier à envoyer */
            Result=Srv_Tight2_OpenFileUpload(Twin,Twin->LocalFullPathFileName);
            if(Result>0) Result=Srv_Tight2_SetFileUploadData(Twin);
            break;

        case TIGHT2_UPLOAD_DATA_REPLY:
            /* Envoi d'un paquet de données */
            Result=Srv_Tight2_SetFileUploadData(Twin);
            break;

        case TIGHT2_UPLOAD_END_REPLY:
            /* Fin d'envoi des données */
            Result=Srv_Tight2_CloseFileUpload(Twin);
            if(Result>0) Result=Clt_SetFileListRequest(Twin,wft->RemoteFileManager.CurrentPath,1);
            break;

        case TIGHT2_DOWNLOAD_START_REPLY:
            /* Ouverture du téléchargement */
            //LONG Fsc_SchedulerDownloadStart(struct FileList *fl)

            Result=Srv_Tight2_OpenFileDownload(Twin,Twin->LocalFullPathFileName);
            if(Result>0) Result=Tight2_SetFileDownloadDataRequest(Twin,sizeof(Twin->Tmp));
            break;

        case TIGHT2_DOWNLOAD_DATA_REPLY:
            /* Réception d'un paquet de données */
            Result=Srv_Tight2_GetFileDownloadData(Twin);
            break;

        case TIGHT2_DOWNLOAD_END_REPLY:
            /* Fermeture du téléchargement */
            Result=Srv_Tight2_CloseFileDownload(Twin);
            if(Result>0) Win_RefreshFileList(wft);
            break;

        case TIGHT2_LAST_REQUEST_FAILED_REPLY:
            Result=Util_WaitReadStringUTF8(Twin,Twin->Tmp,sizeof(Twin->Tmp)-sizeof(char));
            if(Result>0) Sys_Printf("Last error FT: %s\n",Twin->Tmp);
            Fmg_ParentCurrentPath(&wft->RemoteFileManager,TRUE);
            break;

        case TIGHT2_MD5_REPLY:
        case TIGHT2_MKDIR_REPLY:
            break;

        case TIGHT2_REMOVE_REPLY:
            Result=Clt_SetFileListRequest(Twin,wft->RemoteFileManager.CurrentPath,1);
            break;

        case TIGHT2_RENAME_REPLY:
        case TIGHT2_DIRSIZE_REPLY:
            break;
    }

    return Result;
}


/*****
    Gestion de l'affichage
*****/

LONG Srv_GetFrameBufferUpdate(struct Twin *Twin, struct ServerFrameBufferUpdate *Msg)
{
    LONG Result=RESULT_SUCCESS;
    struct ServerFrameBufferRect FBRect;
    ULONG i;

    for(i=0; Result>0 && i<(ULONG)Msg->CountOfRect; i++)
    {
        Result=Gal_WaitRead(Twin,(void *)&FBRect,sizeof(FBRect));
        if(Result>0)
        {
            LONG x=(LONG)FBRect.Rect.x;
            LONG y=(LONG)FBRect.Rect.y;
            LONG w=(LONG)FBRect.Rect.w;
            LONG h=(LONG)FBRect.Rect.h;

            switch(FBRect.EncodingType)
            {
                case RFB_ENCODING_RAW:
                    Result=DecoderRaw(Twin,x,y,w,h);
                    break;

                case RFB_ENCODING_COPYRECT:
                    Result=DecoderCopyRect(Twin,x,y,w,h);
                    break;

                case RFB_ENCODING_RRE:
                    Result=DecoderRRE(Twin,x,y,w,h);
                    break;

                case RFB_ENCODING_CORRE:
                    Result=DecoderCoRRE(Twin,x,y,w,h);
                    break;

                case RFB_ENCODING_HEXTILE:
                case RFB_ENCODING_ZLIBHEX:
                    Result=DecoderHextile(Twin,x,y,w,h);
                    break;

                case RFB_ENCODING_ZLIBRAW:
                    Result=DecoderZLibRaw(Twin,x,y,w,h);
                    break;

                case RFB_ENCODING_TIGHT:
                    Result=DecoderTight(Twin,x,y,w,h);
                    break;

                case RFB_ENCODING_ZRLE:
                    Result=DecoderZRle(Twin,x,y,w,h);
                    break;

                case RFB_ENCODING_XCURSOR:
                    Result=DecoderXCursor(Twin,x,y,w,h);
                    break;

                case RFB_ENCODING_RICHCURSOR:
                    Result=DecoderRichCursor(Twin,x,y,w,h);
                    break;

                case RFB_ENCODING_POINTERPOS:
                    if(Twin->CurrentGlobalPrefs.DisplayOption!=DISPLAY_OPTION_NODISPLAY)
                        Disp_MoveCursor(&Twin->Display,x+1,y+1);
                    break;

                case RFB_ENCODING_LASTRECT:
                    i=(ULONG)Msg->CountOfRect; /*sortie*/
                    break;

                case RFB_ENCODING_NEWFBSIZE:
                    Result=Srv_ResizeDisplay(Twin,w,h);
                    Twin->IsRefreshIncremental=FALSE;
                    i=(ULONG)Msg->CountOfRect; /*sortie*/
                    break;
            }
        }
    }

    Util_FlushDecodeBuffers(Twin,FALSE);

    if(Result>0)
    {
        Disp_RefreshCursor(&Twin->Display);
        Twin->WaitRefreshPriority=0;
    }

    return Result;
}


/*****
    Récupération de la ColorMap calculée par le serveur
*****/

LONG Srv_GetColourMapEntries(struct Twin *Twin, struct ServerSetColourMapEntries *Msg)
{
    LONG Result=RESULT_SUCCESS;
    struct Display *disp=&Twin->Display;
    struct FrameBuffer *fb=&disp->FrameBuffer;
    struct ServerSetColourMapRGB RGB;
    ULONG i;

    fb->ColorCount=(LONG)Msg->CountOfColors;
    for(i=0;i<Msg->CountOfColors && Result>0;i++)
    {
        ULONG Offset=(ULONG)Msg->FirstColor+i;

        Result=Gal_WaitRead(Twin,(void *)&RGB,sizeof(RGB));
        if(Result>0 && Offset<256)
        {
            fb->ColorMapRGB32[Offset]=((RGB.Red&0xff00)<<8)+(RGB.Green&0xff00)+((RGB.Blue&0xff00)>>8);
        }
    }

    if(Result>0)
    {
        if(!Disp_RethinkViewType(disp,TRUE,TRUE,TRUE)) Result=RESULT_NOT_ENOUGH_MEMORY;
    }

    return Result;
}


/*****
    Récupération du clipboard provenant du serveur
*****/

LONG Srv_GetServerCutText(struct Twin *Twin, struct ServerCutText *Msg)
{
    LONG Result=RESULT_SUCCESS;

    if(Msg->Length>0)
    {
        char *Ptr=(char *)Sys_AllocMem(Msg->Length+sizeof(char));

        if(Ptr!=NULL)
        {
            Result=Gal_WaitRead(Twin,(void *)Ptr,(LONG)Msg->Length);

            if(Result>0 && !Twin->CurrentGlobalPrefs.IsNoClipboard)
            {
                Ptr[Result]=0;
                Sys_WriteClipboard(Ptr,(ULONG)Msg->Length);
            }

            Sys_FreeMem(Ptr);
        }
    }

    return Result;
}


/*****
    Changement de taille du framebuffer
*****/

LONG Srv_ResizeDisplay(struct Twin *Twin, LONG Width, LONG Height)
{
    LONG Result=RESULT_NOT_ENOUGH_MEMORY;
    struct FrameBuffer *fb=&Twin->Display.FrameBuffer;

    Disp_ClearCursor(&Twin->Display);
    Twin->Connect.ServerWidth=Width;
    Twin->Connect.ServerHeight=Height;
    if(Frb_Prepare(fb,fb->PixelFormat,Width,Height))
    {
        Twin->IsDisplayChanged=TRUE;
        Result=Gal_ChangeDisplay(Twin);
        Disp_SetViewTitle(&Twin->Display,Gal_MakeDisplayTitle(Twin));
    }

    return Result;
}


/***************************************************************/
/***************************************************************/

/*****
    Récupération de la liste des fichiers
*****/

LONG Srv_Tight2_GetFileListData(struct Twin *Twin, struct FileManager *fm)
{
    UBYTE Buffer[3+1+4+4]; /* 3 pad (alignement) + 1 compression + 4 taille compressé + 4 taille décompressé */
    LONG Result=Gal_WaitRead(Twin,(void *)&Buffer[3],9*sizeof(UBYTE));

    if(Result>0)
    {
        UBYTE CompressionLevel=Buffer[3];
        ULONG *Data=(ULONG *)&Buffer[4];
        ULONG SizeData=Data[0];
        ULONG SizeDst=SizeData;

        if(CompressionLevel) SizeDst=Data[1];

        Result=RESULT_NOT_ENOUGH_MEMORY;
        if(Util_PrepareDecodeBuffers(Twin,CompressionLevel?SizeData:0,SizeDst))
        {
            /* Chargement et décompression si nécessaire */
            if((Result=Gal_WaitRead(Twin,(void *)(CompressionLevel?Twin->SrcBufferPtr:Twin->DstBufferPtr),SizeData))>0)
            {
                if(CompressionLevel)
                {
                    if(Util_ZLibUnCompress(&Twin->Stream,Twin->SrcBufferPtr,SizeData,Twin->DstBufferPtr,SizeDst)!=Z_OK) Result=RESULT_CORRUPTED_DATA;
                }
            }

            if(Result>0)
            {
                UBYTE *Dst=(UBYTE *)Twin->DstBufferPtr,*DstEnd=&((UBYTE *)Twin->DstBufferPtr)[SizeDst];
                ULONG Count=Util_ReadBuffer32(&Dst,DstEnd),i;
                ULONG Size64[2],ModTime64[2];

                for(i=0; i<Count; i++)
                {
                    ULONG Flag,Type;

                    Size64[0]=Util_ReadBuffer32(&Dst,DstEnd);
                    Size64[1]=Util_ReadBuffer32(&Dst,DstEnd);
                    ModTime64[0]=Util_ReadBuffer32(&Dst,DstEnd);
                    ModTime64[1]=Util_ReadBuffer32(&Dst,DstEnd);
                    Flag=Util_ReadBuffer16(&Dst,DstEnd);
                    Util_ReadBufferStringUTF8(&Dst,DstEnd,Twin->Tmp,sizeof(Twin->Tmp));

                    /* Conversion du timestamp en version Amiga */
                    ModTime64[1]=Util_Time64ToAmigaTime(ModTime64);

                    Type=Flag&1?FMNG_FLAG_DIR:FMNG_FLAG_FILE;
                    if(!Fmg_AddItemToFileList(fm,Twin->Tmp,Size64,ModTime64[1],Type)) Result=RESULT_NOT_ENOUGH_MEMORY;
                }
            }

            Util_FlushDecodeBuffers(Twin,FALSE);
        }
    }

    return Result;
}


/*****
    Permet d'ouvrir le fichier à télécharger
*****/

LONG Srv_Tight2_OpenFileDownload(struct Twin *Twin, const char *FullPathFileName)
{
    LONG Result=RESULT_SUCCESS;

    /* Gérer l'ouverture du fichier */
    /* ... */
    //Sys_Printf("Srv_Tight2_OpenFileDownload:%s\n",FullPathFileName);
    Twin->LocalFileHandle=(void *)fopen(FullPathFileName,"wb");
    Twin->FTCurrentSize64[0]=0;
    Twin->FTCurrentSize64[1]=0;

    return Result;
}


/*****
    Permet de fermer le fichier téléchargé
*****/

LONG Srv_Tight2_CloseFileDownload(struct Twin *Twin)
{
    UBYTE Buffer[3+1+8]; /* 3 pad + 1 flag + 8 quad */
    LONG Result=Gal_WaitRead(Twin,(void *)&Buffer[3],9*sizeof(UBYTE));

    if(Result>0)
    {
        UBYTE Flags=Buffer[3];
        ULONG ModTime64[2];
        struct DateStamp ds;

        ModTime64[0]=*(ULONG *)&Buffer[4];
        ModTime64[1]=*(ULONG *)&Buffer[8];
        ModTime64[1]=Util_Time64ToAmigaTime(ModTime64);
        ds.ds_Days=ModTime64[1]/(24*60*60);
        ds.ds_Minute=(ModTime64[1]-ds.ds_Days*24*60*60)/60;
        ds.ds_Tick=((ModTime64[1]-ds.ds_Days*24*60*60)-ds.ds_Minute*60)*TICKS_PER_SECOND;

        /* Gérer la fermeture du fichier */
        /* ... */
        //Sys_Printf("Srv_Tight2_CloseFileDownload: %02lx %08lx%08lx\n",(long)Flags,ModTime64[0],ModTime64[1]);
        if(Twin->LocalFileHandle!=NULL)
        {
            fclose((FILE *)Twin->LocalFileHandle);
            SetFileDate((STRPTR)Twin->LocalFullPathFileName,&ds);
        }
        Twin->LocalFileHandle=NULL;

        //Result=RESULT_SUCCESS;
        Result=Fsc_ScheduleRemoteNext(&Twin->FileList,Twin);
    }

    return Result;
}


/*****
    Réception d'un paquet de données provenant d'un fichier en téléchargement
*****/

LONG Srv_Tight2_GetFileDownloadData(struct Twin *Twin)
{
    LONG Result=RESULT_SUCCESS;
    UBYTE Buffer[3+1+4+4]; /* 3 pad (alignement) + 5 datas */

    Result=Gal_WaitRead(Twin,(void *)&Buffer[3],9*sizeof(UBYTE));
    if(Result>0)
    {
        /*UBYTE CompressionLevel=Buffer[3];*/
        ULONG *Data=(ULONG *)&Buffer[4];

        Result=RESULT_NOT_ENOUGH_MEMORY;
        if(Util_PrepareDecodeBuffers(Twin,0,Data[0]))
        {
            Result=Gal_WaitRead(Twin,(void *)Twin->DstBufferPtr,Data[0]);
            if(Result>0)
            {
                ULONG i,N[2],R[2],Val64[2];

                /* Gérer ici les données du fichier */
                /* ... */
                if(Twin->LocalFileHandle!=NULL) fwrite(Twin->DstBufferPtr,Data[0],sizeof(UBYTE),(FILE *)Twin->LocalFileHandle);
                Val64[0]=0;
                Val64[1]=Data[0];
                Util_Add64(Val64,Twin->FTCurrentSize64);

                N[0]=Twin->FTCurrentSize64[0];
                N[1]=Twin->FTCurrentSize64[1];
                for(i=0;i<16;i++) Util_ShiftL64(N,FALSE);
                Util_DivU64(N,Twin->FTFinalSize64,R);
                Win_SetGaugeProgress(&Twin->WindowFileTransfer,(N[1]*1000)>>16);

                Result=Tight2_SetFileDownloadDataRequest(Twin,sizeof(Twin->Tmp));
            }

            Util_FlushDecodeBuffers(Twin,FALSE);
        }
    }

    return Result;
}


/*****
    Permet d'ouvrir le fichier à envoyer
*****/

LONG Srv_Tight2_OpenFileUpload(struct Twin *Twin, const char *FullPathFileName)
{
    LONG Result=RESULT_SUCCESS;

    /* Gérer l'ouverture du fichier */
    /* ... */
    //Sys_Printf("Srv_Tight2_OpenFileUpload:%s\n",FullPathFileName);

    Twin->FTCurrentSize64[0]=0;
    Twin->FTCurrentSize64[1]=0;
    Twin->LocalFileHandle=(void *)fopen(FullPathFileName,"rb");

    return Result;
}


/*****
    Permet de fermer le fichier envoyé
*****/

LONG Srv_Tight2_CloseFileUpload(struct Twin *Twin)
{
    LONG Result=RESULT_SUCCESS;

    //Sys_Printf("Srv_Tight2_CloseFileUpload\n");
    if(Twin->LocalFileHandle!=NULL)
    {
        fclose(Twin->LocalFileHandle);
        Twin->LocalFileHandle=NULL;
    }

    return Result;
}


/*****
    Envoi d'un paquet de données provenant du fichier à envoyer
*****/

LONG Srv_Tight2_SetFileUploadData(struct Twin *Twin)
{
    LONG Result=RESULT_SUCCESS;

    if(Twin->LocalFileHandle!=NULL)
    {
        LONG Size=fread(Twin->Tmp,sizeof(UBYTE),sizeof(Twin->Tmp),Twin->LocalFileHandle);

        if(Size<=0) Result=Clt_SetFileUploadEndRequest(Twin,0,Twin->FTModTime);
        else
        {
            ULONG N[2],R[2],Size64[2],i;

            Size64[0]=0;
            Size64[1]=Size;
            Util_Add64(Size64,Twin->FTCurrentSize64);

            N[0]=Twin->FTCurrentSize64[0];
            N[1]=Twin->FTCurrentSize64[1];
            for(i=0;i<16;i++) Util_ShiftL64(N,FALSE);
            Util_DivU64(N,Twin->FTFinalSize64,R);
            Win_SetGaugeProgress(&Twin->WindowFileTransfer,(N[1]*1000)>>16);

            Result=Tight2_SetFileUploadDataRequest(Twin,Twin->Tmp,Size);
        }
    }

    return Result;
}


/***************************************************************/
/***************************************************************/

/*****
    Récupération de la liste des fichiers (Protocole Tight 1.x)
*****/

LONG Srv_Tight1_GetFileListData(struct Twin *Twin, struct FileManager *fm, struct ServerTight1FileListData *Msg)
{
    LONG Result=RESULT_SUCCESS;

    if(Msg->NumFiles>0 && Msg->DataSize>0)
    {
        LONG DataLen=Msg->NumFiles*sizeof(struct FileListSizeData);

        Result=RESULT_NOT_ENOUGH_MEMORY;
        if(Util_PrepareDecodeBuffers(Twin,Msg->DataSize+1,DataLen))
        {
            char *NamesPtr=(char *)Twin->SrcBufferPtr;
            struct FileListSizeData *InfoPtr=(struct FileListSizeData *)Twin->DstBufferPtr;

            /* Récupération des propriétés des fichiers (taille et type) */
            if((Result=Gal_WaitRead(Twin,(void *)InfoPtr,DataLen))>0)
            {
                /* Récupération des noms de fichier */
                if((Result=Gal_WaitRead(Twin,(void *)NamesPtr,(LONG)Msg->DataSize))>0)
                {
                    LONG Pos=0,i=0;

                    NamesPtr[Msg->DataSize]=0; /* sécurité */
                    while(Pos<(LONG)Msg->DataSize && Result>0)
                    {
                        LONG Type=(InfoPtr[i].Data==0)?FMNG_FLAG_DIR:FMNG_FLAG_FILE;
                        ULONG Size64[2];

                        Size64[0]=0;
                        Size64[1]=InfoPtr[i].Size;
                        if(Fmg_AddItemToFileList(fm,&NamesPtr[Pos],Size64,0,Type))
                        {
                            Pos+=Sys_StrLen(&NamesPtr[Pos])+sizeof(char);
                            i++;
                        } else Result=RESULT_NOT_ENOUGH_MEMORY;
                    }
                }
            }

            Util_FlushDecodeBuffers(Twin,FALSE);
        }
    }

    return Result;
}


/*****
    Récéption d'un paquet de données provenant d'un fichier en téléchargement
*****/

LONG Srv_Tight1_GetFileDownloadData(struct Twin *Twin, struct ServerTight1FileDownloadData *Msg)
{
    LONG Result=RESULT_SUCCESS;

    if(Msg->RealSize!=0 & Msg->CompressedSize!=0)
    {
        Result=RESULT_NOT_ENOUGH_MEMORY;
        if(Util_PrepareDecodeBuffers(Twin,0,Msg->CompressedSize))
        {
            ULONG i,N[2],R[2];

            Result=Gal_WaitRead(Twin,(void *)Twin->DstBufferPtr,Msg->CompressedSize);
            if(Result>0)
            {
                ULONG Size64[2];
                /* Gérer ici les données du fichier */
                /* ... */
                if(Twin->LocalFileHandle==NULL) Srv_Tight2_OpenFileDownload(Twin,Twin->LocalFullPathFileName);
                if(Twin->LocalFileHandle!=NULL) fwrite(Twin->DstBufferPtr,Msg->CompressedSize,sizeof(UBYTE),(FILE *)Twin->LocalFileHandle);

                Size64[0]=0;
                Size64[1]=Msg->CompressedSize;
                Util_Add64(Size64,Twin->FTCurrentSize64);

                N[0]=Twin->FTCurrentSize64[0];
                N[1]=Twin->FTCurrentSize64[1];
                for(i=0;i<16;i++) Util_ShiftL64(N,FALSE);
                Util_DivU64(N,Twin->FTFinalSize64,R);
                Win_SetGaugeProgress(&Twin->WindowFileTransfer,(N[1]*1000)>>16);
            }

            Util_FlushDecodeBuffers(Twin,FALSE);
        }
    }
    else
    {
        ULONG ModTime;

        if(Twin->LocalFileHandle!=NULL) fclose((FILE *)Twin->LocalFileHandle);
        Twin->LocalFileHandle=NULL;

        Result=Gal_WaitRead(Twin,(void *)&ModTime,sizeof(ModTime));
        if(Result>0)
        {
            struct DateStamp ds;

            ModTime=((ModTime&0xff)<<24)+((ModTime&0xff00)<<8)+((ModTime&0xff0000)>>8)+(ModTime>>24);
            ModTime-=(8*365+2)*24*60*60;
            ds.ds_Days=ModTime/(24*60*60);
            ds.ds_Minute=(ModTime-ds.ds_Days*24*60*60)/60;
            ds.ds_Tick=((ModTime-ds.ds_Days*24*60*60)-ds.ds_Minute*60)*TICKS_PER_SECOND;
            SetFileDate((STRPTR)Twin->LocalFullPathFileName,&ds);

            Result=Fsc_ScheduleRemoteNext(&Twin->FileList,Twin);
        }

    }

    return Result;
}
