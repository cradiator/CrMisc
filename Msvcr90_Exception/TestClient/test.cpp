#include <windows.h>
#include <stdio.h>
#include "../msvcr90_Exception/ExceptionDispatcher.h"

DWORD g_code;

DWORD Filter(DWORD code, EXCEPTION_POINTERS *ep) {
    printf("Filter, ExceptionCode: 0x%X\n", code);
    g_code = code;
    return EXCEPTION_EXECUTE_HANDLER;
}

void foo() {
    __try {
        printf("foo -- try\n");
        
        __try {
            printf("foo -- try -- try\n");
            *(DWORD*)0 = 0;
        }
        __except(EXCEPTION_CONTINUE_SEARCH) {
            printf("foo -- try -- except\n");
        }
    }
    __except(Filter(GetExceptionCode(), GetExceptionInformation())) {
        printf("foo -- except\n");
    }

    __try {
        printf("foo -- try2\n");
        if (g_code == 0xc0000005) {
            printf("leaving.\n");
            __leave;
        }
        printf("after leave\n");
    }
    __finally {
        printf("foo -- finally2\n");
    }
}

int main(int argc, char** argv) {
    HookExceptionRoutine();

    __try {
        printf("main -- try\n");
        foo();
    }
    __except(Filter(GetExceptionCode(), GetExceptionInformation())) {
        printf("main -- except\n");
    }

    printf("done.\n");
    return 0;
}

