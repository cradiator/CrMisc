#pragma once

extern PVOID (WINAPI *fnRtlAllocateHeap)(IN PVOID  HeapHandle,
                                         IN ULONG  Flags,
                                         IN SIZE_T Size);

extern BOOL (WINAPI *fnRtlFreeHeap)(IN PVOID  HeapHandle,
                                    IN ULONG  Flags,
                                    IN PVOID  HeapBase); 

extern PVOID (WINAPI *fnRtlEncodePointer)(IN PVOID Pointer);

extern PVOID (WINAPI *fnRtlDecodePointer)(IN PVOID Pointer);

extern VOID  (WINAPI *fnRtlAcquireSRWLockExclusive)(IN PVOID Lock);

extern VOID  (WINAPI *fnRtlInitializeSRWLock)(IN PVOID Lock);

extern VOID  (WINAPI *fnRtlReleaseSRWLockExclusive)(IN PVOID Lock);

BOOL InitializeInternal();

__declspec(naked) inline PVOID NtCurrentPeb() {
    __asm {
        mov eax, fs:[0x18];     // eax == Teb.NtTib.Self
        mov eax, [eax+0x30];    // eax == Teb.Peb
        ret;
    }
}

inline BOOL IsListEmpty(LIST_ENTRY *Head) {
    return Head == Head->Flink;
}

inline VOID InsertHeadList(IN PLIST_ENTRY ListHead,
                           IN PLIST_ENTRY Entry)
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}

inline VOID InsertTailList(IN PLIST_ENTRY ListHead,
                           IN PLIST_ENTRY Entry)
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}

inline BOOL RemoveEntryList(IN PLIST_ENTRY Entry)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOLEAN)(Flink == Blink);
}

