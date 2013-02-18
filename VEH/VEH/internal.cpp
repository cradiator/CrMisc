#include <windows.h>
#include "internal.h"

PVOID (WINAPI *fnRtlAllocateHeap)(IN PVOID  HeapHandle,
                                  IN ULONG  Flags,
                                  IN SIZE_T Size);

BOOL (WINAPI *fnRtlFreeHeap)(IN PVOID  HeapHandle,
                             IN ULONG  Flags,
                             IN PVOID  HeapBase);

PVOID (WINAPI *fnRtlEncodePointer)(IN PVOID Pointer);

PVOID (WINAPI *fnRtlDecodePointer)(IN PVOID Pointer);

VOID  (WINAPI *fnRtlAcquireSRWLockExclusive)(IN PVOID Lock);

VOID  (WINAPI *fnRtlInitializeSRWLock)(IN PVOID Lock);

VOID  (WINAPI *fnRtlReleaseSRWLockExclusive)(IN PVOID Lock);

BOOL InitializeInternal()
{
    HMODULE hMod = NULL;

    hMod = GetModuleHandleW(L"ntdll.dll");
    *((PVOID*)(&fnRtlAllocateHeap)) = (PVOID)GetProcAddress(hMod, "RtlAllocateHeap");
    if (fnRtlAllocateHeap == NULL)
        return FALSE;

    *((PVOID*)(&fnRtlFreeHeap)) = (PVOID)GetProcAddress(hMod, "RtlFreeHeap");
    if (fnRtlFreeHeap == NULL)
        return FALSE;

    *((PVOID*)(&fnRtlEncodePointer)) = (PVOID)GetProcAddress(hMod, "RtlEncodePointer");
    if (fnRtlEncodePointer == NULL)
        return FALSE;

    *((PVOID*)(&fnRtlDecodePointer)) = (PVOID)GetProcAddress(hMod, "RtlDecodePointer");
    if (fnRtlDecodePointer == NULL)
        return FALSE;

    *((PVOID*)(&fnRtlInitializeSRWLock)) = (PVOID)GetProcAddress(hMod, "RtlInitializeSRWLock");
    if (fnRtlInitializeSRWLock == NULL)
        return FALSE;

    *((PVOID*)(&fnRtlAcquireSRWLockExclusive)) = (PVOID)GetProcAddress(hMod, "RtlAcquireSRWLockExclusive");
    if (fnRtlAcquireSRWLockExclusive == NULL)
        return FALSE;

    *((PVOID*)(&fnRtlReleaseSRWLockExclusive)) = (PVOID)GetProcAddress(hMod, "RtlReleaseSRWLockExclusive");
    if (fnRtlReleaseSRWLockExclusive == NULL)
        return FALSE;

    return TRUE;
}

