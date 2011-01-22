/*
    Commands execution unit for loader.  Handles:
    -   Command registration
    -   Command execution through submodule interface
    -   CSE console command parsing & execution
*/
#include "Loader/commands.h"
#include "obse/CommandTable.h"
#include "obse/ParamInfos.h"


// NOTE: The linking information available to the submodule that allows it to use all members and 
// methods is not part of the Loader project.  Use ONLY virtual methods, ordinary member variables, 
// and static constants from the files below.
// NOTE: Some game classes defined by COEF will conflict with the OBSE classes of the same name.
// Be sure that only one or the other is included in this file.
#include "API/TESForms/TESObjectREFR.h"
#include "Submodule/MyForm.h"

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
        See notes in Cmd_DumpAllMyForms_Execute for an overview of the 'general' way to write execution functions
        Because this function is so simple - it can be written using only ordinary member variables on pre-existing
        MyForm objects - there is no need to use the Submodule Entry to pass the execution into the submodule dll.
    */
    *result = 0; // initialize result
    TESForm* form = 0;  // declare & initialize argument
    ExtractArgsEx(paramInfo, arg1, opcodeOffsetPtr, scriptObj, eventList, &form);  // extract argument from script environment
    if (form && (thisObj = dynamic_cast<TESObjectREFR*>(form))) form = 0;   // check if argument is actually a reference
    if (!form && thisObj) form = thisObj->GetBaseForm(); // if only a reference is provided, use it's base form
    MyForm* myform = dynamic_cast<MyForm*>(form); // typecast to MyForm
    if (!myform) return true; // argument was not a MyForm object
    _MESSAGE("GetMyFormExtraData ( %08X, %i )", myform ? myform->formID : 0, myform->extraData);
    *result = myform->extraData; // return the extraData field from the argument
    return true;
}
DEFINE_COMMAND_PLUGIN(GetMyFormExtraData, "Gets the 'extraData' field of a MyForm object", 0, 1, kParams_OneOptionalInventoryObject)
bool Cmd_SetMyFormExtraData_Execute(COMMAND_ARGS)
{
    /*
        Execution function for SetMyFormExtraData
        See notes in Cmd_DumpAllMyForms_Execute for an overview of the 'general' way to write execution functions
        Because this function is so simple - it can be written using only ordinary member variables on pre-existing
        MyForm objects - there is no need to use the Submodule Entry to pass the execution into the submodule dll.
    */
     *result = 0; // initialize result
    TESForm* form = 0;  // declare & initialize arguments
    UInt32 extraData = 0;
    ExtractArgsEx(paramInfo, arg1, opcodeOffsetPtr, scriptObj, eventList, &extraData, &form);  // extract args from script environment
    if (form && (thisObj = dynamic_cast<TESObjectREFR*>(form))) form = 0;   // check if argument is actually a reference
    if (!form && thisObj) form = thisObj->GetBaseForm();    // if only a reference is provided, use it's base form
    MyForm* myform = dynamic_cast<MyForm*>(form);   // typecast to MyForm
    if (!myform) return true; // argument was not a MyForm object
    _MESSAGE("SetMyFormExtraData ( %08X, %i -> %i )", myform ? myform->formID : 0, myform->extraData, extraData);
    myform->extraData = extraData;  // set the extraData field on the argument
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
