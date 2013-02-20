#include <windows.h>
#include <stdio.h>
#include "../../CommonLib/WinInter.h"

VOID WINAPI ModuleEnumCallback(IN  PLDR_DATA_TABLE_ENTRY pEntry,
                               IN  PVOID                 Context,
                               OUT BOOLEAN*              Stop)
{
    printf("Name: %-20S\tBase: 0x%8.8x\n", pEntry->BaseDllName.Buffer, pEntry->DllBase);
    *Stop = FALSE;
}

int main(int argc, char** argv) {
    LdrEnumerateLoadedModules(FALSE,
                              ModuleEnumCallback,
                              NULL);
    return 0;
}