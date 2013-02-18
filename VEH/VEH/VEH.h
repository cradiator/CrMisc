#pragma once

typedef struct _VEH_NODE {
    LIST_ENTRY  Entry;
    ULONG       RefCount;
    PVECTORED_EXCEPTION_HANDLER Handler;
} VEH_NODE, *PVEH_NODE;

typedef struct _LdrpVectorHandler {
    PVOID       Lock;
    LIST_ENTRY  Head;
} LdrpVectorHandler;


