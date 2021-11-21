# WHPHook
Simple DLL that hooks all the functions in WinHvPlatform.dll for the purpose of logging and or gaining introspection a guest OS.

## Overview and Usage

The general idea of this DLL is to inject it into a process which uses the [Windows Hypervisor Platform](https://docs.microsoft.com/en-us/virtualization/api/hypervisor-platform/hypervisor-platform) (WHP) in order to get a basic understanding of what the process is doing in terms of virtualization.

To do this, create a process which uses WinHvPlatform.dll with the `CREATE_SUSPENDED` flag. Build the modified DLL whose souce code is provided in this repo. Inject the modified WinHvPlatform.dll into the process. Resume the suspended process.

On load the DLL creates a duplex pipe and starts a server thread. A client can connect and then interact with the DLL essentially allowing it to debug the guest OS supported by hypervisor platform. 


## Example
This image shows QEMU running DOS with the Windows Hypervisor Platform accelerator: 
<p align="center">
  <img src="https://user-images.githubusercontent.com/16905064/142759492-db23b7d6-b403-46e7-beba-9a4843831f34.PNG"/>
 </p>
This image shows the 'dump' command:<br>
<p align="center">
  <img src="https://user-images.githubusercontent.com/16905064/142771504-dea0aee4-8595-448f-9390-8c5f212d82da.PNG"/>
</p>


## Notes
* It's important to note that this doesn't include/hook the WHP functions from the WinHvEmulation.dll, though an identical method could be used to hook its functions as well.
* Currently just works with QEMU's usage of the WHP, but should work with VMware products and others that utilize the WHP.
* Just an experiment/PoC/toy 🧐
