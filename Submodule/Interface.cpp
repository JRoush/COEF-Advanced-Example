#include "Submodule/Interface.h"
#include "Submodule/Version.h"
#include "Submodule/MyForm.h"

void SubmoduleInterface::ListMyForms()
{
    // dumps info on all MyForm objects in the extended data handler to the output log
    // the list is accessed using the FormList() method of the MyForm::extendedForm object
    _MESSAGE("Dumping MyForm List ...");
    gLog.Indent();
    for (BSSimpleList<TESForm*>::Node* node = &MyForm::extendedForm.FormList().firstNode; node && node->data; node = node->next)
    {
        MyForm* myform = (MyForm*)node->data;
        _MESSAGE("MyForm %02X '%s' (%08X): name='%s', icon='%s', weight=%f, value=%i, extraData=%i",
            myform->GetFormType(),myform->GetEditorID(),myform->formID, 
            myform->name.c_str(), myform->texturePath.c_str(), myform->weight, myform->goldValue, myform->extraData);
    }
    gLog.Outdent();
}
void SubmoduleInterface::SetMyFormExtraData(TESForm* form, UInt32 extraData)
{
    MyForm* myform = dynamic_cast<MyForm*>(form);   // typecast to MyForm
    if (!myform) return; // argument was not a MyForm object
    _MESSAGE("SetMyFormExtraData ( %08X, %i -> %i )", myform ? myform->formID : 0, myform->extraData, extraData);
    myform->extraData = extraData;  // set the extraData field on the argument
}
UInt32 SubmoduleInterface::GetMyFormExtraData(TESForm* form)
{
    MyForm* myform = dynamic_cast<MyForm*>(form);   // typecast to MyForm
    if (!myform) return 0; // argument was not a MyForm object
    _MESSAGE("GetMyFormExtraData ( %08X, %i )", myform ? myform->formID : 0, myform->extraData);
    return myform->extraData; // return the extraData field from the argument
}
const char* SubmoduleInterface::Description()
{
    static char buffer[0x100];
    sprintf_s(buffer,sizeof(buffer), SOLUTIONNAME ", v%i.%i beta%i", MAJOR_VERSION,MINOR_VERSION,BETA_VERSION);
    return buffer;
}

