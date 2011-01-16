/*
    Commands header for loader
    Interface between loader code and command execution code
*/
#pragma once

#include "obse/PluginAPI.h"
#include "Submodule/Interface.h"

// global OBSE interface objects
extern PluginHandle                 g_pluginHandle;
extern const OBSEInterface*         g_obseIntfc;
extern OBSEArrayVarInterface*       g_arrayIntfc;
extern OBSEStringVarInterface*      g_stringIntfc;
extern OBSEScriptInterface*         g_scriptIntfc;
extern OBSECommandTableInterface*   g_cmdIntfc;

// global submodule interface
extern SubmoduleInterface*        g_submoduleInfc;

// opcodes - this is not an officially assigned base, use for testing purposes only!
const int OPCODEBASE        = 0x5000;

// registers new script commands
void Register_Commands();

// parses CSE console commands
void CSEPrintCallback(const char* Message, const char* Prefix);
