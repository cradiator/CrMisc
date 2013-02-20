#include <windows.h>
#include "../../CommonLib/WinInter.h"

ULONG BasepExeLdrEntry;

/*
VOID WINAPI BasepLocateExeLdrEntry(IN  PLDR_DATA_TABLE_ENTRY pEntry,
                                   IN  PVOID                 Context,
                                   OUT BOOLEAN*              Stop)
{

}

HMODULE WINAPI
crLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
    ULONG          dllCharacteristics;
    UNICODE_STRING uniLibFileName;

    if (lpLibFileName == NULL)
        goto _INVALID_PARAM;

    if (dwFlags & 0x0FFFFE004)
        goto _INVALID_PARAM;

    if (hFile != NULL)
        goto _INVALID_PARAM;

    if ((dwFlags & LOAD_LIBRARY_AS_DATAFILE) != 0 &&
        (dwFlags & LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE) != 0)
        goto _INVALID_PARAM;

    dllCharacteristics = 0;
    if (dwFlags & DONT_RESOLVE_DLL_REFERENCES)
        dllCharacteristics |= IMAGE_FILE_EXECUTABLE_IMAGE;
    if (dwFlags & LOAD_IGNORE_CODE_AUTHZ_LEVEL)
        dllCharacteristics |= IMAGE_FILE_SYSTEM;

    RtlInitUnicodeString(&uniLibFileName, lpLibFileName);
    while(uniLibFileName.Length &&
          uniLibFileName.Buffer[(uniLibFileName.Length >> 1) - 1] == L' ') {
        uniLibFileName.Length -= 2;
    }


    if (BasepExeLdrEntry == 0) {
        LdrEnumerateLoadedModules(FALSE,
                                  BasepLocateExeLdrEntry,
                                  NtCurrentPeb().ImageBaseAddress);     // Peb + 0x08

    }

_INVALID_PARAM:
    BaseSetLastNtError(STATUS_INVALID_PARAMETER);
    return NULL;
}

*/

