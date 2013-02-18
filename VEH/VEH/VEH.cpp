#include <windows.h>
#include "internal.h"
#include "VEH.h"

LdrpVectorHandler LdrpVectorHandlerList[2];

PVOID WINAPI
RtlpAddVectoredHandler(ULONG FirstHandler,
                       PVECTORED_EXCEPTION_HANDLER VectorHandler,
                       ULONG Type)
{
    PVOID Peb = NtCurrentPeb();
    PVEH_NODE VehNode = NULL;

    VehNode = (PVEH_NODE)fnRtlAllocateHeap(*(PVOID*)((PBYTE)Peb + 0x18),      // Peb.ProcessHeap
                                           0,                                 // No flags
                                           sizeof(VEH_NODE));                 // 0x10 bytes
    if (VehNode == NULL) {
        return NULL;
    }

    VehNode->RefCount = 1;
    VehNode->Handler  = (PVECTORED_EXCEPTION_HANDLER)fnRtlEncodePointer(VectorHandler);
    fnRtlAcquireSRWLockExclusive(&LdrpVectorHandlerList[Type].Lock);

    if (IsListEmpty(&LdrpVectorHandlerList[Type].Head)) {
        InterlockedBitTestAndSet((LONG*)((PBYTE)Peb+0x28),       // Peb.EnvironmentUpdateCount, seems not a count...
                                 Type + 2);
    }

    if (FirstHandler == 0) {
        InsertHeadList(&LdrpVectorHandlerList[Type].Head, &VehNode->Entry);
    }
    else {
        InsertTailList(&LdrpVectorHandlerList[Type].Head, &VehNode->Entry);
    }

    fnRtlReleaseSRWLockExclusive(&LdrpVectorHandlerList[Type].Lock);

    return VehNode;
}

BOOL WINAPI
RtlpRemoveVectoredHandler(PVOID Handle,
                          ULONG Type)
{
    LdrpVectorHandler* HandlerList = &LdrpVectorHandlerList[Type];
    VEH_NODE *CurrentNode = (VEH_NODE*)HandlerList->Head.Flink;

    fnRtlAcquireSRWLockExclusive(&HandlerList->Lock);

    while (CurrentNode != (VEH_NODE*)&HandlerList->Head) {
        if ((PVOID)CurrentNode != Handle) {
            CurrentNode = (VEH_NODE*)CurrentNode->Entry.Flink;
            continue;
        }

        // find the entry.
        CurrentNode->RefCount--;
        if (CurrentNode->RefCount == 0) {
            PVOID Peb = NtCurrentPeb();
            if (RemoveEntryList(&CurrentNode->Entry)) {
                // list empty
                InterlockedBitTestAndReset((LONG*)((PBYTE)Peb+0x28),       // Peb.EnvironmentUpdateCount
                                           Type + 2);
            }

            fnRtlReleaseSRWLockExclusive(&HandlerList->Head);
            fnRtlFreeHeap(*(PVOID*)((PBYTE)Peb + 0x18),
                           0,
                           CurrentNode);
        }
        else {
            fnRtlReleaseSRWLockExclusive(&HandlerList->Head);
        }

        return TRUE;
    }

    fnRtlReleaseSRWLockExclusive(&HandlerList->Head);
    return FALSE;
}

BOOL WINAPI
_RtlpCallVectoredHandler(PEXCEPTION_RECORD ExceptionRecord,
                         PCONTEXT          ContextRecord,
                         ULONG             Type)
{
    PVOID Peb = NtCurrentPeb();
    ULONG *pEnvironmentUpdateCount = (ULONG*)((PBYTE)Peb+0x28);
    ULONG typeMask = 1 << (Type + 2);
    BOOL  Handled = FALSE;
    LIST_ENTRY *needDeleteList  = NULL;
    EXCEPTION_POINTERS exceptionPointers;
    LdrpVectorHandler  *vectorHandler = &LdrpVectorHandlerList[Type];
    
    if (((*pEnvironmentUpdateCount) & typeMask) == 0) {
        return Handled;
    }

    needDeleteList = NULL;
    exceptionPointers.ExceptionRecord = ExceptionRecord;
    exceptionPointers.ContextRecord   = ContextRecord;
    fnRtlAcquireSRWLockExclusive(&vectorHandler->Lock);

    VEH_NODE* currentNode = (VEH_NODE*)vectorHandler->Head.Flink;
    while(&currentNode->Entry != &vectorHandler->Head) {
        DWORD handlerResult;
        VEH_NODE* oldNode = currentNode;
        currentNode = (VEH_NODE*)currentNode->Entry.Flink;

        oldNode->RefCount++;

        fnRtlReleaseSRWLockExclusive(&vectorHandler->Lock);
        PVECTORED_EXCEPTION_HANDLER handler = (PVECTORED_EXCEPTION_HANDLER)fnRtlDecodePointer(oldNode->Handler);

        handlerResult = handler(&exceptionPointers);

        fnRtlAcquireSRWLockExclusive(&vectorHandler->Lock);
        oldNode->RefCount--;

        if (oldNode->RefCount == 0) {
            if (RemoveEntryList(&oldNode->Entry)) {
                // list empty
                InterlockedBitTestAndReset((volatile LONG *)pEnvironmentUpdateCount,
                                           Type + 2);
            }

            oldNode->Entry.Flink = needDeleteList;
            needDeleteList = &oldNode->Entry;
        }

        if (handlerResult == EXCEPTION_CONTINUE_EXECUTION) {
            Handled = TRUE;
            break;
        }
    }

    fnRtlReleaseSRWLockExclusive(&vectorHandler->Lock);

    while(needDeleteList) {
        LIST_ENTRY* next = needDeleteList->Flink;
        fnRtlFreeHeap(*(PVOID*)((PBYTE)Peb + 0x18),
                      0,
                      needDeleteList);

        needDeleteList = next;
    }

    return Handled;
}

PVOID WINAPI 
crAddVectoredExceptionHandler(ULONG FirstHandler,
                              PVECTORED_EXCEPTION_HANDLER VectorHandler)
{
    return RtlpAddVectoredHandler(FirstHandler, VectorHandler, 0);
}

ULONG WINAPI 
crRemoveVectoredExceptionHandler(PVOID Handler)
{
    return RtlpRemoveVectoredHandler(Handler, 0);
}

PVOID WINAPI 
crAddVectoredContinueHandler(ULONG FirstHandler,
                            PVECTORED_EXCEPTION_HANDLER VectorHandler)
{
    return RtlpAddVectoredHandler(FirstHandler, VectorHandler, 1);
}

ULONG WINAPI 
crRemoveVectoredContinueHandler(PVOID Handler)
{
    return RtlpRemoveVectoredHandler(Handler, 1);
}

