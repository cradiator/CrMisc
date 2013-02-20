#pragma once

#define STATUS_SUCCESS 0

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef const UNICODE_STRING *PCUNICODE_STRING;

typedef struct _LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    ULONG SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    ULONG  Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    // ......
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

typedef struct _PEB_LDR_DATA {
    ULONG   Length;
    BOOLEAN Initialized;
    PVOID   SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

#define NTDLL_API __declspec(dllimport)


// import functions from ntdll
typedef VOID (WINAPI *PLDR_ENUM_CALLBACK)(IN  PLDR_DATA_TABLE_ENTRY pEntry,
                                              IN  PVOID                 Context,
                                              OUT BOOLEAN*              Stop);
extern "C" {

NTDLL_API NTSTATUS WINAPI RtlEnterCriticalSection(IN PRTL_CRITICAL_SECTION CriticalSection);
NTDLL_API NTSTATUS WINAPI RtlLeaveCriticalSection(IN PRTL_CRITICAL_SECTION CriticalSection);

NTDLL_API VOID RtlInitUnicodeString (IN PUNICODE_STRING DestinationString,
                                     IN PCWSTR          SourceString);

NTDLL_API NTSTATUS WINAPI LdrEnumerateLoadedModules(IN BOOLEAN            ReservedFlag,
                                                    IN PLDR_ENUM_CALLBACK EnumProc,
                                                    IN PVOID              Context);
};


// import functions from kernel32
#define KERNEL32_API __declspec(dllimport)
extern "C" {

KERNEL32_API ULONG WINAPI BaseSetLastNtError(NTSTATUS status);

};