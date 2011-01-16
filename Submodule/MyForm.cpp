#include "Submodule/MyForm.h"
#include "Submodule/Submodule.rc.h"
#include "Components/EventManager.h"

#include "API/TES/TESDataHandler.h"
#include "API/TESFiles/TESFile.h"
#include "API/CSDialogs/TESDialog.h"
#include "Utilities/Memaddr.h"

// local declaration of module handle defined in submodule.cpp
// this handle is needed to extract resources embedded in this module (e.g. dialog templates)
extern HMODULE hModule;

// TESForm virtual method overrides
MyForm::~MyForm()
{
    _VMESSAGE("Destroying '%s'/%p:%p @ <%p>",GetEditorID(),GetFormType(),formID,this);
    /*
        Clean up any dynamically allocated members here.
    */
}
bool MyForm::LoadForm(TESFile& file)
{
    _VMESSAGE("Loading '%s'/%p:%p @ <%p>",GetEditorID(),GetFormType(),formID,this);
    /*
        Load form data from a file record.
        This method must be overwritten (TESForm::LoadForm does nothing), unless this
        form class derives from some child of TESForm that has already overwritten it.

        Chunks are usually saved in a fixed order, but all vanilla implementations of LoadForm
        allow them to be loaded in a more flexible order - hence the loop and switch over chunk
        types.

    */

    file.InitializeFormFromRecord(*this);

    char buffer[0x200];
    for(UInt32 chunktype = file.GetChunkType(); chunktype; chunktype = file.GetNextChunk() ? file.GetChunkType() : 0)
    {
        // process chunk
        switch (Swap32(chunktype))
        {

        // editor id      
        case 'EDID':
            memset(buffer,0,sizeof(buffer));    // initialize buffer to zero, good practice
            file.GetChunkData(buffer,sizeof(buffer)-1); // load chunk into buffer
            _VMESSAGE("EDID chunk: '%s'",buffer);
            SetEditorID(buffer);
            break;   

        // name
        case 'FULL':            
            TESFullName::LoadComponent(*this,file); // use base class method
            _VMESSAGE("FULL chunk: '%s'",name.c_str());
            break;

        // description
        case 'DESC':            
            TESDescription::LoadComponent(*this,file); // use base class method
            _VMESSAGE("DESC chunk: '%s'",GetDescription(this,Swap32('DESC')));
            break;

        // icon
        case 'ICON':            
            TESIcon::LoadComponent(*this,file); // use base class method
            _VMESSAGE("ICON chunk: path '%s'",texturePath.c_str());
            break;

        // extra data
        case 'DATA':
            // load all simple BaseFormComponents using LoadGenericComponents()
            // the extraData value, specific to this form class, is stored at the end of the DATA chunk
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
    /*
        Save form data to a file record.
        This method must be overwritten (TESForm::SaveFormChunks does nothing), unless this
        form class derives from some child of TESForm that has already overwritten it.
    */

    // initialize the global Form Record memory buffer, to which chunks are written by all 'Save' methods
    // InitializeFormRecord() also automatically saves the EDID chunk
    InitializeFormRecord(); 

    // Save component chunks
    TESFullName::SaveComponent(); // use base class method, saves in a FULL chunk
    TESDescription::SaveComponent(); // use base class method, saves in a DESC chunk
    TESIcon::SaveComponent(Swap32('ICON')); // use base class method, saves in an ICON chunk

    // save all simple BaseFormComponents using SaveGenericComponents() in a DATA chunk
    // the extraData value, specific to this form class, is saved at the end of the DATA chunk
    SaveGenericComponents(&extraData,sizeof(extraData));

    // close the global form record buffer & write out to disk
    FinalizeFormRecord();
}
UInt8 MyForm::GetFormType()
{
    /*
        Return the form type code.  This method *MUST* be overwritten
        It must return the type code assigned when this form type was registered with ExtendedForm.
        It may, in exceptional circumstances, also return zero as an 'invalid type' code.
    */
    return extendedForm.FormType();
}
void MyForm::CopyFrom(TESForm& form)
{
    _VMESSAGE("Copying '%s'/%p:%p @ <%p> ONTO '%s'/%p:%p @ <%p> ",
        form.GetEditorID(),form.GetFormType(),form.formID,&form,GetEditorID(),GetFormType(),formID,this);
    /*
        Copy member values.  This method *MUST* be overwritten (TESForm::CopyFrom does nothing).
        This function does not copy 'index' values like formID, editorID, mgefCode, etc. *unless*
        one of the forms involved is flagged as temporary.
    */

    MyForm* source = dynamic_cast<MyForm*>(&form);
    if (!source) return;    // source has wrong polymorphic type

    CopyGenericComponentsFrom(form); // copy all BaseFormComponent properties
    extraData = source->extraData; // copy extraData, which is specific this form class

}
bool MyForm::CompareTo(TESForm& compareTo)
{
    _VMESSAGE("Comparing '%s'/%p:%p @ <%p> TO '%s'/%p:%p @ <%p> ",
        GetEditorID(),GetFormType(),formID,this,compareTo.GetEditorID(),compareTo.GetFormType(),compareTo.formID,&compareTo);
    /*
        Return false if forms are identical, including polymorphic type.
        This method *MUST* be overwritten (TESForm::CompareTo does nothing).
        This function apparently *does* compare 'index' values like formID, editorID, mgefCode, etc.
    */

    MyForm* source = dynamic_cast<MyForm*>(&compareTo);
    if (!source) true;    // source has wrong polymorphic type

    if (CompareGenericComponentsTo(compareTo)) return true; // compare all BaseFormComponent properties
    if (extraData != source->extraData) return true;    // compare extraData, which is specific this form class

    return false; // forms are identical
}
#ifndef OBLIVION
bool MyForm::DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    /*
        Handle any dialog messages specific to this form class' dialog window
        Returns true if message handled.
    */
    if (uMsg == WM_COMMAND && LOWORD(wParam) == IDC_GETTYPEINFO)
    {
        // user has clicked on the 'Get Form Type Info' button
        char buffer[0x100];
        sprintf_s(buffer,sizeof(buffer),"MyForm from plugin '%s' \nForm type 0x%02X \nRecord type '%4.4s'",
            extendedForm.pluginName, extendedForm.FormType(), extendedForm.ShortName());
         MessageBox(dialog,buffer,"Extended Form Type Information",0);
         return true;
    }
    // call TESFormIDListView::DialogMessageCallback to process messages general to all TESFormIDListViews
    // this includes messages related to the form list control, the context menu, and the controls 
    // associated with all BaseFormComponents.
    return TESFormIDListView::DialogMessageCallback(dialog,uMsg,wParam,lParam,result);
}
void MyForm::SetInDialog(HWND dialog)
{
    /*
        Set control values to match members        
    */

    HWND control = 0;

    if (dialogHandle == 0)
    {
        // the global dialog handle hasn't been updated, meaning that this is the
        // first time this method has been called since the dialog was opened
        // use this block to initialize the dialog - populate combos, lists, etc.
        _DMESSAGE("Initializing Dialog");
        
        control = GetDlgItem(dialog,IDC_EXTRADATA); // get handle of extraData combobox
        TESComboBox::PopulateWithActorValues(control,true,true); // populate combo with a list of actor values

        dialogHandle = dialog;  // set the global dialog handle
    }

    // call TESFormIDListView::SetInDialog to update the controls associated with all BaseFormComponents
    TESFormIDListView::SetInDialog(dialog);

    // update the extraData combo selection to match the current value of extraData
    control = GetDlgItem(dialog,IDC_EXTRADATA);
    TESComboBox::SetCurSelByData(control,(void*)extraData);
}
void MyForm::GetFromDialog(HWND dialog)
{
    /*
        Set member values to match controls        
    */

    HWND control = 0;

    // call TESFormIDListView::GetFromDialog to update the properties associated with all BaseFormComponents
    TESFormIDListView::GetFromDialog(dialog);

    // set the value of extraData from the current combo selection
    control = GetDlgItem(dialog,IDC_EXTRADATA);
    extraData = (UInt32)TESComboBox::GetCurSelData(control);
}
void MyForm::CleanupDialog(HWND dialog)
{
    /*
        Perform cleanup when the dialog is closed        
    */

    dialogHandle = 0;   // clear the global dialog handle, since the window is now closed
}
#endif

// bitmask of virtual methods that need to be manually copied from vanilla vtbls (see notes in Constructor)
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
    /*
        Initialize a new instance of this form class   
        Note the initializers following the function name above - these call the default contstructors
        for each of the base classes, and initialize the value of the extraData to zero.
    */

    // set the form type assigned during extended form registration
    formType = extendedForm.FormType(); 

    /*
            Many of the Oblivion classes defined in COEF are incomplete.  In particular, while
        the number of virtual methods is always correct, the signatures of many are still unknown.
        For compatibility reasons, COEF marks such methods with the '_NOUSE' macro and does not
        import them.  This means that when the compiler automatically generates a virtual method
        table (vtbl) for each class, some of the entries are blank.       
            When dealing with instances created by the Game/CS this makes little differece, since 
        those objects use the Game/CS version of the vtbls.  If an instance is created by a COEF-
        based plugin, however, it will use the COEF version of the vtbl.  This can lead to serious 
        errors.
            To address this, the COEF vtbl must be patched at run time by copying the addresses of
        the NOUSE_ methods from the vanilla vtbl to fill in the blanks.  It needs to be done only
        once per new form class, the first time that class' constructor is called.
            A polymorphic class has one vtbl per base class.  In this example, that means six 
        vtbls.
    */
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
// This global object is used to register the form class with the ExtendedForm COEF component
ExtendedForm MyForm::extendedForm(SOLUTIONNAME,MYFORM_CLASSNAME,MYFORM_CLASSNAME,MyForm::CreateMyForm);  
TESForm* MyForm::CreateMyForm() { return new MyForm; } // method used by ExtendedForm to create new instances of this class

// CS dialog management 
#ifndef OBLIVION
UInt32 MyForm::kMenuIdentifier = 0xCC00;    // unique identifier for new menu item (see InitializeMyForm())
HWND  MyForm::dialogHandle = 0; // handle of open dialog window, if any
LRESULT MyForm_CSMenuHook(WPARAM wparam, LPARAM lparam)
{
    /*
        CSMainWindow_WMCommand event handler
        Peeks at WM_COMMAND messages sent to the main CS window
        Returns zero if message has been handled
    */
    if (LOWORD(wparam) == MyForm::kMenuIdentifier)
    {
        // WM_COMMAND messagesent by new menu item
        MyForm::OpenDialog();
        return 0;
    }
    return 1;
}
void MyForm::OpenDialog()
{
    /*
        Open CS dialog for editing forms of this type
    */
    if (dialogHandle == 0)  // check if dialog is already open
    {
         // define a 'FormEditParam' object, passed as an argument to the DialogProc
        TESDialog::FormEditParam param;
        param.formType = extendedForm.FormType();
        param.form = 0;
        // open a modeless dialog for editing forms of this type
        // TESFormIDListView::DlgProc is used as the DialogProc
        HWND handle = CreateDialogParam(    hModule, // handle of module in which dialog template is embedded
                                            MAKEINTRESOURCE(IDD_MYFORMDLG), // dialog template identifier
                                            TESDialog::csHandle, // handle of parent window
                                            &TESFormIDListView::DlgProc, // pointer to DialogProc
                                            (LPARAM)&param); // application specific parameter
        _DMESSAGE("Opened Dialog w/ handle %p",handle);
    }
}
#endif

// global initialization function
void MyForm::InitializeMyForm()
{
    /*
        Perform one-time initialization required for this form class
    */
    _MESSAGE("Initializing " MYFORM_CLASSNAME " ...");

    // register form type with the ExtendedForm COEF component
    extendedForm.Register(MYFORM_SHORTNAME);
    
    #ifndef OBLIVION

    // generate a (hopefully) unqiue menu identifier from form type
    kMenuIdentifier += extendedForm.FormType();

    // insert new item into CS main menu
    HMENU menu = GetMenu(TESDialog::csHandle); // get main CS menu handle
    menu = GetSubMenu(menu,3);              // get 'World' submenu handle
    MENUITEMINFO iteminfo;      
    iteminfo.cbSize = sizeof(iteminfo);        
    iteminfo.fMask = MIIM_ID | MIIM_FTYPE | MIIM_STRING;
    iteminfo.fType = MFT_STRING;
    char menulabel[] = SOLUTIONNAME " " MYFORM_CLASSNAME " ...";
    iteminfo.dwTypeData = menulabel;
    iteminfo.cch = sizeof(menulabel);
    iteminfo.wID = kMenuIdentifier;
    InsertMenuItem(menu,0,true,&iteminfo); // Insert new entry at top of submenu

    // register event handler with the EventManager COEF component
    // the CSMainWindow_WMCommand occurs when WM_COMMAND messages are sent to the main CS window
    // This includes menu selections by the user, so this event handler will be able to
    // determine when the user clicks on the newly added menu item.
    EventManager::RegisterEventCallback(EventManager::CSMainWindow_WMCommand,&MyForm_CSMenuHook);

    #endif
}