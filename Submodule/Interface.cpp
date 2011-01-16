#include "Submodule/Interface.h"
#include "Submodule/Version.h"
#include "API/TESForms/TESObjectREFR.h"
#include "API/BSTypes/BSStringT.h"

void SubmoduleInterface::COEFTest(TESObjectREFR* thisObj, const char* argA, const char* argB, const char* argC)
{// this is a stub, just to demonstrate the concept of a general interface function
    BSStringT desc;
    if (thisObj) thisObj->GetDebugDescription(desc);
    _DMESSAGE("Test command on <%p> '%s' w/ args A='%s' B='%s' C='%s'",thisObj,desc.c_str(),argA,argB,argC);
}
const char* SubmoduleInterface::Description()
{
    static char buffer[0x100];
    sprintf_s(buffer,sizeof(buffer), SOLUTIONNAME ", v%i.%i beta%i", MAJOR_VERSION,MINOR_VERSION,BETA_VERSION);
    return buffer;
}

