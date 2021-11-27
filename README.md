# WHPHook
Simple DLL that hooks all the functions in WinHvPlatform.dll for the purpose of logging and or gaining introspection at the hypervisor level.

## Overview and Usage

The general idea of this DLL is replace the normal WinHvPlatform.dll [Windows Hypervisor Platform](https://docs.microsoft.com/en-us/virtualization/api/hypervisor-platform/hypervisor-platform) (WHP) with our custom one in order to log or manilupate data between VM exits. 
One way to do this would be to repalce the DLL in `C:\Windows\System32` <br> Another method (and the preferred) is to inject it into a process by creating the process which uses WinHvPlatform.dll with the `CREATE_SUSPENDED` flag. Load the "fake" DLL via `CreateRemoteThread` on `LoadLibrary` and then resume the process. Because our DLL is loaded first, its exported functions will be the ones called by the process which will remain oblivious of the switch. The loaded "hooking" DLL then loads the real WinHvPlatform.dll so it can correctly act as a middleman.

On load the DLL creates a duplex pipe (`\\.\pipe\whp_hook`) and starts a server thread. A client can connect and then interact with the DLL essentially allowing it to debug the guest OS supported by hypervisor platform. 

For testing and this readme, I use QEMU because of its general purpose nature, but this would work for other programs such as VMware products when running on top of Hyper-V and therefore forced to use the WHP API.

Currently three extreamly basic commands:
* `info` : Get the number of vCPUs and the partition handle value
* `vtop` : Translate a virtual address to a physical address. This uses [WHvTranslateGva](https://docs.microsoft.com/en-us/virtualization/api/hypervisor-platform/funcs/whvtranslategva)
* `dump` : Get the general purpose registers of a specified vCPU 

## Example
This image shows QEMU running DOS with the Windows Hypervisor Platform accelerator: 
<p align="center">
  <img src="https://user-images.githubusercontent.com/16905064/142759492-db23b7d6-b403-46e7-beba-9a4843831f34.PNG"/>
 </p>
This image shows the 'dump' command:<br>
<p align="center">
  <img src="https://user-images.githubusercontent.com/16905064/142771504-dea0aee4-8595-448f-9390-8c5f212d82da.PNG"/>
</p>
This is an example of what the log file will show:<br>
<p align="center">
  <img src="https://user-images.githubusercontent.com/16905064/142778064-7dbf98f7-4386-4ffe-b09a-a42143ec7b09.PNG"/>
</p>

## Building
* ⚠ In Launcher.c you may need to change some of the hardcoded QEMU paths as well as the path (G_Dll) the the hooking WinHvPlatform.dll
* ℹ️ Run the build.bat script the to build all the binaries

## Notes
* Default (hardcoded) log file location is `C:\Windows\Temp\whp_hook_log.txt` This can totally be disabled by commenting out the `DEBUG` define.
* It's important to note that this doesn't include/hook the WHP functions from the WinHvEmulation.dll, though an identical method could be used to hook its functions as well.
* Currently just works with QEMU's usage of the WHP, but should work with VMware products and others that utilize the WHP.
* Just an experiment/PoC/toy. There are so many ways to improve this 🧐
