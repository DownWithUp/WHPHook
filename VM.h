#pragma once
#include <Windows.h>

typedef enum _WHPHOOK_ERROR
{
    ErrorSuccess,
    ErrorUnknown,
    ErrorBadPipe,
    ErrorNoPipePID,
    ErrorUnableToOpenProcess,
    ErrorNotInitialized,
    ErrorReadProcessMemory
} WHPHOOK_ERROR;

class VM
{
public:
    ULONG	VMProcessId;
    HANDLE	VMProcessHandle;
    HANDLE	VMPipeHandle;
    BOOL	SuccessfullyInitialized;

public:
    VM();
    ~VM();
    WHPHOOK_ERROR Initialize(_Out_ PHANDLE Pipe);
};

