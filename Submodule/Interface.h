/*
    Submodule interface class
    Interface between submodule code and loader code, and to other obse plugins if desired
*/
#pragma once

class   TESObjectREFR;      // COEF/API/TESForms/TESObjectREFR.h

class SubmoduleInterface
{
public:
    // commands
    virtual /*00*/ void             ListMyForms();
    // internals
    virtual /*04*/ const char*      Description();  // prints & returns a short description of this plugin
};
