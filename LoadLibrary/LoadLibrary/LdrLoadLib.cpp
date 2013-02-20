#include <Windows.h>
#include "../../CommonLib/WinInter.h"
#include "LdrLoadLib.h"

BOOLEAN              LdrpInLdrInit;
RTL_CRITICAL_SECTION LdrpLoaderLock;
PEB_LDR_DATA         PebLdr;


// 
NTSTATUS WINAPI
crLdrEnumerateLoadedModules(IN BOOLEAN            ReservedFlag,
                            IN PLDR_ENUM_CALLBACK EnumProc,
                            IN PVOID              Context)
{
    if (ReservedFlag || EnumProc == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (LdrpInLdrInit) {
        RtlEnterCriticalSection(&LdrpLoaderLock);
    }

    LIST_ENTRY *currentNode = PebLdr.InLoadOrderModuleList.Flink;
    BOOLEAN stop = FALSE;
    __try {
        while(currentNode != &PebLdr.InLoadOrderModuleList) {
            LDR_DATA_TABLE_ENTRY* entry = CONTAINING_RECORD(currentNode, LDR_DATA_TABLE_ENTRY, InLoadOrderLinks);
            EnumProc(entry, Context, &stop);
            if (stop) {
                break;
            }
            currentNode = currentNode->Flink;
        }
    }
    __finally {
        if (LdrpInLdrInit) {
            RtlLeaveCriticalSection(&LdrpLoaderLock);
        }        
    }

    return STATUS_SUCCESS;
}

