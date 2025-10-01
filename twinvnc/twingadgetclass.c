#include "system.h"
#include "twingadgetclass.h"
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <clib/alib_protos.h>
#include <graphics/gfxmacros.h>

/*
    01-07-2017 (Seg)    Améliorations: ajout du mode GA_Disabled
    25-05-2004 (Seg)    Gestion d'un gadget BOOPSI
    16-12-2004 (Seg)    Refonte de la classe gadget
*/


struct TwinImageData
{
    APTR VisualInfo;
    LONG Width;
    LONG Height;
    ULONG *ColorMapRGB;
    UBYTE *NewImage;
    UBYTE *OrgImage;
    LONG ImageWidth;
    LONG ImageHeight;
    LONG BrushPosX;
    LONG BrushPosY;
    LONG BrushWidth;
    LONG BrushHeight;
};


struct TwinGadgetData
{
    Object *TwinObjectImage;
};


/***** Prototypes */
Class *Tgc_MakeTwinGadgetClass(void);
void Tgc_FreeTwinGadgetClass(Class *);

M_HOOK_PROTO(TwinGadgetDispatcher, Class *, Object *, Msg);
ULONG TwinGadget_NEW(Class *,Object *,struct opSet *);
ULONG TwinGadget_DISPOSE(Class *, Object *, Msg);
ULONG TwinGadget_RENDER(Class *, Object *, struct gpRender *);

Class *MakeTwinImageClass(void);
M_HOOK_PROTO(TwinImageDispatcher, Class *, Object *, Msg);
ULONG TwinImage_NEW(Class *, Object *, struct opSet *);
ULONG TwinImage_DISPOSE(Class *, Object *, Msg);
ULONG TwinImage_DRAW(Class *, Object *, struct impDraw *);
void DrawChunkyImage(struct impDraw *, struct TwinImageData *);


/*******************************************************************/
/****** GESTION DE L'OBJET GADGET                             ******/
/*******************************************************************/

/*****
    Création de la classe TwinGadget
*****/

Class *Tgc_MakeTwinGadgetClass(void)
{
    Class *IClass=MakeClass(NULL,BUTTONGCLASS,NULL,sizeof(struct TwinGadgetData),0);

    if(IClass!=NULL)
    {
        IClass->cl_Dispatcher.h_SubEntry=NULL;
        IClass->cl_Dispatcher.h_Entry=(HOOKFUNC)&TwinGadgetDispatcher;
        IClass->cl_Dispatcher.h_Data=NULL;

        IClass->cl_UserData=(ULONG)MakeTwinImageClass();
        if(!IClass->cl_UserData)
        {
            Tgc_FreeTwinGadgetClass(IClass);
            IClass=NULL;
        }
    }

    return IClass;
}


/*****
    Destruction d'une class TwinGadget
*****/

void Tgc_FreeTwinGadgetClass(Class *IClass)
{
    Class *ImageClass=(Class *)IClass->cl_UserData;

    if(FreeClass(IClass))
    {
        if(ImageClass!=NULL) FreeClass(ImageClass);
    }
}


/*****
  Dispatcher: les événements du gadgets sont réceptionnés ici
*****/

M_HOOK_FUNC(TwinGadgetDispatcher, Class *IClass, Object *IObject, Msg IMsg)
{
    ULONG RetVal;

    switch(IMsg->MethodID)
    {
        case OM_NEW:
            RetVal=TwinGadget_NEW(IClass,IObject,(struct opSet *)IMsg);
            break;

        case OM_DISPOSE:
            RetVal=TwinGadget_DISPOSE(IClass,IObject,IMsg);
            break;

        case GM_RENDER:
            RetVal=TwinGadget_RENDER(IClass,IObject,(struct gpRender *)IMsg);
            break;

        default:
            RetVal=DoSuperMethodA(IClass,IObject,IMsg);
            break;
    }

    return RetVal;
}


/*****
    Création d'un TwinGadget
*****/

ULONG TwinGadget_NEW(Class *IClass, Object *IObject, struct opSet *opSet)
{
    /* Dans la méthode NEW, IObject ne correspond pas à l'objet. Il
       correspond à un pointeur sur la classe dont l'objet est en train
       d'être instancié, donc IClass=IObject.
       Le premier DoSuperMethod() ci-dessous retourne un pointeur
       sur le nouvel objet créé.
    */
    ULONG NewIObject=DoSuperMethodA(IClass,IObject,(Msg)opSet);

    if(NewIObject)
    {
        struct TwinGadgetData *gd=(struct TwinGadgetData *)INST_DATA(IClass,NewIObject);

        /* Création de l'image associée à ce gadget */
        gd->TwinObjectImage=NewObject((Class *)IClass->cl_UserData,NULL,
                IA_Width,(LONG)GetTagData(GA_Width,(ULONG)NULL,opSet->ops_AttrList),
                IA_Height,(LONG)GetTagData(GA_Height,(ULONG)NULL,opSet->ops_AttrList),
                GT_VisualInfo,(LONG)GetTagData(GT_VisualInfo,(ULONG)NULL,opSet->ops_AttrList),
                TIMG_ColorMapID,(LONG)GetTagData(TIMG_ColorMapID,(ULONG)NULL,opSet->ops_AttrList),
                TIMG_ColorMapRGB,(LONG)GetTagData(TIMG_ColorMapRGB,(ULONG)NULL,opSet->ops_AttrList),
                TIMG_Image,(LONG)GetTagData(TIMG_Image,(ULONG)NULL,opSet->ops_AttrList),
                TIMG_PosX,(LONG)GetTagData(TIMG_PosX,0UL,opSet->ops_AttrList),
                TIMG_PosY,(LONG)GetTagData(TIMG_PosY,0UL,opSet->ops_AttrList),
                TIMG_Width,(LONG)GetTagData(TIMG_Width,0UL,opSet->ops_AttrList),
                TIMG_Height,(LONG)GetTagData(TIMG_Height,0UL,opSet->ops_AttrList),
                GA_Previous,NULL,
                TAG_DONE);

        if(gd->TwinObjectImage!=NULL)
        {
            /* On affecte légalement l'objet image dans l'objet bouton */
            SetAttrs((APTR)NewIObject,GA_Image,gd->TwinObjectImage,TAG_DONE);
        }
        else
        {
            CoerceMethod(IClass,(Object *)NewIObject,OM_DISPOSE);
            NewIObject=0;
        }
    }

    return NewIObject;
}


/*****
    Fermeture du TwinGadget
*****/

ULONG TwinGadget_DISPOSE(Class *IClass, Object *IObject, Msg IMsg)
{
    struct TwinGadgetData *gd=(struct TwinGadgetData *)INST_DATA(IClass,IObject);

    /* Destruction des ressources de l'objet */
    if(gd->TwinObjectImage!=NULL) DisposeObject((APTR)gd->TwinObjectImage);

    return DoSuperMethodA(IClass,IObject,IMsg);
}


/*****
    Gestion du rendu
*****/

ULONG TwinGadget_RENDER(Class *IClass, Object *IObject, struct gpRender *gpr)
{
    struct Gadget *g=(struct Gadget *)IObject;
    ULONG RetVal=DoSuperMethodA(IClass,IObject,(Msg)gpr);

    /* Note: Dans le cas d'un bouton image, la super classe ne sait pas
       griser l'image en mode GA_Disabled=TRUE. Donc,on gère ce cas dans
       le RENDER de la classe bouton
    */
    if((g->Flags&GFLG_DISABLED)!=0)
    {
        DrawImageState(gpr->gpr_RPort,(struct Image *)g->GadgetRender,g->LeftEdge,g->TopEdge,IDS_DISABLED,gpr->gpr_GInfo->gi_DrInfo);
    }

    return RetVal;
}


/*******************************************************************/
/****** GESTION DE L'OBJET IMAGE                              ******/
/*******************************************************************/

/*****
    Création de la classe TwinImage
*****/

Class *MakeTwinImageClass(void)
{
    Class *IClass=MakeClass(NULL,IMAGECLASS,NULL,sizeof(struct TwinImageData),0);

    if(IClass!=NULL)
    {
        IClass->cl_Dispatcher.h_SubEntry=NULL;
        IClass->cl_Dispatcher.h_Entry=(HOOKFUNC)&TwinImageDispatcher;
        IClass->cl_Dispatcher.h_Data=NULL;
    }

    return IClass;
}


/*****
    Dispatcher: les événements de l'objet image sont réceptionnés ici
*****/

M_HOOK_FUNC(TwinImageDispatcher, Class *IClass, Object *IObject, Msg IMsg)
{
    ULONG RetVal;

    switch(IMsg->MethodID)
    {
        case OM_NEW:
            RetVal=TwinImage_NEW(IClass,IObject,(struct opSet *)IMsg);
            break;

        case OM_DISPOSE:
            RetVal=TwinImage_DISPOSE(IClass,IObject,IMsg);
            break;

        case IM_DRAW:
        case IM_DRAWFRAME:
            RetVal=TwinImage_DRAW(IClass,IObject,(struct impDraw *)IMsg);
            break;

        default:
            RetVal=DoSuperMethodA(IClass,IObject,IMsg);
            break;
    }

    return RetVal;
}


/*****
    Création du TwinImage
*****/

ULONG TwinImage_NEW(Class *IClass, Object *IObject, struct opSet *opSet)
{
    /* Dans la méthode NEW, IObject ne correspond pas à l'objet. Il
       correspond à un pointeur sur la classe dont l'objet est en train
       d'être instancié, donc IClass=IObject.
       Le premier DoSuperMethod() ci-dessous retourne un pointeur
       sur le nouvel objet créé.
    */
    ULONG NewIObject=DoSuperMethodA(IClass,IObject,(Msg)opSet);

    if(NewIObject)
    {
        BOOL IsSuccess=TRUE;
        struct TwinImageData *imgd=(struct TwinImageData *)INST_DATA(IClass,NewIObject);
        LONG *ColorMapID=(LONG *)GetTagData(TIMG_ColorMapID,(ULONG)NULL,opSet->ops_AttrList);
        UBYTE *ChkImg=(UBYTE *)GetTagData(TIMG_Image,(ULONG)NULL,opSet->ops_AttrList);

        /* Initialisation de la structure */
        imgd->VisualInfo=(APTR)GetTagData(GT_VisualInfo,(ULONG)NULL,opSet->ops_AttrList);
        imgd->Width=(LONG)GetTagData(IA_Width,(ULONG)0,opSet->ops_AttrList);
        imgd->Height=(LONG)GetTagData(IA_Height,(ULONG)0,opSet->ops_AttrList);
        imgd->ColorMapRGB=(ULONG *)GetTagData(TIMG_ColorMapRGB,(ULONG)NULL,opSet->ops_AttrList);
        imgd->OrgImage=NULL;
        imgd->NewImage=NULL;
        imgd->ImageWidth=0;
        imgd->ImageHeight=0;

        /* Si on a une image, on prend les paramètres */
        if(ChkImg!=NULL)
        {
            imgd->ImageWidth=(LONG)((UWORD *)ChkImg)[2];
            imgd->ImageHeight=(LONG)((UWORD *)ChkImg)[3];
            imgd->OrgImage=&ChkImg[10+4*((UWORD *)ChkImg)[4]];
        }

        imgd->BrushPosX=(LONG)GetTagData(TIMG_PosX,0UL,opSet->ops_AttrList);
        imgd->BrushPosY=(LONG)GetTagData(TIMG_PosY,0UL,opSet->ops_AttrList);
        imgd->BrushWidth=(LONG)GetTagData(TIMG_Width,0UL,opSet->ops_AttrList);
        imgd->BrushHeight=(LONG)GetTagData(TIMG_Height,0UL,opSet->ops_AttrList);
        if(imgd->BrushWidth==0) imgd->BrushWidth=imgd->ImageWidth;
        if(imgd->BrushHeight==0) imgd->BrushHeight=imgd->ImageHeight;

        /* Si on a une image et une table de couleur, on convertit alors l'image */
        if(imgd->OrgImage!=NULL && ColorMapID!=NULL)
        {
            LONG Size=imgd->ImageWidth*imgd->ImageHeight;

            if((imgd->NewImage=Sys_AllocMem(Size*sizeof(UBYTE))))
            {
                LONG i;

                for(i=0;i<Size;i++) imgd->NewImage[i]=(UBYTE)ColorMapID[imgd->OrgImage[i]];
            } else IsSuccess=FALSE;
        }

        if(!IsSuccess)
        {
            CoerceMethod(IClass,(Object *)NewIObject,OM_DISPOSE);
            NewIObject=0;
        }
    }

    return NewIObject;
}


/*****
    Fermeture du TwinImage
*****/

ULONG TwinImage_DISPOSE(Class *IClass, Object *IObject, Msg IMsg)
{
    struct TwinImageData *imgd=(struct TwinImageData *)INST_DATA(IClass,IObject);

    /* Libération des ressources de l'objet image */
    if(imgd->NewImage!=NULL) FreeVec((void *)imgd->NewImage);

    return DoSuperMethodA(IClass,IObject,IMsg);
}


/*****
    Méthode pour dessiner l'objet
*****/

ULONG TwinImage_DRAW(Class *IClass, Object *IObject, struct impDraw *impDraw)
{
    ULONG RetVal=TRUE; /* code de retour facultatif! */
    struct TwinImageData *imgd=INST_DATA(IClass,(Object *)IObject);
    struct RastPort *rp=impDraw->imp_RPort;
    LONG x=impDraw->imp_Offset.X,y=impDraw->imp_Offset.Y;

    switch(impDraw->imp_State)
    {
        case IDS_NORMAL:
            DrawChunkyImage(impDraw,imgd);
            if(imgd->VisualInfo!=NULL)
            {
                DrawBevelBox(rp,x,y,imgd->Width,imgd->Height,GT_VisualInfo,imgd->VisualInfo,TAG_DONE);
            }
            break;

        case IDS_SELECTED:
            DrawChunkyImage(impDraw,imgd);
            if(imgd->VisualInfo!=NULL)
            {
                DrawBevelBox(rp,x,y,imgd->Width,imgd->Height,GT_VisualInfo,imgd->VisualInfo,GTBB_Recessed,TRUE,TAG_DONE);
            }
            break;

        case IDS_DISABLED:
            {
                const UWORD *pens=impDraw->imp_DrInfo->dri_Pens;
                UWORD pattern[]={0x8888,0x2222};

                SetDrMd(rp,JAM1);
                SetAPen(rp,pens[SHADOWPEN]);
                SetAfPt(rp,pattern,1);
                RectFill(rp,x,y,x+imgd->Width-1,y+imgd->Height-1);
                SetAfPt(rp,NULL,0);
            }
            break;

        default:
            RetVal=DoSuperMethodA(IClass,(Object *)IObject,(Msg)impDraw);
            break;
    }

    return RetVal;
}


/*****
    Sous routine pour blitter une image
*****/

void DrawChunkyImage(struct impDraw *impDraw, struct TwinImageData *imgd)
{
    if(imgd->OrgImage!=NULL)
    {
        struct RastPort *rp=impDraw->imp_RPort;
        LONG w=imgd->BrushWidth,h=imgd->BrushHeight;
        LONG x=impDraw->imp_Offset.X+(imgd->Width-w)/2;
        LONG y=impDraw->imp_Offset.Y+(imgd->Height-h)/2;

        if(imgd->NewImage!=NULL)
        {
            /* Pour les écrans 8 bits ou moins, RTG ou AGA */
            LONG OffsetBase=imgd->BrushPosY*imgd->ImageWidth+imgd->BrushPosX;
            WriteChunkyPixels(rp,x,y,x+w-1,y+h-1,&imgd->NewImage[OffsetBase],imgd->ImageWidth);
        }
        else
        {
            /* Pour les écrans 16 bits ou plus RTG */
            WriteLUTPixelArray(imgd->OrgImage,imgd->BrushPosX,imgd->BrushPosY,(UWORD)imgd->ImageWidth,rp,imgd->ColorMapRGB,(UWORD)x,(UWORD)y,(UWORD)w,(UWORD)h,CTABFMT_XRGB8);
        }
    }
}

