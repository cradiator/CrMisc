#include <windows.h>
#include <Winternl.h>
#include <process.h>        // import __security_check_cookie

#define _EXCEPTION_DISPATCHER_API __declspec(dllexport)
#include "ExceptionDispatcher.h"

VOID __cdecl 
__local_unwind4(DWORD* CookiePointer, PVOID EstablishFrame , DWORD EnclosingLevel);

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
        mov  eax, RtlUnwind
        call eax
    }

__ReturnPoint_0:
    return;
}

// If an exception happen as doing a local unwind, this function is the nested exception handler.
// The call convention is __cdecl, but in fact it could be __stdcall too.
// The EstablishFrame would be
//                   ebp
//                   CookiePointer
//                   PrevEstablishFrame
//                   EnclosingLevel
//                   Cookie
//                   _unwind_handler4
//                   next
EXCEPTION_DISPOSITION __cdecl
_unwind_handler4(PEXCEPTION_RECORD ExceptionRecord,
                 PVOID             EstablishFrame,
                 PCONTEXT          ContextRecord,
                 PVOID             DispatchContext)
{
    if ( (ExceptionRecord->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND)) == 0) {
        return ExceptionContinueSearch;
    }

    // first check is the establishframe cookie is overwritten.
    DWORD cookie = *(DWORD*)((DWORD)EstablishFrame + 8);
    cookie ^= (DWORD)EstablishFrame;
    __security_check_cookie(cookie);

    __asm {
        mov  eax, EstablishFrame
        push ebp
        mov  ebp, eax
        push [eax+0x0c]      // EstablishFrame.EnclosingLevel
        push [eax+0x10]     // EstablishFrame.PrevEstablishFrame
        push [eax+0x14]     // EstablishFrame.CookiePointer
        call __local_unwind4
        add  esp, 0x0c
        pop  ebp
    }

    *(DWORD*)DispatchContext = (DWORD)EstablishFrame;
    return ExceptionCollidedUnwind;
}

// Do the real local unwind work.
// This function is protected by a special seh node. I have no idea how to write it in C language.
// Is there anyway I can write this function in C ??
__declspec(naked) VOID __cdecl 
__local_unwind4(DWORD* CookiePointer, PVOID EstablishFrame , DWORD TargetEnclosingLevel)
{
    __asm {
        push ebx
        push esi
        push edi
        mov  edx, [esp+0x10]        // edx = CookiePointer
        mov  eax, [esp+0x14]        // eax = EstablishFrame
        mov  ecx, [esp+0x18]        // ecx = EnclosingLevel

        // establish new EstablishFrame for the protected seh node.
        push ebp                     // context frame
        push edx                     // CookiePointer
        push eax                     // PrevEstablishFrame
        push ecx                     // EnclosingLevel
        push ecx                     // Cookie
        push _unwind_handler4        // handler
        push fs:[0]                  // next

        mov  eax, __security_cookie  // edit the Cookie above to real value
        xor  eax, esp
        mov  [esp+0x08], eax         

        // complete establish the seh.
        mov  fs:[0], esp

_lu_top:
        mov  eax, [esp+0x30]        // EstablishFrame
        mov  ebx, [eax+8]           // EstablishFrame.EH4_SCOPETABLE
        mov  ecx, [esp+0x2c]        // CookiePointer
        xor  ebx, [ecx]             // ebx = decoded EH4_SCOPETABLE
        mov  esi, [eax+0x0c]        // eax = EstablishFrame.EnclosingLevel
        cmp  esi, END_POS
        jz   _lu_done               // meet the last level. exit.

        mov  edx, [esp+0x34]        // edx = EnclosingLevel
        cmp  edx, END_POS
        jz   __update_establishframe

        cmp  esi, edx               // EstablishFrame.EnclosingLevel <= EnclosingLevel
        jbe  _lu_done
    
__update_establishframe:
        // Update the EstablishFrame.EnlosingLevel to EnclosingLevel
        lea  esi, [esi+esi*2]
        lea  ebx, [ebx+esi*4+0x10]      // ebx = &EH4_SCOPETABLE.ScopeRecord[esi]
        mov  ecx, [ebx]                 // ecx = EH4_SCOPETABLE.ScopeRecord[esi].EnclosingLevel
        mov  [eax+0x0c], ecx            // EstablishFrame.EnclosingLevel = EH4_SCOPETABLE.ScopeRecord[esi].EnclosingLevel
        cmp  [ebx+4], 0                 // EH4_SCOPETABLE.ScopeRecord[esi].FilterFunc != NULL ?
        jnz  _lu_top

//        push 0x101
//        mov  eax, [ebx+8]               // eax = EH4_SCOPETABLE.ScopeRecord[esi].HandlerFunc
//       call __NLG_Notify

        mov  ecx, 1
        mov  eax, [ebx+8]
//        call __NLG_Call
        call eax

        jmp  _lu_top

_lu_done:
        pop  ebx
        mov  fs:[0], ebx
        add  esp, 0x18
        pop  edi
        pop  esi
        pop  ebx
        retn
    }
}


// Call __local_unwind4 in the context "ContextFrame"
// Because we have to modify ebp, we can not use C language.
__declspec(naked) VOID __fastcall
_EH4_LocalUnwind(PVOID EstablishFrame, 
                 DWORD TargetEnclosingLevel, 
                 PVOID ContextFrame, 
                 DWORD *CookiePointer)
{
    // mov ebp, ContextFrame;
    // __local_unwind4(CookiePointer, EstablishFrame, EnclosingLevel);
    __asm {
        push ebp
        mov  ebp, [esp+8]       // mov ebp, ContextFrame
        push edx                // push TargetEnclosingLevel
        push ecx                // push EstablishFrame
        push [esp+0x14]         // push CookiePointer
        call __local_unwind4
        add  esp, 0xc
        pop  ebp
        retn 8
    }
}

__declspec(naked) VOID __fastcall
_EH4_TransferToHandler(PVOID HandlerFunc, PVOID ContextFrame)
{
    __asm {
        mov  ebp, edx       // ContextFrame
        mov  esi, ecx       // HandlerFunc
        mov  eax, ecx

        // clear up all register and jump to the handler
        xor  eax, eax
        xor  ebx, ebx
        xor  ecx, ecx
        xor  edx, edx
        xor  edi, edi
        jmp  esi
    }
}


_EXCEPTION_DISPATCHER_API EXCEPTION_DISPOSITION __cdecl
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
    if (DecodeScopeTable->GsCookieOffset != END_POS) {
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

                *(DWORD*)(((DWORD)EstablishFrame) + 0x0c) = NextEnclosingLevel;
                
                // Check EHCookie and GSCookie again.
                if (DecodeScopeTable->GsCookieOffset != END_POS) {
                    DWORD GSCookie = *(DWORD*)(ContextFrame + DecodeScopeTable->GsCookieOffset);
                    GSCookie ^=  (DWORD)ContextFrame + DecodeScopeTable->GsCookieXOROffset;
                    CookieCheckFunction(GSCookie);
                }

                DWORD EHCookie = *(DWORD*)(ContextFrame + DecodeScopeTable->EHCookieOffset);
                EHCookie ^= (DWORD)ContextFrame + DecodeScopeTable->EHCookieXOROffset;
                CookieCheckFunction(EHCookie);

                // This function would not return.
                _EH4_TransferToHandler(CurrentScopeTableRecord->HandlerFunc, ContextFrame);
                break;
            }

            // loop back, continue to deal with outer scope table record.
            CurrentEnclosingLevel  = NextEnclosingLevel;
        }
    } // end of if ((ExceptionRecord->ExceptionFlags & EXCEPTION_UNWIND) == 0)

    if (EstablishFrame->EnclosingLevel != END_POS) {
        _EH4_LocalUnwind(EstablishFrame, EstablishFrame->EnclosingLevel, ContextFrame, CookiePointer);
        Revalidate = TRUE;
    }

__EXIT:
    if (Revalidate) {
        // Check EHCookie and GSCookie again.
        if (DecodeScopeTable->GsCookieOffset != END_POS) {
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

