/*
    Main unit for loader.  Handles:
    -   DLL entry point
    -   OBSE plugin interface        
    -   OBSE,CSE messaging interface
    -   Serialization interface
    -   global debugging log

    The basic structure of this plugin is:
    -   A 'loader' dll, located in the OBSE/Plugins folder, which is loaded & processed by obse.
        The loader does the necessary version checks, registers obse interfaces & script commands, etc.
        It then loads ...
        -   ExportInjector.dll, which the player should have installed in the OBSE/Plugins/COEF folder.
            ExportInjector finds *.eed files (hich the player should also have installed with COEF), and uses them to
            build a table of exported functions and data objects for the Game/CS.
        Once ExportInjector is loaded (or if it's already been loaded by another plugin), then the loader loads a ...
        -   Submodule DLL, located in some subdirectory of Oblivion/Data.  This submodule contains the actual 'guts' of
            the plugin - all of the code that is based on COEF classes.  It exports a special named function, Initialize(),
            which writes any hooks and patches needed by the plugin, and returns a ...
            -   Submodule interface, as laid out in Submodule/Interface.h.  This interface is how the loader communicates
                with the submodule, to invoke next script commands or react to messages from obse and other plugins.
                This interface might also be dispatched to other plugins, so that they can invoke the commands directly.

*/
#include "obse/PluginAPI.h"             // for interfacing with obse
#include "Loader/commands.h"            // defines script & console commands
#include "Submodule/Version.h"          // version info for this plugin
#include "Submodule/Interface.h"        // for interfacing with the submodule
#include "Submodule/CSE_Interface.h"    // for interfacing with CSE, if present

/*--------------------------------------------------------------------------------------------*/
// global debugging log
OutputTarget*   _gLogFile = NULL;
_declspec(dllexport) OutputLog _gLog;
OutputLog&      gLog = _gLog;

/*--------------------------------------------------------------------------------------------*/
// global interfaces and handles
PluginHandle                    g_pluginHandle          = kPluginHandle_Invalid;    // identifier for this plugin, for internal use by obse
const OBSEInterface*            g_obseIntfc             = NULL; // master obse interface, for generating more specific intefaces below
OBSEArrayVarInterface*          g_arrayIntfc            = NULL; // obse array variable interface
OBSEStringVarInterface*         g_stringIntfc           = NULL; // obse string variable interface
OBSEScriptInterface*            g_scriptIntfc           = NULL; // obse script interface
OBSECommandTableInterface*      g_cmdIntfc              = NULL; // obse script command interface
OBSESerializationInterface*     g_serializationIntfc    = NULL; // obse cosave serialization interface
OBSEMessagingInterface*         g_messagingIntfc        = NULL; // obse inter-plugin messaging interface
HMODULE                         g_hExportInjector       = 0;    // windows module handle for ExportInjector library
HMODULE                         g_hSubmodule            = 0;    // windows module handle for submodule dll
SubmoduleInterface*             g_submoduleInfc         = NULL; // submodule interface
CSEInterface*                   g_cseIntfc              = NULL; // CSE master interface, if CSE is present
CSEConsoleInterface*            g_cseConsoleInfc        = NULL; // CSE console interface

/*--------------------------------------------------------------------------------------------*/
// CSE console output target
// this is a custom OutputTarget that forwards (plain text) output to the CSE console, if CSE is present 
class CSEConsoleTarget : public BufferTarget
{
public:
    // interface
    virtual void    WriteOutputLine(const OutputStyle& style, time_t time, int channel, const char* source, const char* text)
    {
        if (!g_cseConsoleInfc || !g_cseConsoleInfc->PrintToConsole) return;  // bad CSE console interface
        BufferTarget::WriteOutputLine(consoleStyle,time,channel,source,text); // generate output line, without timestamp or source
        g_cseConsoleInfc->PrintToConsole(SOLUTIONNAME,LastOutputLine());  // write output line to CSE console
    }
    // static output style
    OutputStyle  consoleStyle;
} _CSETarget;

/*--------------------------------------------------------------------------------------------*/
// Serialization routines - these are just stubs, to illustrate the basic concept
static void SaveCallback(void * reserved)
{// called during game save by obse to serialize private plugin data to the obse cosave
    _MESSAGE("Writing to cosave ...");
    g_serializationIntfc->OpenRecord('HEAD',RECORD_VERSION(COSAVE_VERSION));    // open a 'HEAD" record for this plugin
	const char* desc = g_submoduleInfc ? g_submoduleInfc->Description() : SOLUTIONNAME;  // get a descriptive string for this plugin
	g_serializationIntfc->WriteRecordData(desc, strlen(desc)); // write description to 'HEAD' record
    // the 'HEAD' record (which is the only record written by this plugin) is automatically closed at the end of this function
}
static void LoadCallback(void * reserved)
{// called during game load by obse to deserialize private plugin data from the obse cosave
    _MESSAGE("Loading from cosave ...");
    UInt32	type, version, length;
	char	buf[512];
    gLog.Indent();
	while(g_serializationIntfc->GetNextRecordInfo(&type, &version, &length)) // loop through records from this plugin
	{
        switch(type)
		{
			case 'HEAD':    // this is a 'HEAD' record
				g_serializationIntfc->ReadRecordData(buf, length); // copy record contents into a string buffer & print
				buf[length] = 0;
                _DMESSAGE("HEADER RECORD, Version(%08X): '%s'", version, buf);
				break;
			default:        // record of unkown type
                _DMESSAGE("Record Type[%.4s] Version(%08X) Length(%08X)", &type, version, length);
				break;
		}
	}
    gLog.Outdent();
}
static void PreloadCallback(void * reserved)
{// called *before* game load by obse to deserialize private plugin data from the obse cosave	
    _DMESSAGE("Preload Game callback ...");
}
static void NewGameCallback(void * reserved)
{// called when a new game is started by obse to initialize private plugin data
    _DMESSAGE("New Game callback ...");
}

/*--------------------------------------------------------------------------------------------*/
// Messaging API
void GeneralMessageHandler(OBSEMessagingInterface::Message* msg)
{// registered during plugin load; recieves messages from plugins that don't have explicit listeners
    if (msg->sender && strcmp(msg->sender,"CSE") == 0 && msg->type == 'CSEI')
    {
        // CSE interface message
        _VMESSAGE("Received CSE interface message");         
        g_cseIntfc = (CSEInterface*)msg->data; // message data is a pointer to CSE interface object
        if (g_cseIntfc) g_cseConsoleInfc = (CSEConsoleInterface*)g_cseIntfc->InitializeInterface(CSEInterface::kCSEInterface_Console); // get console interface
        if (g_submoduleInfc && g_cseConsoleInfc) 
        {
            _VMESSAGE("Attached to CSE console");
            gLog.AttachTarget(_CSETarget);   // attach CSE console target to output log
            _CSETarget.LoadRulesFromINI("Data\\obse\\Plugins\\" SOLUTIONNAME "\\Settings.ini","CSEConsole.Log"); // load target rules for CSE console
            _CSETarget.consoleStyle.includeTime = _CSETarget.consoleStyle.includeSource = false; // setup output style for console
            g_cseConsoleInfc->RegisterCallback(CSEPrintCallback); // register parser for CSE console output
        }
        return;
    }
    _VMESSAGE("Received unknown message type=%i from '%s' w/ data <%p> len=%04X", msg->type, msg->sender, msg->data, msg->dataLen);
}
void OBSEMessageHandler(OBSEMessagingInterface::Message* msg)
{// registered during plugin load; recieves (event) messages from OBSE itself
    int x = 0;
	switch (msg->type)
	{
	case OBSEMessagingInterface::kMessage_ExitGame:
		_VMESSAGE("Received 'exit game' message");
		break;
	case OBSEMessagingInterface::kMessage_ExitToMainMenu:
		_VMESSAGE("Received 'exit game to main menu' message");
		break;
	case OBSEMessagingInterface::kMessage_PostLoad:
		_VMESSAGE("Received 'post-load' message"); 
		break;
	case OBSEMessagingInterface::kMessage_LoadGame:
	case OBSEMessagingInterface::kMessage_SaveGame:
		_VMESSAGE("Received 'save/load game' message with file path %s", msg->data);
		break;
	case OBSEMessagingInterface::kMessage_Precompile: 
		_VMESSAGE("Received 'pre-compile' message.");
		break;
	case OBSEMessagingInterface::kMessage_PreLoadGame:
		_VMESSAGE("Received 'pre-load game' message with file path %s", msg->data);
		break;
	case OBSEMessagingInterface::kMessage_ExitGame_Console:
		_VMESSAGE("Received 'quit game from console' message");
		break;
    case OBSEMessagingInterface::kMessage_PostLoadGame:
        _VMESSAGE("Received 'post-load game' message");
        break;
    case 9: // TODO - use appropriate constant
        _VMESSAGE("Received post-post-load message");
        // request a CSE interface
        _VMESSAGE("Requesting CSE interface ... ");
        g_messagingIntfc->Dispatch(g_pluginHandle, 'CSEI', NULL, 0, "CSE");
        break;
	default:
		_VMESSAGE("Received OBSE message type=%i from '%s' w/ data <%p> len=%04X", msg->type, msg->sender, msg->data, msg->dataLen);
		break;
	}
}

/*--------------------------------------------------------------------------------------------*/
// OBSE plugin query
extern "C" bool _declspec(dllexport) OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info)
{
    // attach html-formatted log file to loader output handler
    HTMLTarget* tgt = new HTMLTarget(obse->isEditor ? "Data\\obse\\plugins\\" SOLUTIONNAME "\\CS.log.html" 
                                                    : "Data\\obse\\plugins\\" SOLUTIONNAME "\\Game.log.html");
    _gLogFile = tgt;
    gLog.AttachTarget(*_gLogFile);
     // load rules for loader output from INI
    tgt->LoadRulesFromINI("Data\\obse\\Plugins\\" SOLUTIONNAME "\\Settings.ini",obse->isEditor ? "CS.Log" : "Game.Log");

	// fill out plugin info structure
	info->infoVersion = PluginInfo::kInfoVersion;   // info structure version
	info->name = SOLUTIONNAME;                      // plugin name
	info->version = RECORD_VERSION(0);              // plugin version
    _MESSAGE("OBSE Query (CSMode = %i) v%p (%i.%i beta%i)",obse->isEditor,RECORD_VERSION(0),MAJOR_VERSION,MINOR_VERSION,BETA_VERSION);

    // check obse version
    g_obseIntfc = obse;
    if(obse->obseVersion < OBSE_VERSION_INTEGER)
	{
		_FATALERROR("OBSE version too old (got %08X expected at least %08X)", obse->obseVersion, OBSE_VERSION_INTEGER);
		return false;
	}
    
    // load ExportInjector library, which will automatically find *.eed files build an export table according to it's ini settings
    // ExportInjector *must* be loaded before the submodule, since loading the submodule will explicitly require the export table
    // Technically, only the first COEF-based plugin needs to perform this step; further LoadLibrary calls on ExportInjector.dll will do nothing
    _MESSAGE("Loading ExportInjector ...");
    g_hExportInjector = LoadLibrary("Data\\obse\\Plugins\\COEF\\API\\ExportInjector.dll");
    if (g_hExportInjector)
    {
        _DMESSAGE("ExportInjector loaded at <%p>", g_hExportInjector);
    }
    else
    {
        _FATALERROR("Could not load ExportInjector.dll.  Check that this file is installed correctly.");
        return false;
    }

    // load submodule 
    // the submodule dll contains the actual "meat" of the plugin, or as much of it as is based on COEF
    // in this example, there are two different submodules to choose from (CS vs Game, see Submodule.cpp for details)
    const char* modulename = obse->isEditor ? "Data\\obse\\plugins\\" SOLUTIONNAME "\\Submodule.CS.dll" 
                                            : "Data\\obse\\plugins\\" SOLUTIONNAME "\\Submodule.Game.dll";
    _MESSAGE("Loading Submodule '%s' ...", modulename);
    g_hSubmodule = LoadLibrary(modulename);
    if (g_hSubmodule)
    {
        _DMESSAGE("Submodule loaded at <%p>", g_hSubmodule);
    }
    else
    {
        _FATALERROR("Could not load submodule '%s'.  Check that this file is installed correctly.", modulename);
        return false;
    }

	if(!obse->isEditor) // game-specific checks
	{		
        // check Game version
		if(obse->oblivionVersion != OBLIVION_VERSION)
		{
			_FATALERROR("Incorrect Oblivion version (got %08X need %08X)", obse->oblivionVersion, OBLIVION_VERSION);
			return false;
        }

        // get serialization interface	
        g_serializationIntfc = (OBSESerializationInterface*)obse->QueryInterface(kInterface_Serialization);
		if(!g_serializationIntfc)
		{
			_FATALERROR("Serialization interface not found");
			return false;
		}
		if(g_serializationIntfc->version < OBSESerializationInterface::kVersion)
		{
			_FATALERROR("Incorrect serialization version found (got %08X need %08X)", g_serializationIntfc->version, OBSESerializationInterface::kVersion);
			return false;
		}
        
        // get array var interface
		g_arrayIntfc = (OBSEArrayVarInterface*)obse->QueryInterface(kInterface_ArrayVar);
		if (!g_arrayIntfc)
		{
			_FATALERROR("Array interface not found");
			return false;
		}

        // get script Interface
		g_scriptIntfc = (OBSEScriptInterface*)obse->QueryInterface(kInterface_Script);	
        if (!g_scriptIntfc)
		{
			_FATALERROR("Script interface not found");
			return false;
		}
	}
	else    // CS specific checks
	{
		// check CS version
        if(obse->editorVersion != CS_VERSION)
		{
			_FATALERROR("TESCS version %08X, expected %08X)", obse->editorVersion, CS_VERSION);
			return false;
		}
	}

	// all checks pass, this plugin should continue to loading stage
	return true;
}

/*--------------------------------------------------------------------------------------------*/
// OBSE plugin load
extern "C" bool _declspec(dllexport) OBSEPlugin_Load(const OBSEInterface * obse)
{
	_MESSAGE("OBSE Load (CSMode = %i)",obse->isEditor);

    // get obse interface, plugin handle
    g_obseIntfc = obse;
	g_pluginHandle = obse->GetPluginHandle();

	if(!obse->isEditor)
	{
        // register serialization interface callbacks
		g_serializationIntfc->SetSaveCallback(g_pluginHandle, SaveCallback);
		g_serializationIntfc->SetLoadCallback(g_pluginHandle, LoadCallback);
		g_serializationIntfc->SetNewGameCallback(g_pluginHandle, NewGameCallback);
		g_serializationIntfc->SetPreloadCallback(g_pluginHandle, PreloadCallback);

		// get string variable interface
		g_stringIntfc = (OBSEStringVarInterface*)obse->QueryInterface(kInterface_StringVar);
		g_stringIntfc->Register(g_stringIntfc); // DEPRECATED: use RegisterStringVarInterface() in GameAPI.h
	}

	// get messaging interface, register message listeners
	g_messagingIntfc = (OBSEMessagingInterface*)obse->QueryInterface(kInterface_Messaging);
	g_messagingIntfc->RegisterListener(g_pluginHandle, "OBSE", OBSEMessageHandler); // register to recieve messages from OBSE          
    g_messagingIntfc->RegisterListener(g_pluginHandle, NULL, GeneralMessageHandler); // register general message listener

	// get command table interface
	g_cmdIntfc = (OBSECommandTableInterface*)obse->QueryInterface(kInterface_CommandTable);

    // initialize submodule
    FARPROC pQuerySubmodule = GetProcAddress(g_hSubmodule,"Initialize"); // get pointer to Initialize() method exported by submodule
    if (pQuerySubmodule) 
    {
        g_submoduleInfc = (SubmoduleInterface*)pQuerySubmodule();   // call Submodule::Initialize() method, returns submodule interface
        if (!g_submoduleInfc) 
        {
            _ERROR("Submodule returned invalid interface");
        }
        _DMESSAGE("Submodule interface found at <%p>",g_submoduleInfc);
    }
    else _ERROR("Could not initialize submodule.");

    // Register script commands
    Register_Commands();

	return true;
}

/*--------------------------------------------------------------------------------------------*/
// Windows dll load
extern "C" BOOL WINAPI DllMain(HANDLE  hDllHandle, DWORD dwReason, LPVOID  lpreserved)
{// called when plugin is loaded into process memory, before obse takes over
	switch(dwReason)
    {
    case DLL_PROCESS_DETACH:    // dll unloaded 
        // delete dynamically allocated log file target
        if (_gLogFile)
        {
            gLog.DetachTarget(*_gLogFile);
            delete _gLogFile;
        }
        break;
    }   
	return true;
}