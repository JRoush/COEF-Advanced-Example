/*
    Main unit for OBME submodules.  Handles:
    -   Initialization (writing hooks & patches)
    -   global debugging log
    -   global OBME interface

    This example plugin has two submodule DLLs - one for the CS, and one for the game.  
    This is necessary because the game and CS use slightly different definitions for many COEF classes.
    However, there is (usually) a lot of overlap between the code for the two.
    The best all-around solution, to minimize redefinition and the hassle similar but separate VS projects,
    is to use a single 'Submodule' project that compiles as the CS submodule under the 'Debug_CS' and 'Release_CS' 
    configurations, but compiles as the Game submodule under the 'Debug_Game' and 'Release_Game' configurations.
    IMPORTANT: this project must be compiled *twice*, once using a 'CS' configuration, and once using a 'Game'
    configuration.  One will generate a 'CS' dll, and the other a 'Game' dll. 
*/
#include "Submodule/Interface.h"

/*--------------------------------------------------------------------------------------------*/
// global debugging log for the submodule
_declspec(dllimport) OutputLog _gLog;
OutputLog& gLog = _gLog;

/*--------------------------------------------------------------------------------------------*/
// global submodule interface
SubmoduleInterface  g_submoduleIntfc;   

/*--------------------------------------------------------------------------------------------*/
// submodule initialization
extern "C" _declspec(dllexport) void* Initialize()
{   
    // begin initialization  
    _MESSAGE("Initializing Submodule ..."); 

    // ... Perform hooks & patches here
    
    // initialization complete
    _DMESSAGE("Submodule initialization completed sucessfully");
    return &g_submoduleIntfc;
}

/*--------------------------------------------------------------------------------------------*/
// submodule loading
#ifndef MFC
// Project uses standard windows libraries, define an entry point for the DLL to handle loading/unloading
BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved)
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:    // dll loaded
        _MESSAGE("Attaching Submodule ..."); 
        break;
    case DLL_PROCESS_DETACH:    // dll unloaded
        _MESSAGE("Detaching Submodule ...");      
        break;
    }   
    return true;
}
#else
// Project uses MFC, we define here an instance of CWinApp to make this a 'well-formed' DLL
class CSubmoduleApp : public CWinApp
{
public:
    virtual BOOL InitInstance()
    {// dll loaded       
        _MESSAGE("Attaching Submodule ..."); 
        return true;
    }
    virtual int ExitInstance() 
    {// dll unloaded
       _MESSAGE("Detaching Submodule ...");      
       return CWinApp::ExitInstance();
    }
} gApp;
#endif