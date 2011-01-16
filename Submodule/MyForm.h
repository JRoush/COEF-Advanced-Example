/* 
    Example of a new Form class

    See Components/ExtendedForm.h for general details on new form classes.  The comments
    here are specific to this example.

    This new form type is derived from multiple BaseFormComponents, which give it the 'standard'
    Name, Description, Icon, Gold value, and Weight properties.  The Game/CS code for these classes
    is frequently used implicitly (for example, the CopyFrom() method defined in MyForm.cpp calls
    the TESForm::CopyGenericComponentsFrom(), which in turn calls the CopyFrom methods for each
    individual BaseFormComponent to copy their properties.

    The most important base class is TESFormIDListView.  This base derives from TESForm, so it provides
    the necessary inheritance to that class.  It also has additional methods targeted to a specific
    style of CS dialog - one where all forms of this type are edited in a single dialog, a la EffectSettings,
    TESRaces, etc.

    This form class takes up several limited resources, which can potentially cause conflicts.
    -   Form Type Code
        As discussed in ExtendedForm.h, this new form class is automatically assigned a form type 
        code upon registration.

    -   Short Name / Plugin Name / Class Name:
        As discussed in ExtendedForm.h, this new form class *must* define a 4-character short name 
        for use in serialization, and this string must be *unique* among all new form classes from 
        all plugins.
    
    -   Menu Identifier:
        Each item in a menu must have a unique identifier, used to identify it when it is selected 
        by the user.  There are quite a few possible identifiers (any 16 bit number > ~0x4000 works),
        but the possibility for conflict with other plugins that insert menu or toolbar items remains.
        The identifier for the new menu item introduced to open the MyForms dialog is determined by 
        adding the assigned form type to a base value 0xCC00.
*/
#pragma once

// base classes
#include "API/TESForms/TESForm.h" // TESFormIDListView
#include "API/TESForms/BaseFormComponent.h" // additonal form components
#include "Components/ExtendedForm.h"

// Macros for short name and class name, which must be unique among all plugins, and just this plugin, respectively
#define MYFORM_SHORTNAME "MYFM"
#define MYFORM_CLASSNAME "MyForm" 

// MyForm - new form class definition
class MyForm :  public TESFormIDListView,
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