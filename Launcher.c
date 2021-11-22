#include <Windows.h>
#include <stdio.h>

PCWSTR G_Dll = L"C:\\Something\\WinHvPlatform.dll"; // WHPHook dll

void main()
{
    PROCESS_INFORMATION procInfo;
    STARTUPINFO         startInfo;
    LPVOID              lpRemote

    ZeroMemory(&startInfo, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);
    CreateProcess(L"C:\\Program Files\\qemu\\qemu-system-x86_64.exe", 
        L"qemu-system-x86_64.exe -fda \"C:\\Users\\User\\Downloads\\Dos6.22.img\" --boot a -accel whpx -m 2G",
        NULL, NULL,
        FALSE, CREATE_SUSPENDED, NULL, L"C:\\Program Files\\qemu\\", &startInfo, &procInfo);
		
	CloseHandle(procInfo.hThread);
    lpRemote = VirtualAllocEx(procInfo.hProcess, 0, 0x1000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	if (lpRemote)
	{
        WriteProcessMemory(procInfo.hProcess, lpA, G_Dll, lstrlenW(G_Dll) * 2, NULL);
        CreateRemoteThread(procInfo.hProcess, NULL, 0, &LoadLibraryW, lpA, 0, NULL);
        Sleep(1000); // Sleep to ensure the DLL is loaded before destorying the path's memory
        VirtualFreeEx(procInfo.hProcess, lpA, 0, MEM_RELEASE);
	}
}
