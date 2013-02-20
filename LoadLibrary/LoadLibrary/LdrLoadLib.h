#pragma once


NTSTATUS WINAPI
crLdrEnumerateLoadedModules(IN BOOLEAN            ReservedFlag,
                            IN PLDR_ENUM_CALLBACK EnumProc,
                            IN PVOID              Context);
