#include <exec/exec.h>
#include <proto/exec.h>
#include <cybergraphx/cgxvideo.h>
#include <proto/cgxvideo.h>
#include <proto/dos.h>
#include <proto/utility.h>

struct Library *CGXVideoBase=NULL;
struct DosLibrary *DOSBase=NULL;
struct Library *UtilityBase=NULL;


BYTE Version[] = {"\0$VER: PatchOverlay 1.0 by Seg of Dimension 08.12.2005"};

ULONG __asm (*OldFunc)(
        REGISTER __a0 struct VLayerHandle *,
        REGISTER __a1 struct Window *,
        REGISTER __a2 struct TagItem *,
        REGISTER __a6 struct Library *)=NULL;

ULONG __asm __saveds NewFunc(
        REGISTER __a0 struct VLayerHandle *h,
        REGISTER __a1 struct Window *win,
        REGISTER __a2 struct TagItem *tags,
        REGISTER __a6 struct Library *lib);


ULONG __saveds main(void)
{
    DOSBase=(struct DosLibrary *)OpenLibrary("dos.library",39L);
    if(DOSBase!=NULL)
    {
        UtilityBase=OpenLibrary("utility.library",39L);
        if(UtilityBase!=NULL)
        {
            Printf("Open cgxvideo.library...\n");

            CGXVideoBase=OpenLibrary("cgxvideo.library",0L);
            if(CGXVideoBase!=NULL)
            {
                BOOL IsReseted=FALSE;

                Printf("Patch AttachVLayerTagList() function\n");

                Forbid();
                OldFunc=SetFunction(CGXVideoBase,-42,(APTR)NewFunc);
                CacheClearU();
                Permit();

                Wait(SIGBREAKF_CTRL_C);

                while(!IsReseted)
                {
                    APTR CurFunc;

                    Forbid();
                    CurFunc=SetFunction(CGXVideoBase,-42,(APTR)OldFunc);
                    if(CurFunc==(APTR)NewFunc) IsReseted=TRUE;
                    else SetFunction(CGXVideoBase,-42,CurFunc);
                    CacheClearU();
                    Permit();

                    if(!IsReseted)
                    {
                        Printf("Try to remove patch...\n");
                        Delay(5*50);
                    }
                }

                Printf("Patch removed!\n");

                CloseLibrary(CGXVideoBase);
            }

            CloseLibrary(UtilityBase);
        }

        CloseLibrary((struct Library *)DOSBase);
    }

    return 0;
}


ULONG __asm __saveds NewFunc(
        REGISTER __a0 struct VLayerHandle *h,
        REGISTER __a1 struct Window *win,
        REGISTER __a2 struct TagItem *tags,
        REGISTER __a6 struct Library *lib)
{
    ULONG Result;
    struct TagItem *tstate;
    struct TagItem *tag;
    BOOL IsOk=FALSE,IsRightIndent=FALSE,IsBottomIndent=FALSE;
    LONG LeftIndent=0,TopIndent=0;
    LONG TagCount=1; /* On compte le TAG_DONE! */

    /* Calcule le nombre de tags dans la taglist */
    tstate=tags;
    while(tag=NextTagItem(&tstate))
    {
        switch(tag->ti_Tag)
        {
            case VOA_LeftIndent:
                LeftIndent=tag->ti_Data;
                break;

            case VOA_TopIndent:
                TopIndent=tag->ti_Data;
                break;

            case VOA_RightIndent:
                IsRightIndent=TRUE;
                break;

            case VOA_BottomIndent:
                IsBottomIndent=TRUE;
                break;
        }

        TagCount++;
    }

    /* Tests pour patcher si necessaire seulement... */
    if(LeftIndent>0 || TopIndent>0)
    {
        struct TagItem *NewTags;

        /* On rajoute 1 ou 2 tags si ceux-ci sont a la fois necessaires et non presents
           dans la taglist originale */
        if(LeftIndent>0 && !IsRightIndent) TagCount++;
        if(TopIndent>0 && !IsBottomIndent) TagCount++;

        /* Allocation de la nouvelle taglist */
        NewTags=AllocVec(TagCount*sizeof(struct TagItem),MEMF_PUBLIC);
        if(NewTags!=NULL)
        {
            struct TagItem *Ptr=NewTags;

            IsOk=TRUE;

            /* On recopie et modifie la nouvelle taglist */
            tstate=tags;
            while(tag=NextTagItem(&tstate))
            {
                ULONG Value=tag->ti_Data;

                switch(tag->ti_Tag)
                {
                    case VOA_RightIndent:
                        Value+=LeftIndent;
                        break;

                    case VOA_BottomIndent:
                        Value+=TopIndent;
                        break;
                }

                Ptr->ti_Tag=tag->ti_Tag;
                Ptr->ti_Data=Value;
                Ptr++;
            }

            /* On rajoute le tag VOA_RightIndent s'il n'etait pas present */
            if(LeftIndent>0 && !IsRightIndent)
            {
                Ptr->ti_Tag=VOA_RightIndent;
                Ptr->ti_Data=LeftIndent;
                Ptr++;
            }

            /* On rajoute le tag VOA_BottomIndent s'il n'etait pas present */
            if(TopIndent>0 && !IsBottomIndent)
            {
                Ptr->ti_Tag=VOA_BottomIndent;
                Ptr->ti_Data=TopIndent;
                Ptr++;
            }

            Ptr->ti_Tag=TAG_DONE;
            Ptr->ti_Data=0L;

            Result=OldFunc(h,win,NewTags,lib);

            FreeVec(NewTags);
        }
    }


    if(!IsOk) Result=OldFunc(h,win,tags,lib);

    return Result;
}

