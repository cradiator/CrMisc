#include <windows.h>
#include <Winternl.h>

#define _EXCEPTION_DISPATCHER_API __declspec(dllexport)
#include "ExceptionDispatcher.h"

// This function adjust ebp to the ContextFrame when exception happen.
// Clear all register and call Filter function.
__declspec(naked) DWORD __fastcall 
_EH4_CallFilterFunc(PEXCEPTION_FILTER Filter, 
                    PVOID ContextFrame)
{
    __asm {
        push ebp
        push esi
        push edi
        push ebx
        mov  ebp, edx       // ebp == ContextFrame
        xor  eax, eax
        xor  ebx, ebx
        xor  edx, edx
        xor  esi, esi
        xor  edi, edi
        call ecx            // call Filter
        pop  ebx
        pop  edi
        pop  esi
        pop  ebp
        ret
    }
}

// Call RtlUnwind
VOID __fastcall
_EH4_GlobalUnwind(PVOID TargetFrame)
{
    // same as RtlUnwind(TargetFrame,
    //                   &&__ReturnPoint_0,
    //                   NULL,                 // ExceptionRecord
    //                   NULL);                // ReturnValue
    // But we can not use the operator && to get a label's address in vc++ complier.
    // It's a GCC extension.
    __asm {
        push NULL             // ReturnValue
        push NULL             // ExceptionRecord
        push __ReturnPoint_0  // TargetIP
        push TargetFrame     
        call RtlUnwind
    }

__ReturnPoint_0:
    return;
}

VOID __cdecl __local_unwind4(DWORD* CookiePointer, PVOID EstablishFrame , DWORD EnclosingLevel)
{
    return;
}


// Call __local_unwind4 in the context "ContextFrame"
// Because we have to modify ebp, so we use assemble.
__declspec(naked) VOID __fastcall
_EH4_LocalUnwind(PVOID EstablishFrame, 
                 DWORD EnclosingLevel, 
                 PVOID ContextFrame, 
                 DWORD *CookiePointer)
{
    // mov ebp, ContextFrame;
    // __local_unwind4(CookiePointer, EstablishFrame, EnclosingLevel);
    __asm {
        push ebp
        mov  ebp, [esp+8]       // mov ebp, ContextFrame
        push edx                // push EnclosingLevel
        push ecx                // push EstablishFrame
        push [esp+0x14]         // push CookiePointer
        call __local_unwind4
        add  esp, 0xc
        pop  ebp
        retn 8
    }
}


_EXCEPTION_DISPATCHER_API
EXCEPTION_DISPOSITION __cdecl
cr_except_handler4_common(DWORD                 *CookiePointer, 
                          PCOOKIE_CHECK_ROUTINE CookieCheckFunction,
                          EXCEPTION_RECORD      *ExceptionRecord,
                          Msvcr90ExceptionRegistrationRecord *EstablishFrame,
                          CONTEXT               *ContextRecord,
                          PVOID                 *DispatcherContext)
{
    BOOL  Revalidate;
    DWORD CurrentEnclosingLevel;
    DWORD Dispositon;
    EH4_SCOPETABLE_RECORD *CurrentScopeTableRecord;
    EXCEPTION_POINTERS ExceptionPointers;

    EH4_SCOPETABLE *DecodeScopeTable = (EH4_SCOPETABLE*)((DWORD)EstablishFrame->ScopeTable ^ *CookiePointer);
    PBYTE ContextFrame = (PBYTE)(&(EstablishFrame->SavedEbp));      // The context ebp when exception happen
    Revalidate = FALSE;
    Dispositon = ExceptionContinueSearch;

    // Check is there a stack overflow when exception happened.
    if (DecodeScopeTable->GsCookieOffset & END_POS) {
        DWORD GSCookie = *(DWORD*)(ContextFrame + DecodeScopeTable->GsCookieOffset);
        GSCookie ^=  (DWORD)ContextFrame + DecodeScopeTable->GsCookieXOROffset;
        CookieCheckFunction(GSCookie);
    }

    DWORD EHCookie = *(DWORD*)(ContextFrame + DecodeScopeTable->EHCookieOffset);
    EHCookie ^= (DWORD)ContextFrame + DecodeScopeTable->EHCookieXOROffset;
    CookieCheckFunction(EHCookie);


    // Only deal with the seh without flag EXCEPTION_UNWIND
    if ((ExceptionRecord->ExceptionFlags & EXCEPTION_UNWIND) == 0) {
        // Initialize the ExceptionPointers and save it in EstablishFrame-4.
        // EstablishFrame->ExceptionPointers = ExceptionPointers;
        *(EXCEPTION_POINTERS**)((DWORD)EstablishFrame - 4) = &ExceptionPointers;
        
        ExceptionPointers.ExceptionRecord = ExceptionRecord;
        ExceptionPointers.ContextRecord   = ContextRecord;

        // Get enclosing level when exception happened.
        CurrentEnclosingLevel = EstablishFrame->EnclosingLevel;
        if (CurrentEnclosingLevel == END_POS) {
            goto __EXIT;
        }

        for (;;) {
            CurrentScopeTableRecord = &DecodeScopeTable->ScopeRecord[CurrentEnclosingLevel];
            PEXCEPTION_FILTER  CurrentFilter = CurrentScopeTableRecord->FilterFunc;
            DWORD NextEnclosingLevel = CurrentScopeTableRecord->EnclosingLevel;
            
            if (CurrentFilter == NULL) {
                if (CurrentEnclosingLevel == END_POS) {
                    goto __EXIT;
                }

                continue;
            }

            int result = _EH4_CallFilterFunc(CurrentFilter, ContextFrame);
            Revalidate = TRUE;
            if (result < 0) {
                // EXCEPTION_CONTINUE_EXECUTION
                Dispositon = ExceptionContinueExecution;
                goto __EXIT;
            }
            else if (result == 0) {
                // EXCEPTION_CONTINUE_SEARCH
                if (CurrentEnclosingLevel == END_POS) {
                    goto __EXIT;
                }
            }
            else {
                // EXCEPTION_EXECUTE_HANDLER
                if (ExceptionRecord->ExceptionCode == CPP_EXCEPTION_CODE) {
                    // TODO: how about C++ exception?? 
                }

                _EH4_GlobalUnwind(EstablishFrame);
                if (EstablishFrame->EnclosingLevel != CurrentEnclosingLevel) {
                    _EH4_LocalUnwind(EstablishFrame, CurrentEnclosingLevel, ContextFrame, CookiePointer);
                } 
            }

            // loop back, continue to deal with outer scope table record.
            CurrentEnclosingLevel  = NextEnclosingLevel;
        }
    }

    if (EstablishFrame->EnclosingLevel != END_POS) {
        /////////// TODO: _EH4_LocalUnwind
    }

__EXIT:
    if (Revalidate) {
        // Check EHCookie and GSCookie again.
        if (DecodeScopeTable->GsCookieOffset & END_POS) {
            DWORD GSCookie = *(DWORD*)(ContextFrame + DecodeScopeTable->GsCookieOffset);
            GSCookie ^=  (DWORD)ContextFrame + DecodeScopeTable->GsCookieXOROffset;
            CookieCheckFunction(GSCookie);
        }

        DWORD EHCookie = *(DWORD*)(ContextFrame + DecodeScopeTable->EHCookieOffset);
        EHCookie ^= (DWORD)ContextFrame + DecodeScopeTable->EHCookieXOROffset;
        CookieCheckFunction(EHCookie);

    }

    return (EXCEPTION_DISPOSITION)Dispositon;
}

