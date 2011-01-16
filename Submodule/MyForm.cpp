#include "Submodule/MyForm.h"
#include "Submodule/Submodule.rc.h"
#include "Components/EventManager.h"

#include "API/TES/TESDataHandler.h"
#include "API/TESFiles/TESFile.h"
#include "API/CSDialogs/TESDialog.h"
#include "Utilities/Memaddr.h"

// local declaration of module handle defined in submodule.cpp
extern HMODULE hModule;

// TESForm virtual method overrides
MyForm::~MyForm()
{
    _VMESSAGE("Destroying '%s'/%p:%p @ <%p>",GetEditorID(),GetFormType(),formID,this);
}
bool MyForm::LoadForm(TESFile& file)
{
    _VMESSAGE("Loading '%s'/%p:%p @ <%p>",GetEditorID(),GetFormType(),formID,this);
    file.InitializeFormFromRecord(*this);

    char buffer[0x200];
    for(UInt32 chunktype = file.GetChunkType(); chunktype; chunktype = file.GetNextChunk() ? file.GetChunkType() : 0)
    {
        // process chunk
        switch (Swap32(chunktype))
        {

        // editor id chunk      
        case 'EDID':
            // already processed by loading routine 
            memset(buffer,0,sizeof(buffer));
            file.GetChunkData(buffer,sizeof(buffer)-1);
            _VMESSAGE("EDID chunk: '%s'",buffer);
            SetEditorID(buffer);
            break;   

        // name
        case 'FULL':            
            TESFullName::LoadComponent(*this,file);
            _VMESSAGE("FULL chunk: '%s'",name.c_str());
            break;

        // description
        case 'DESC':            
            TESDescription::LoadComponent(*this,file);
            _VMESSAGE("DESC chunk: '%s'",GetDescription(this,Swap32('DESC')));
            break;

        // icon
        case 'ICON':            
            TESIcon::LoadComponent(*this,file);
            _VMESSAGE("ICON chunk: path '%s'",texturePath.c_str());
            break;

        // extra data
        case 'DATA':
            TESForm::LoadGenericComponents(file,&extraData,sizeof(extraData));
            _VMESSAGE("DATA chunk: value %i weight %f extraData %i",goldValue,weight,extraData);
            break;

        // unrecognized chunk type
        default:
            gLog.PushStyle();
            _WARNING("Unexpected chunk '%4.4s' {%08X} w/ size %08X", &chunktype, chunktype, file.currentChunk.chunkLength);
            gLog.PopStyle();
            break;

        }
        // continue to next chunk
    } 
    return true;
}
void MyForm::SaveFormChunks()
{
    _VMESSAGE("Saving '%s'/%p:%p @ <%p>",GetEditorID(),GetFormType(),formID,this);
    InitializeFormRecord();

    TESFullName::SaveComponent();
    TESDescription::SaveComponent();
    TESIcon::SaveComponent(Swap32('ICON'));
    SaveGenericComponents(&extraData,sizeof(extraData));

    FinalizeFormRecord();
}
UInt8 MyForm::GetFormType()
{
    return extendedForm.FormType();
}
void MyForm::CopyFrom(TESForm& form)
{
    _VMESSAGE("Copying '%s'/%p:%p @ <%p> ONTO '%s'/%p:%p @ <%p> ",
        form.GetEditorID(),form.GetFormType(),form.formID,&form,GetEditorID(),GetFormType(),formID,this);

    MyForm* source = dynamic_cast<MyForm*>(&form);
    if (!source) return;    // source has wrong polymorphic type

    CopyGenericComponentsFrom(form); // copy generic components

    extraData = source->extraData;

}
bool MyForm::CompareTo(TESForm& compareTo)
{
    _VMESSAGE("Comparing '%s'/%p:%p @ <%p> TO '%s'/%p:%p @ <%p> ",
        GetEditorID(),GetFormType(),formID,this,compareTo.GetEditorID(),compareTo.GetFormType(),compareTo.formID,&compareTo);

    MyForm* source = dynamic_cast<MyForm*>(&compareTo);
    if (!source) true;    // source has wrong polymorphic type

    if (CompareGenericComponentsTo(compareTo)) return true; // mismatch in generic components

    if (extraData != source->extraData) return true;    // extraData doesn't match

    return false;
}
#ifndef OBLIVION
bool MyForm::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if (uMsg == WM_COMMAND && LOWORD(wParam) == IDC_GETTYPEINFO)
    {
        // user has clicked on the 'Get Form Type Info' button
        char buffer[0x100];
        sprintf_s(buffer,sizeof(buffer),"MyForm from plugin '%s' \nForm type 0x%02X \nRecord type '%4.4s'",
            extendedForm.pluginName, extendedForm.FormType(), extendedForm.ShortName());
         MessageBox(dialog,buffer,"Extended Form Type Information",0);
         return true;
    }
    return TESFormIDListView::DialogMessageCallback(dialog,uMsg,wParam,lParam,result);
}
void MyForm::SetInDialog(HWND dialog)
{
    HWND control = 0;

    if (dialogHandle == 0)
    {
        _DMESSAGE("Initializing Dialog");
        
        control = GetDlgItem(dialog,IDC_EXTRADATA);
        TESComboBox::PopulateWithActorValues(control,true,true);

        dialogHandle = dialog;
    }
    TESFormIDListView::SetInDialog(dialog);

    control = GetDlgItem(dialog,IDC_EXTRADATA);
    TESComboBox::SetCurSelByData(control,(void*)extraData);
}
void MyForm::GetFromDialog(HWND dialog)
{
    HWND control = 0;

    TESFormIDListView::GetFromDialog(dialog);

    control = GetDlgItem(dialog,IDC_EXTRADATA);
    extraData = (UInt32)TESComboBox::GetCurSelData(control);
}
void MyForm::CleanupDialog(HWND dialog)
{
    dialogHandle = 0;
}
#endif

// bitmask of virtual methods that need to be manually copied from vanilla vtbls
#ifdef OBLIVION
    const UInt32 TESForm_NoUseMethods[2] = { 0x86048400, 0x001E1FC7 };
    const UInt32 BaseFormComponent_NoUseMethods[1] = {0x00};
#else
    const UInt32 TESForm_NoUseMethods[3] = { 0x090800D0, 0xDF783F8F, 0x00000105 };
    const UInt32 BaseFormComponent_NoUseMethods[1] = {0xD0};
#endif
memaddr TESForm_vtbl                (0x00A3BE3C,0x0093DA0C);
memaddr TESFullName_vtbl            (0x00A322A0,0x00938118);
memaddr TESDescription_vtbl         (0x00A3B938,0x0093D00C);
memaddr TESIcon_vtbl                (0x00A320A4,0x0093C9DC);
memaddr TESValueForm_vtbl           (0x00A3C680,0x0093F4F4);
memaddr TESWeightForm_vtbl          (0x00A3C6A0,0x0093F588);

// Constructor
MyForm::MyForm()
: TESFormIDListView(), TESFullName(), TESDescription(),TESIcon(),TESWeightForm(),TESValueForm(), extraData(0)
{
    _VMESSAGE("Constructing '%s'/%p:%p @ <%p>",GetEditorID(),GetFormType(),formID,this);

    // use form type assigned during extended form registration
    formType = extendedForm.FormType(); 

    // first time through - set up vtbl
    if (static bool runonce = true)
    {
        // patch up TESForm vtbl, including it's BaseFormComponent section        
        memaddr thisvtbl = (UInt32)memaddr::GetObjectVtbl(this);
        _MESSAGE("Patching MyForm TESForm vtbl @ <%p>",thisvtbl);
        gLog.Indent();
        for (int i = 0; i < sizeof(TESForm_NoUseMethods)*0x8; i++)
        {
            if ((TESForm_NoUseMethods[i/0x20] >> (i%0x20)) & 1)
            {
                thisvtbl.SetVtblEntry(i*4,TESForm_vtbl.GetVtblEntry(i*4));
                _VMESSAGE("Patched Offset 0x%04X",i*4);
            }
        }
        gLog.Outdent();

        // patch up component vtbls
        memaddr nameVtbl = (UInt32)memaddr::GetObjectVtbl(dynamic_cast<TESFullName*>(this));
        memaddr descVtbl = (UInt32)memaddr::GetObjectVtbl(dynamic_cast<TESDescription*>(this));
        memaddr iconVtbl = (UInt32)memaddr::GetObjectVtbl(dynamic_cast<TESIcon*>(this));
        memaddr valueVtbl = (UInt32)memaddr::GetObjectVtbl(dynamic_cast<TESValueForm*>(this));
        memaddr weightVtbl = (UInt32)memaddr::GetObjectVtbl(dynamic_cast<TESWeightForm*>(this));
        _MESSAGE("Patching MyForm TESFullName vtbl @ <%p> ...",nameVtbl);        
        _MESSAGE("Patching MyForm TESDescription vtbl @ <%p> ...",descVtbl);        
        _MESSAGE("Patching MyForm TESIcon vtbl @ <%p> ...",iconVtbl);
        _MESSAGE("Patching MyForm TESValueForm vtbl @ <%p> ...",valueVtbl);
        _MESSAGE("Patching MyForm TESWeightForm vtbl @ <%p> ...",weightVtbl);
        gLog.Indent();
        for (int i = 0; i < sizeof(BaseFormComponent_NoUseMethods)*0x8; i++)
        {
            if ((BaseFormComponent_NoUseMethods[i/0x20] >> (i%0x20)) & 1)
            {
                nameVtbl.SetVtblEntry(i*4,TESFullName_vtbl.GetVtblEntry(i*4));
                descVtbl.SetVtblEntry(i*4,TESDescription_vtbl.GetVtblEntry(i*4));
                iconVtbl.SetVtblEntry(i*4,TESIcon_vtbl.GetVtblEntry(i*4));
                valueVtbl.SetVtblEntry(i*4,TESValueForm_vtbl.GetVtblEntry(i*4));
                weightVtbl.SetVtblEntry(i*4,TESWeightForm_vtbl.GetVtblEntry(i*4));
                _VMESSAGE("Patched Offset 0x%04X",i*4);
            }
        }
        gLog.Outdent();

        runonce  =  false;
    }
}
// COEF ExtendedForm component
ExtendedForm MyForm::extendedForm(SOLUTIONNAME,"MyForm","My Form",MyForm::CreateMyForm);  
TESForm* MyForm::CreateMyForm() { return new MyForm; }

// CS dialog management 
#ifndef OBLIVION
UInt32 MyForm::kMenuIdentifier = 0xCC00;
HWND  MyForm::dialogHandle = 0;
LRESULT MyForm_CSMenuHook(WPARAM wparam, LPARAM lparam)
{
    if (LOWORD(wparam) == MyForm::kMenuIdentifier)
    {
        // WM_COMMAND messagesent by added menu item
        MyForm::OpenDialog();
        return 0;
    }
    return 1;
}
void MyForm::OpenDialog()
{
    if (dialogHandle == 0)
    {
        TESDialog::FormEditParam param;
        param.formType = extendedForm.FormType();
        param.form = 0;
        HWND handle = CreateDialogParam(hModule,MAKEINTRESOURCE(IDD_MYFORMDLG),TESDialog::csHandle,&TESFormIDListView::DlgProc,(LPARAM)&param);
        _DMESSAGE("Opened Dialog w/ handle %p",handle);
    }
}
#endif

// global initialization function
void MyForm::InitializeMyForm()
{
    _MESSAGE("Initializing MyForm ...");

    // register form type
    extendedForm.Register(MYFORM_SHORTNAME);
    
    #ifndef OBLIVION

    // generate a (hopefully) unqiue menu identifier from form type
    kMenuIdentifier += extendedForm.FormType();

    // insert menu item
    HMENU menu = GetMenu(TESDialog::csHandle); // get main CS menu handle
    menu = GetSubMenu(menu,3);              // get 'World' submenu handle
    MENUITEMINFO iteminfo;      
    iteminfo.cbSize = sizeof(iteminfo);        
    iteminfo.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STRING;
    iteminfo.fType = MFT_STRING;
    char menulabel[] = SOLUTIONNAME " Forms ...";
    iteminfo.dwTypeData = menulabel;
    iteminfo.cch = sizeof(menulabel);
    iteminfo.wID = kMenuIdentifier;
    InsertMenuItem(menu,0,true,&iteminfo); // Insert new entry at top of submenu

    // register event handler
    EventManager::RegisterEventCallback(EventManager::CSMainWindow_WMCommand,&MyForm_CSMenuHook);

    #endif
}