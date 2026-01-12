//- Define the sole extension module object.
//- Use function-local static to avoid non-const global variables and static initialization issues.
namespace {
CAcExtensionModule& ArxToolModule() {
    static CAcExtensionModule module;
    return module;
}
}  // namespace
//- Now you can use the CAcModuleResourceOverride class in
//- your application to switch to the correct resource instance.
//- Please see the ObjectARX Documentation for more details

//-----------------------------------------------------------------------------
//- DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
    //- Remove this if you use lpReserved
    UNREFERENCED_PARAMETER(lpReserved);

    if (dwReason == DLL_PROCESS_ATTACH) {
        _hdllInstance = hInstance;
        ArxToolModule().AttachInstance(hInstance);
        InitAcUiDLL();
    } else if (dwReason == DLL_PROCESS_DETACH) {
        ArxToolModule().DetachInstance();
    }
    return TRUE;
}
