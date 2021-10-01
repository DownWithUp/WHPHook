# WHPHook
Simple DLL that hooks all the functions in WinHvPlatform.dll for the purpose of logging. 

## Overview and Usage

The general idea of this DLL is to inject it into a process which uses the [Windows Hypervisor Platform](https://docs.microsoft.com/en-us/virtualization/api/hypervisor-platform/hypervisor-platform) in order to get a basic understanding of what the process is doing in terms of virtualization.

To do this, create a process which uses WinHvPlatform.dll with the `CREATE_SUSPENDED` flag. Build the modified DLL whose souce code is provided in this repo. Inject the modified WinHvPlatform.dll into the process. Resume the suspended process.

The shouce code for the DLL opens a mailslot server and writes each API call as it occurs. A very simple mailslot server's source code is provided. 

## Example
This image shows QEMU running DOS with the Windows Hypervisor Platform accelerator: 
![Image1](https://user-images.githubusercontent.com/16905064/135654525-024380e3-77e6-49a3-9941-3fcf30ea00ab.png)

## Notes
* It's important to note that this obviosuly doesn't include the WHP functions from the WinHvEmulation.dll, though an identical method could be used to hook its functions as well.
* The `printf` calls are the main performance bottleneck.
* No error checking.
