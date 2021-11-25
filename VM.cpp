#include "VM.h"


VM::VM()
{
    this->VMProcessId = 0;
    this->VMProcessHandle = INVALID_HANDLE_VALUE;
    this->VMPipeHandle = INVALID_HANDLE_VALUE;
    this->SuccessfullyInitialized = FALSE;
}

VM::~VM()
{
    if (this->VMProcessHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(this->VMProcessHandle);
    }

    if (this->VMPipeHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(this->VMPipeHandle);
    }
}

WHPHOOK_ERROR VM::Initialize(_Out_ PHANDLE Pipe)
{
    WHPHOOK_ERROR err;
    ULONG         ulProcessId;
    HANDLE        hPipe;
    HANDLE        hProcess;


    err = ErrorUnknown;
    hPipe = CreateFileA("\\\\.\\pipe\\whp_hook", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        err = ErrorBadPipe;
        *(HANDLE*)Pipe = INVALID_HANDLE_VALUE;
        return(err);
    }

    this->VMPipeHandle = hPipe;
    *(HANDLE*)Pipe = hPipe;

    if (!GetNamedPipeServerProcessId(this->VMPipeHandle, &ulProcessId))
    {
        err = ErrorNoPipePID;
        return(err);
    }

    this->VMProcessId = ulProcessId;

    hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, this->VMProcessId);
    if (hProcess == INVALID_HANDLE_VALUE)
    {
        err = ErrorUnableToOpenProcess;
        return(err);
    }

    this->VMProcessHandle = hProcess;
    err = ErrorSuccess;
    return(err);
}
