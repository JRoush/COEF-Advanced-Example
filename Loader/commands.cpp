/*
    Commands execution unit for loader.  Handles:
    -   Command registration
    -   Command execution through submodule interface
    -   CSE console command parsing & execution
*/
#include "Loader/commands.h"
#include "obse/CommandTable.h"
#include "obse/ParamInfos.h"

// Include the OBSE version of TESObjectREFR for use in processing arguments
// We cannot use the COEF version because 
// (1) the loader doesn't import it's member functions
// (2) it would collide with the OBSE version, which is forcibly included by PluginAPI.h
// The second issue will be fixed in a future version of OBSE, so we may be 
// able to use COEF classes here in a limited capacity at some later date.
#include "obse/GameObjects.h"   

/*--------------------------------------------------------------------------------------------*/
// Ported from CommandTable.cpp so we don't have to include the entire file
bool Cmd_Default_Execute(COMMAND_ARGS) {return true;} // nop command handler for script editor
bool Cmd_Default_Eval(COMMAND_ARGS_EVAL) {return true;} // nop command evaluator for script editor

/*--------------------------------------------------------------------------------------------*/
// New script commands for MyForm
bool Cmd_ListMyForms_Execute(COMMAND_ARGS)
{
    /*
        Execution function for ListMyForms
        This method requires the use of some both COEF-specific code (the ExtendedForm component)
        and a static variable of the MyForm class.  These are not available in the loader project.
        Therefore, to proceed with this command, execution has to be transferred to the Submodule.
        This is done with the Submodule Interface - see below.
    */
    *result = 0; // initialize result
    g_submoduleInfc->ListMyForms(); // invoke the ListMyForms() method of the submodule interface to hand off execution
    return true;
}
DEFINE_COMMAND_PLUGIN(ListMyForms, "Lists all MyForms in the extended data handler", 0, 1, kParams_OneOptionalInventoryObject)
bool Cmd_GetMyFormExtraData_Execute(COMMAND_ARGS)
{
    /*
        Execution function for GetMyFormExtraData
    */
    *result = 0; // initialize result
    TESForm* form = 0;  // declare & initialize argument
    g_scriptIntfc->ExtractArgsEx(paramInfo, arg1, opcodeOffsetPtr, scriptObj, eventList, &form);  // extract argument from script environment
    if (form && (thisObj = OBLIVION_CAST(form,TESForm,TESObjectREFR))) form = 0;   // check if argument is actually a reference
    if (!form && thisObj) form = thisObj->GetBaseForm(); // if only a reference is provided, use it's base form
    *result = (SInt32)g_submoduleInfc->GetMyFormExtraData(form); // use interface function to execute command
    return true;
}
DEFINE_COMMAND_PLUGIN(GetMyFormExtraData, "Gets the 'extraData' field of a MyForm object", 0, 1, kParams_OneOptionalInventoryObject)
bool Cmd_SetMyFormExtraData_Execute(COMMAND_ARGS)
{
    /*
        Execution function for SetMyFormExtraData
    */
     *result = 0; // initialize result
    TESForm* form = 0;  // declare & initialize arguments
    UInt32 extraData = 0;
    g_scriptIntfc->ExtractArgsEx(paramInfo, arg1, opcodeOffsetPtr, scriptObj, eventList, &extraData, &form);  // extract args from script environment
    if (form && (thisObj = OBLIVION_CAST(form,TESForm,TESObjectREFR))) form = 0; // check if argument is actually a reference
    if (!form && thisObj) form = thisObj->GetBaseForm(); // if only a reference is provided, use it's base form
    g_submoduleInfc->SetMyFormExtraData(form,extraData); // use interface function to execute command
    return true;
}
DEFINE_COMMAND_PLUGIN(SetMyFormExtraData, "Sets the 'extraData' field of a MyForm object", 0, 2, kParams_OneInt_OneOptionalInventoryObject)

/*--------------------------------------------------------------------------------------------*/
// command registration
void Register_Commands()
{// called during plugin loading to register new script commands
    _MESSAGE("Registering Commands from opcode base %04X ...", OPCODEBASE);
    g_obseIntfc->SetOpcodeBase(OPCODEBASE); // set opcode base
    g_obseIntfc->RegisterCommand(&kCommandInfo_ListMyForms); // register test command
    g_obseIntfc->RegisterCommand(&kCommandInfo_GetMyFormExtraData); // register test command
    g_obseIntfc->RegisterCommand(&kCommandInfo_SetMyFormExtraData); // register test command
}

/*--------------------------------------------------------------------------------------------*/
// CSE command parsing
void CSEPrintCallback(const char* message, const char* prefix)
{/* 
    Called whenever output is provided to CSE console, if present
    Commands recognized by this parser take the form 'solutionName commandName [args...]'
*/
    char buffer[0x200];
    strcpy_s(buffer,sizeof(buffer),message);
    char* context = 0;
    if (!message || _stricmp(strtok_s(const_cast<char*>(buffer)," \t",&context),SOLUTIONNAME) != 0) return; // output is not a command targeted to this plugin
    const char* command = strtok_s(0," \t",&context);   // extract command name using strtok()
    _DMESSAGE("%s %s %s",prefix,buffer,command);
    if (!command) return;    
    const char* argA = strtok_s(0," \t",&context);  // extract 1st arg, if present
    const char* argB = strtok_s(0," \t",&context);  // extract 2nd arg, if present
    const char* argC = strtok_s(0," \t",&context);  // extract 3rd arg, if present
    if (_stricmp(command,"Description") == 0)    // 'Description' command
    {
        _MESSAGE("%s",g_submoduleInfc->Description());
    }
    else if (_stricmp(command,"ListMyForms") == 0)    // 'ListMyForms' command
    {        
        g_submoduleInfc->ListMyForms();
    }
}
