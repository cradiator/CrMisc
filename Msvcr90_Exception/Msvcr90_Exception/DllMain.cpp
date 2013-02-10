#include <windows.h>
#include "detours.h"

#define _EXCEPTION_DISPATCHER_API __declspec(dllexport)
#include "ExceptionDispatcher.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL,
                    DWORD fdwReason,
                    LPVOID lpvReserved
                    )
{
    return TRUE;
}


bool WINAPI HookExceptionRoutine()
{
    static HMODULE hMsvcr90 = NULL;
    static bool bInit = false;

    if (bInit) {
        return true;
    }

    if (hMsvcr90 == NULL) {
        hMsvcr90 = GetModuleHandleW(L"msvcr90.dll");
        if (hMsvcr90 == NULL)
            return false;
    }

    PVOID routine = GetProcAddress(hMsvcr90, "_except_handler4_common");
    if (routine == NULL)
        return false;

    DetourTransactionBegin();
    DetourAttach(&routine, cr_except_handler4_common);
    DetourTransactionCommit();

    return true;
}

