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
    virtual /*00*/ void             COEFTest(TESObjectREFR* thisObj, const char* argA, const char* argB, const char* argC);
    // internals
    virtual /*04*/ const char*      Description();  // prints & returns a short description of this plugin
};
