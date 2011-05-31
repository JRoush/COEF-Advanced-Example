/*
    Submodule interface class
    Interface between submodule code and loader code, and to other obse plugins if desired
*/
#pragma once

class   TESObjectREFR;      // COEF/API/TESForms/TESObjectREFR.h
class   TESForm;            // COEF/API/TESForms/TESForm.h

class SubmoduleInterface
{
public:
    // commands
    virtual /*00*/ void             ListMyForms();
    virtual /*04*/ void             SetMyFormExtraData(TESForm* myForm, UInt32 extraData);
    virtual /*04*/ UInt32           GetMyFormExtraData(TESForm* form);
    // internals
    virtual /*04*/ const char*      Description();  // prints & returns a short description of this plugin
};
