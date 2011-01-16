/* 
    Example of a new Form class
*/
#pragma once

// base classes
#include "API/TESForms/TESForm.h" // TESFormIDListView
#include "API/TESForms/BaseFormComponent.h" // additonal form components
#include "Components/ExtendedForm.h"

#define MYFORM_SHORTNAME "MYFM" 

class MyForm :  public TESFormIDListView, // a child of TESForm, defines methods for editing all forms of this type in a single CS dialog
                public TESFullName,     // gives this form a standard "name" property
                public TESDescription,  // gives this form a standard "description" property 
                public TESIcon,         // gives this form an icon
                public TESValueForm,    // gives this form a gold value
                public TESWeightForm    // gives this form a weight
{
public:

    // members
    //     /*00/00*/ TESForm        18/24
    //     /*18/24*/ TESFullName    0C/0C
    //     /*24/30*/ TESDescription 08/10
    //     /*2C/40*/ TESIcon        0C/18
    //     /*38/58*/ TESValueForm   08/08
    //     /*40/60*/ TESWeightForm  08/08
    MEMBER /*48/68*/ UInt32         extraData;  // one new member, in addition to the base clases
    //       4C/6C <-- total object size

    // TESFormIDListView virtual method overrides
    // Note the use of the '_LOCAL' macro to indicate that these functions are being (re)implemented by this plugin
    _LOCAL /*010/034*/ virtual              ~MyForm();
    _LOCAL /*01C/040*/ virtual bool         LoadForm(TESFile& file);
    _LOCAL /*024/048*/ virtual void         SaveFormChunks();
    _LOCAL /*070/074*/ virtual UInt8        GetFormType();
    _LOCAL /*0B4/0B8*/ virtual void         CopyFrom(TESForm& form);
    _LOCAL /*0B8/0BC*/ virtual bool         CompareTo(TESForm& compareTo);
    #ifndef OBLIVION
    _LOCAL /*---/10C*/ virtual bool         DialogMessageCallback(HWND dialog, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& result); 
    _LOCAL /*---/114*/ virtual void         SetInDialog(HWND dialog);
    _LOCAL /*---/118*/ virtual void         GetFromDialog(HWND dialog);
    _LOCAL /*---/11C*/ virtual void         CleanupDialog(HWND dialog);
    #endif

    // constructor
    _LOCAL MyForm();
    
    // COEF ExtendedForm component
    static ExtendedForm         extendedForm; 
    _LOCAL static TESForm*      CreateMyForm(); // creates a blank MyForm

    // CS dialog management
    #ifndef OBLIVION
    static UInt32               kMenuIdentifier;   // identifier for new menu item
    static HWND                 dialogHandle; // handle of dialog if currently open
    _LOCAL static void          OpenDialog(); // opens dialog if it is not currently open
    #endif

    // global initialization function, called once when submodule is first loaded
    _LOCAL static void          InitializeMyForm();

private:
};