#pragma once

#ifndef _EXCEPTION_DISPATCHER_API
#define _EXCEPTION_DISPATCHER_API __declspec(dllimport)
#endif

#define END_POS            0xFFFFFFFE
#define CPP_EXCEPTION_CODE 0x0E06D7363

/// in wrk
// begin_ntddk begin_wdm
//
// Exception flag definitions.
//

// begin_winnt
#define EXCEPTION_NONCONTINUABLE 0x1    // Noncontinuable exception
// end_winnt

// end_ntddk end_wdm
#define EXCEPTION_UNWINDING 0x2         // Unwind is in progress
#define EXCEPTION_EXIT_UNWIND 0x4       // Exit unwind is in progress
#define EXCEPTION_STACK_INVALID 0x8     // Stack out of limits or unaligned
#define EXCEPTION_NESTED_CALL 0x10      // Nested exception handler call
#define EXCEPTION_TARGET_UNWIND 0x20    // Target unwind in progress
#define EXCEPTION_COLLIDED_UNWIND 0x40  // Collided exception handler call

#define EXCEPTION_UNWIND (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND | \
    EXCEPTION_TARGET_UNWIND | EXCEPTION_COLLIDED_UNWIND)

/// The exe file that use msvcr90 __try/__except realize would this function.
/// Used by _except_handler4_common to check cookie.
typedef void (__fastcall *PCOOKIE_CHECK_ROUTINE)(DWORD cookie);

/// When we use 
/// __try {...} 
/// __except (filter-expression) {...}
/// The "filter-expression" in __except generate this function.
/// This function would be called in _except_handler4_common but execute in the context where the exception happened.
/// 
/// return EXCEPTION_EXECUTE_HANDLER
///        EXCEPTION_CONTINUE_SEARCH
///        EXCEPTION_CONTINUE_EXECUTION
typedef DWORD (__cdecl *PEXCEPTION_FILTER)();

typedef struct _EXCEPTION_REGISTRATION_RECORD {
    struct _EXCEPTION_REGISTRATION_RECORD *Next;
    PEXCEPTION_ROUTINE Handler;
} EXCEPTION_REGISTRATION_RECORD;

typedef struct _EH4_SCOPETABLE_RECORD {
    DWORD             EnclosingLevel;
    PEXCEPTION_FILTER FilterFunc;
    PVOID             HandlerFunc;      // We would jmp into it. The prototype has no arguments and return value.
} EH4_SCOPETABLE_RECORD;

typedef struct _EH4_SCOPETABLE {
    DWORD   GsCookieOffset;
    DWORD   GsCookieXOROffset;
    DWORD   EHCookieOffset;
    DWORD   EHCookieXOROffset;
    EH4_SCOPETABLE_RECORD ScopeRecord[1];   // Variable length array.
                                            // One __try/__except or __try/__finally pair have one EH4_SCOPETABLE_RECORD.
} EH4_SCOPETABLE;

typedef struct _Msvcr90ExceptionRegistrationRecord {
    // 
    // EXCEPTION_POINTERS         ExceptionPointers; 
    EXCEPTION_REGISTRATION_RECORD Record;
    EH4_SCOPETABLE* ScopeTable;
    DWORD           EnclosingLevel;
    PVOID           SavedEbp;           // &SavedEbp is the context frame when exception happened.
} Msvcr90ExceptionRegistrationRecord;


_EXCEPTION_DISPATCHER_API EXCEPTION_DISPOSITION __cdecl
cr_except_handler4_common(DWORD                 *CookiePointer, 
                          PCOOKIE_CHECK_ROUTINE CookieCheckFunction,
                          EXCEPTION_RECORD      *ExceptionRecord,
                          Msvcr90ExceptionRegistrationRecord *EstablishFrame,
                          CONTEXT               *ContextRecord,
                          PVOID                 *DispatcherContext);



// It is the export function used by test client.
_EXCEPTION_DISPATCHER_API bool WINAPI HookExceptionRoutine();

