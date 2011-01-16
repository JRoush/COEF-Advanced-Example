/*
    Commands execution unit for loader.  Handles:
    -   Command registration
    -   Command execution through submodule interface
    -   CSE console command parsing & execution
*/

#include "Loader/commands.h"
#include "obse/CommandTable.h"
#include "obse/ParamInfos.h"

/*--------------------------------------------------------------------------------------------*/
// Ported from CommandTable.cpp so we don't have to include the entire file
bool Cmd_Default_Execute(COMMAND_ARGS) {return true;} // nop command handler for script editor
bool Cmd_Default_Eval(COMMAND_ARGS_EVAL) {return true;} // nop command evaluator for script editor

/*--------------------------------------------------------------------------------------------*/
// Test command
static ParamInfo kParams_ThreeOptionalStrings[3] =
{
	{	"stringA",	kParamType_String,	1 },
    {	"stringB",	kParamType_String,	1 },
    {	"stringC",	kParamType_String,	1 },
};
bool Cmd_coefTest_Execute(COMMAND_ARGS)
{// handler for test command
    *result = 0; // initialize result
    char bufferA[0x200] = {{0}}; // initialize args
    char bufferB[0x200] = {{0}};
    char bufferC[0x200] = {{0}};
    ExtractArgs(PASS_EXTRACT_ARGS, bufferA, bufferB, bufferC);  // extract args from script environment
    g_submoduleInfc->COEFTest(thisObj,bufferA,bufferB,bufferC); // execute appropriate function through submodule interface
    return true;
}
DEFINE_COMMAND_PLUGIN(coefTest, "test command, accepts 3 optional string arguments", 0, 3, kParams_ThreeOptionalStrings)

/*--------------------------------------------------------------------------------------------*/
// command registration
void Register_Commands()
{// called during plugin loading to register new script commands
    _MESSAGE("Registering Commands from opcode base %04X ...", OPCODEBASE);
    g_obseIntfc->SetOpcodeBase(OPCODEBASE); // set opcode base
    g_obseIntfc->RegisterCommand(&kCommandInfo_coefTest); // register test command
}

/*--------------------------------------------------------------------------------------------*/
// CSE command parsing
void CSEPrintCallback(const char* message, const char* prefix)
{/* 
    called whenever output is provided to CSE console, if present
    commands recognized by this parser take the form 'solutionname commandName [args...]'
    the only command for this example plugin is 'Description', which prints a description of the plugin to the log
*/
    const char tag[] = SOLUTIONNAME " "; // CSE console commands to this plugin must begin with this 'tag'
    if (!message || _strnicmp(message,tag,sizeof(tag)-1) != 0) return; // output is not a command targeted to this plugin
    message += sizeof(tag)-1;
    _DMESSAGE("%s %s",prefix,message);
    
    if (_stricmp(message,"DESCRIPTION") == 0)    // 'Description' command
    {
        _MESSAGE("%s",g_submoduleInfc->Description());
    }
}
