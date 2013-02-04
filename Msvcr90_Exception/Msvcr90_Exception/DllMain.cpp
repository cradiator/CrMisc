#include <windows.h>
#include "ExceptionDispatcher.h"

BOOL WINAPI DllMain(HINSTANCE hinstDLL,
                    DWORD fdwReason,
                    LPVOID lpvReserved
                    )
{
    cr_except_handler4_common(NULL, NULL, NULL, NULL, NULL, NULL);
}
