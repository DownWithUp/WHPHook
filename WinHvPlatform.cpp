#include <windows.h>
#include <WinHvPlatformDefs.h>
#include "CommunicationTypes.h"

#define DEBUG

// Globals
HMODULE G_RealLibrary;
HANDLE  G_hLogFile = INVALID_HANDLE_VALUE;
HANDLE  G_hClientMailslot = INVALID_HANDLE_VALUE;
HANDLE  G_hPipe = INVALID_HANDLE_VALUE;

BOOL G_bBreakBeforeRun = FALSE;
BOOL G_bListenForReply = FALSE;
COMMS_PACKET G_LastPacket;

CRITICAL_SECTION G_CriticalSection;

typedef HRESULT(WINAPI* WHvGetCapability_Real)(
    _In_ WHV_CAPABILITY_CODE CapabilityCode, 
    _Out_writes_bytes_to_(CapabilityBufferSizeInBytes, *WrittenSizeInBytes) VOID* CapabilityBuffer, 
    _In_ UINT32 CapabilityBufferSizeInBytes, 
    _Out_opt_ UINT32* WrittenSizeInBytes
    );
typedef HRESULT(WINAPI* WHvCreatePartition_Real)(_Out_ WHV_PARTITION_HANDLE* Partition);
typedef HRESULT(WINAPI* WHvSetupPartition_Real)(_In_ WHV_PARTITION_HANDLE Partition);
typedef HRESULT(WINAPI* WHvDeletePartition_Real)(_In_ WHV_PARTITION_HANDLE Partition);
typedef HRESULT(WINAPI* WHvGetPartitionProperty_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ WHV_PARTITION_PROPERTY_CODE PropertyCode,
    _Out_writes_bytes_to_(PropertyBufferSizeInBytes, *WrittenSizeInBytes) VOID* PropertyBuffer,
    _In_ UINT32 PropertyBufferSizeInBytes, 
    _Out_opt_ UINT32* WrittenSizeInBytes
    );
typedef HRESULT(WINAPI* WHvSetPartitionProperty_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ WHV_PARTITION_PROPERTY_CODE PropertyCode,
    _In_reads_bytes_(PropertyBufferSizeInBytes) const VOID* PropertyBuffer,
    _In_ UINT32 PropertyBufferSizeInBytes
    );
typedef HRESULT(WINAPI* WHvMapGpaRange_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ VOID* SourceAddress,
    _In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress, 
    _In_ UINT64 SizeInBytes,
    _In_ WHV_MAP_GPA_RANGE_FLAGS Flags
    );
typedef HRESULT(WINAPI* WHvUnmapGpaRange_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress,
    _In_ UINT64 SizeInBytes
    );
typedef HRESULT(WINAPI* WHvTranslateGva_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ UINT32 VpIndex, 
    _In_ WHV_GUEST_VIRTUAL_ADDRESS Gva,
    _In_ WHV_TRANSLATE_GVA_FLAGS TranslateFlags,
    _Out_ WHV_TRANSLATE_GVA_RESULT* TranslationResult, 
    _Out_ WHV_GUEST_PHYSICAL_ADDRESS* Gpa
    );
typedef HRESULT(WINAPI* WHvCreateVirtualProcessor_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ UINT32 VpIndex, 
    _In_ UINT32 Flags
    );
typedef HRESULT(WINAPI* WHvDeleteVirtualProcessor_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ UINT32 VpIndex
    );
typedef HRESULT(WINAPI* WHvRunVirtualProcessor_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ UINT32 VpIndex,
    _Out_writes_bytes_(ExitContextSizeInBytes) VOID* ExitContext, 
    _In_ UINT32 ExitContextSizeInBytes
    );
typedef HRESULT(WINAPI* WHvCancelRunVirtualProcessor_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ UINT32 VpIndex, 
    _In_ UINT32 Flags
    );
typedef HRESULT(WINAPI* WHvGetVirtualProcessorRegisters_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ UINT32 VpIndex,
    _In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames,
    _In_ UINT32 RegisterCount,
    _Out_writes_(RegisterCount) WHV_REGISTER_VALUE* RegisterValues
    );
typedef HRESULT(WINAPI* WHvSetVirtualProcessorRegisters_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ UINT32 VpIndex,
    _In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames,
    _In_ UINT32 RegisterCount,
    _In_reads_(RegisterCount) const WHV_REGISTER_VALUE* RegisterValues
    );
/*
 * QEMU comment:
 * These are supplemental functions that may not be present
 * on all versions and are not critical for basic functionality.
 */
typedef HRESULT(WINAPI* WHvSuspendPartitionTime_Real)(_In_ WHV_PARTITION_HANDLE Partition);
typedef HRESULT(WINAPI* WHvRequestInterrupt_Real)(
    _In_ WHV_PARTITION_HANDLE Partition, 
    _In_ const WHV_INTERRUPT_CONTROL* Interrupt,
    _In_ UINT32 InterruptControlSize
    );
typedef HRESULT(WINAPI* WHvGetVirtualProcessorInterruptControllerState2_Real)(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _Out_writes_bytes_to_(StateSize, *WrittenSize) VOID* State,
    _In_ UINT32 StateSize,
    _Out_opt_ UINT32* WrittenSize
    );
typedef HRESULT(WINAPI* WHvSetVirtualProcessorInterruptControllerState2_Real)( 
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_reads_bytes_(StateSize) const VOID* State,
    _In_ UINT32 StateSize
    );

WHvGetCapability_Real pWHvGetCapability_Real;
WHvCreatePartition_Real pWHvCreatePartition_Real;
WHvSetupPartition_Real pWHvSetupPartition_Real;
WHvDeletePartition_Real pWHvDeletePartition_Real;
WHvGetPartitionProperty_Real pWHvGetPartitionProperty_Real;
WHvSetPartitionProperty_Real pWHvSetPartitionProperty_Real;
WHvMapGpaRange_Real pWHvMapGpaRange_Real;
WHvUnmapGpaRange_Real pWHvUnmapGpaRange_Real;
WHvTranslateGva_Real pWHvTranslateGva_Real;
WHvCreateVirtualProcessor_Real pWHvCreateVirtualProcessor_Real;
WHvDeleteVirtualProcessor_Real pWHvDeleteVirtualProcessor_Real;
WHvRunVirtualProcessor_Real pWHvRunVirtualProcessor_Real;
WHvCancelRunVirtualProcessor_Real pWHvCancelRunVirtualProcessor_Real;
WHvGetVirtualProcessorRegisters_Real pWHvGetVirtualProcessorRegisters_Real;
WHvSetVirtualProcessorRegisters_Real pWHvSetVirtualProcessorRegisters_Real;
WHvSuspendPartitionTime_Real pWHvSuspendPartitionTime_Real;
WHvRequestInterrupt_Real pWHvRequestInterrupt_Real;
WHvGetVirtualProcessorInterruptControllerState2_Real pWHvGetVirtualProcessorInterruptControllerState2_Real;
WHvSetVirtualProcessorInterruptControllerState2_Real pWHvSetVirtualProcessorInterruptControllerState2_Real;
  
extern "C"
{
    __declspec(dllexport) HRESULT WINAPI WHvGetCapability(_In_ WHV_CAPABILITY_CODE CapabilityCode, 
        _Out_writes_bytes_to_(CapabilityBufferSizeInBytes, *WrittenSizeInBytes) VOID* CapabilityBuffer, 
        _In_ UINT32 CapabilityBufferSizeInBytes, 
        _Out_opt_ UINT32* WrittenSizeInBytes
    );
    __declspec(dllexport) HRESULT WINAPI WHvCreatePartition(_Out_ WHV_PARTITION_HANDLE* Partition);
    __declspec(dllexport) HRESULT WINAPI WHvSetupPartition(_In_ WHV_PARTITION_HANDLE Partition);
    __declspec(dllexport) HRESULT WINAPI WHvDeletePartition(_In_ WHV_PARTITION_HANDLE Partition);
    __declspec(dllexport) HRESULT WINAPI WHvGetPartitionProperty(_In_ WHV_PARTITION_HANDLE Partition, 
        _In_ WHV_PARTITION_PROPERTY_CODE PropertyCode,
        _Out_writes_bytes_to_(PropertyBufferSizeInBytes, 
        *WrittenSizeInBytes) VOID* PropertyBuffer,
        _In_ UINT32 PropertyBufferSizeInBytes, _Out_opt_ UINT32* WrittenSizeInBytes
    );
    __declspec(dllexport) HRESULT WINAPI WHvSetPartitionProperty(_In_ WHV_PARTITION_HANDLE Partition, 
        _In_ WHV_PARTITION_PROPERTY_CODE PropertyCode,
        _In_reads_bytes_(PropertyBufferSizeInBytes) const VOID* PropertyBuffer, 
        _In_ UINT32 PropertyBufferSizeInBytes
    );
    __declspec(dllexport) HRESULT WINAPI WHvMapGpaRange(_In_ WHV_PARTITION_HANDLE Partition, 
        _In_ VOID* SourceAddress,
        _In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress, 
        _In_ UINT64 SizeInBytes, 
        _In_ WHV_MAP_GPA_RANGE_FLAGS Flags
    );
    __declspec(dllexport) HRESULT WINAPI WHvUnmapGpaRange(_In_ WHV_PARTITION_HANDLE Partition, 
        _In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress,
        _In_ UINT64 SizeInBytes
    );
    __declspec(dllexport) HRESULT WINAPI WHvTranslateGva(_In_ WHV_PARTITION_HANDLE Partition, 
        _In_ UINT32 VpIndex, 
        _In_ WHV_GUEST_VIRTUAL_ADDRESS Gva,
        _In_ WHV_TRANSLATE_GVA_FLAGS TranslateFlags, 
        _Out_ WHV_TRANSLATE_GVA_RESULT* TranslationResult, 
        _Out_ WHV_GUEST_PHYSICAL_ADDRESS* Gpa
    );
    __declspec(dllexport) HRESULT WINAPI WHvCreateVirtualProcessor(_In_ WHV_PARTITION_HANDLE Partition, 
        _In_ UINT32 VpIndex, 
        _In_ UINT32 Flags
    );
    __declspec(dllexport) HRESULT WINAPI WHvDeleteVirtualProcessor(_In_ WHV_PARTITION_HANDLE Partition, 
        _In_ UINT32 VpIndex
    );
    __declspec(dllexport) HRESULT WINAPI WHvRunVirtualProcessor(_In_ WHV_PARTITION_HANDLE Partition, 
        _In_ UINT32 VpIndex,
        _Out_writes_bytes_(ExitContextSizeInBytes) VOID* ExitContext, 
        _In_ UINT32 ExitContextSizeInBytes
    );
    __declspec(dllexport) HRESULT WINAPI WHvCancelRunVirtualProcessor(_In_ WHV_PARTITION_HANDLE Partition, 
        _In_ UINT32 VpIndex, 
        _In_ UINT32 Flags
    );
    __declspec(dllexport) HRESULT WINAPI WHvGetVirtualProcessorRegisters(_In_ WHV_PARTITION_HANDLE Partition, 
        _In_ UINT32 VpIndex,
        _In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames, 
        _In_ UINT32 RegisterCount, 
        _Out_writes_(RegisterCount) WHV_REGISTER_VALUE* RegisterValues
    );
    __declspec(dllexport) HRESULT WINAPI WHvSetVirtualProcessorRegisters(_In_ WHV_PARTITION_HANDLE Partition,
        _In_ UINT32 VpIndex,
        _In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames,
        _In_ UINT32 RegisterCount,
        _In_reads_(RegisterCount) const WHV_REGISTER_VALUE* RegisterValues
    );
    __declspec(dllexport) HRESULT WINAPI WHvSuspendPartitionTime(_In_ WHV_PARTITION_HANDLE Partition);
    __declspec(dllexport) HRESULT WINAPI WHvRequestInterrupt(
        _In_ WHV_PARTITION_HANDLE Partition,
        _In_ const WHV_INTERRUPT_CONTROL* Interrupt,
        _In_ UINT32 InterruptControlSize
    );
    __declspec(dllexport) HRESULT WINAPI WHvGetVirtualProcessorInterruptControllerState2(
        _In_ WHV_PARTITION_HANDLE Partition,
        _In_ UINT32 VpIndex,
        _Out_writes_bytes_to_(StateSize, *WrittenSize) VOID* State,
        _In_ UINT32 StateSize,
        _Out_opt_ UINT32* WrittenSize
    );
    __declspec(dllexport) HRESULT WINAPI WHvSetVirtualProcessorInterruptControllerState2(
        _In_ WHV_PARTITION_HANDLE Partition,
        _In_ UINT32 VpIndex,
        _In_reads_bytes_(StateSize) const VOID* State,
        _In_ UINT32 StateSize
    );
}


void CommandProcessing(HANDLE Pipe)
{
    COMMS_PACKET        commsPacket;


    if (ConnectNamedPipe(G_hPipe, NULL))
    {
        while (TRUE)
        {
            if (G_bListenForReply)
            {
                if (ReadFile(Pipe, &commsPacket, sizeof(commsPacket), NULL, NULL))
                {
                    memcpy(&G_LastPacket, &commsPacket, sizeof(commsPacket));
                    switch (commsPacket.PacketType)
                    {
                    case StringPacket:
                        MessageBoxA(0, commsPacket.Packet.String, 0, 0); // Debug
                        break;
                    case StopAndInfoPacket:
                        EnterCriticalSection(&G_CriticalSection);
                        G_bBreakBeforeRun = TRUE;
                        LeaveCriticalSection(&G_CriticalSection);
                        break;
                    case StopAndTranslatePacket:
                        EnterCriticalSection(&G_CriticalSection);
                        G_bBreakBeforeRun = TRUE;
                        LeaveCriticalSection(&G_CriticalSection);
                        break;
                    case StopAndDumpGPRegsPacket:
                        EnterCriticalSection(&G_CriticalSection);
                        G_bBreakBeforeRun = TRUE;
                        LeaveCriticalSection(&G_CriticalSection);
                        break;
                    default:
                        break;
                    }

                    EnterCriticalSection(&G_CriticalSection);
                    G_bListenForReply = FALSE;
                    LeaveCriticalSection(&G_CriticalSection);
                }
                else
                {
                    goto Restart;
                }
            } 
        }
    }
    else
    {
        // Restart
        Restart:
        CloseHandle(Pipe);
        G_bListenForReply = TRUE;
        EnterCriticalSection(&G_CriticalSection);
        G_hPipe = CreateNamedPipeA("\\\\.\\pipe\\whp_hook", PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_TYPE_BYTE, PIPE_UNLIMITED_INSTANCES, 0x100, 0x1000, NMPWAIT_USE_DEFAULT_WAIT, NULL);
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CommandProcessing, G_hPipe, 0, 0);
        LeaveCriticalSection(&G_CriticalSection);
        ExitThread(0);
    }
}
    
HRESULT WHvGetCapability(_In_ WHV_CAPABILITY_CODE CapabilityCode, 
    _Out_writes_bytes_to_(CapabilityBufferSizeInBytes, *WrittenSizeInBytes) VOID* CapabilityBuffer, 
    _In_ UINT32 CapabilityBufferSizeInBytes, 
    _Out_opt_ UINT32* WrittenSizeInBytes
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvGetCapability\n", strlen("WHvGetCapability\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvGetCapability_Real(CapabilityCode, CapabilityBuffer, CapabilityBufferSizeInBytes, WrittenSizeInBytes));
}

HRESULT WINAPI WHvCreatePartition(_Out_ WHV_PARTITION_HANDLE* Partition)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvCreatePartition\n", strlen("WHvCreatePartition\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvCreatePartition_Real(Partition));
}

HRESULT WINAPI WHvSetupPartition(_In_ WHV_PARTITION_HANDLE Partition)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvSetupPartition\n", strlen("WHvSetupPartition\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvSetupPartition_Real(Partition));
}

HRESULT WINAPI WHvDeletePartition(_In_ WHV_PARTITION_HANDLE Partition)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvDeletePartition\n", strlen("WHvDeletePartition\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvDeletePartition_Real(Partition));
}

HRESULT WINAPI WHvGetPartitionProperty(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ WHV_PARTITION_PROPERTY_CODE PropertyCode,
    _Out_writes_bytes_to_(PropertyBufferSizeInBytes, *WrittenSizeInBytes) VOID* PropertyBuffer,
    _In_ UINT32 PropertyBufferSizeInBytes,
    _Out_opt_ UINT32* WrittenSizeInBytes
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvGetPartitionProperty\n", strlen("WHvGetPartitionProperty\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvGetPartitionProperty_Real(Partition, PropertyCode, PropertyBuffer, PropertyBufferSizeInBytes, WrittenSizeInBytes));
}

HRESULT WINAPI WHvSetPartitionProperty(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ WHV_PARTITION_PROPERTY_CODE PropertyCode,
    _In_reads_bytes_(PropertyBufferSizeInBytes) const VOID* PropertyBuffer,
    _In_ UINT32 PropertyBufferSizeInBytes
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvSetPartitionProperty\n", strlen("WHvSetPartitionProperty\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvSetPartitionProperty_Real(Partition, PropertyCode, PropertyBuffer, PropertyBufferSizeInBytes));
}

HRESULT WINAPI WHvMapGpaRange(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ VOID* SourceAddress,
    _In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress,
    _In_ UINT64 SizeInBytes,
    _In_ WHV_MAP_GPA_RANGE_FLAGS Flags
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvMapGpaRange\n", strlen("WHvMapGpaRange\n")  , NULL, NULL);
#endif // DEBUG
    return(pWHvMapGpaRange_Real(Partition, SourceAddress, GuestAddress, SizeInBytes, Flags));
}

HRESULT WINAPI WHvUnmapGpaRange(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ WHV_GUEST_PHYSICAL_ADDRESS GuestAddress,
    _In_ UINT64 SizeInBytes
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvUnmapGpaRange\n", strlen("WHvUnmapGpaRange\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvUnmapGpaRange_Real(Partition, GuestAddress, SizeInBytes));
}

HRESULT WINAPI WHvTranslateGva(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_ WHV_GUEST_VIRTUAL_ADDRESS Gva,
    _In_ WHV_TRANSLATE_GVA_FLAGS TranslateFlags,
    _Out_ WHV_TRANSLATE_GVA_RESULT* TranslationResult,
    _Out_ WHV_GUEST_PHYSICAL_ADDRESS* Gpa
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvTranslateGva\n", strlen("WHvTranslateGva\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvTranslateGva_Real(Partition, VpIndex, Gva, TranslateFlags, TranslationResult, Gpa));
}

HRESULT WINAPI WHvCreateVirtualProcessor(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_ UINT32 Flags
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvCreateVirtualProcessor\n", strlen("WHvCreateVirtualProcessor\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvCreateVirtualProcessor_Real(Partition, VpIndex, Flags));
}

HRESULT WINAPI WHvDeleteVirtualProcessor(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvDeleteVirtualProcessor\n", strlen("WHvDeleteVirtualProcessor\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvDeleteVirtualProcessor_Real(Partition, VpIndex));
}

HRESULT WINAPI WHvRunVirtualProcessor(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _Out_writes_bytes_(ExitContextSizeInBytes) VOID* ExitContext,
    _In_ UINT32 ExitContextSizeInBytes
)
{
    COMMS_PACKET commsPacket;

#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvRunVirtualProcessor\n", strlen("WHvRunVirtualProcessor\n"), NULL, NULL);
#endif // DEBUG

    if (G_bBreakBeforeRun)
    {
        ZeroMemory(&commsPacket, sizeof(commsPacket));

        WHV_REGISTER_NAME regNames[] = 
        { 
            WHvX64RegisterRax, WHvX64RegisterRcx, WHvX64RegisterRdx, WHvX64RegisterRbx, WHvX64RegisterRsp,
            WHvX64RegisterRbp, WHvX64RegisterRsi, WHvX64RegisterRdi, WHvX64RegisterR8, WHvX64RegisterR9, WHvX64RegisterR10, WHvX64RegisterR11,
            WHvX64RegisterR12, WHvX64RegisterR13, WHvX64RegisterR14, WHvX64RegisterR15, WHvX64RegisterRip, WHvX64RegisterRflags 
        };

        switch (G_LastPacket.PacketType)
        {
        case StopAndInfoPacket:
            commsPacket.PacketType = StopAndInfoPacket;
            commsPacket.Packet.StopAndInfo.PartitionHandle = Partition;

            pWHvGetPartitionProperty_Real(Partition, WHvPartitionPropertyCodeProcessorCount, &commsPacket.Packet.StopAndInfo.vCPUCount,
                sizeof(commsPacket.Packet.StopAndInfo.vCPUCount), NULL);
            break;
        case StopAndTranslatePacket:
            WHV_TRANSLATE_GVA_RESULT translateResult;
            WHV_GUEST_PHYSICAL_ADDRESS resultAddress;
            pWHvTranslateGva_Real(Partition, VpIndex, (WHV_GUEST_VIRTUAL_ADDRESS)G_LastPacket.Packet.StopAndTranslate.InputVA, 
                WHvTranslateGvaFlagValidateRead, &translateResult, &resultAddress);
                
            commsPacket.PacketType = StopAndTranslatePacket;
            commsPacket.Packet.StopAndTranslate.ResultCode = translateResult.ResultCode;
            commsPacket.Packet.StopAndTranslate.ResultAddress = resultAddress;
            break;
        case StopAndDumpGPRegsPacket:
            commsPacket.PacketType = StopAndDumpGPRegsPacket;
            commsPacket.Packet.StopAndDumpGPRegsPacket.vCPUIndex = G_LastPacket.Packet.StopAndDumpGPRegsPacket.vCPUIndex;
            memset(commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs, G_LastPacket.Packet.StopAndDumpGPRegsPacket.vCPUIndex, 
                sizeof(commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs));
                
            pWHvGetVirtualProcessorRegisters_Real(Partition, 0, regNames,
                _countof(regNames), commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs);
            break;
        default:
            break;
        }

        WriteFile(G_hPipe, &commsPacket, sizeof(commsPacket), NULL, NULL);

        EnterCriticalSection(&G_CriticalSection);
        G_bBreakBeforeRun = FALSE;
        G_bListenForReply = TRUE;
        LeaveCriticalSection(&G_CriticalSection);
    }

    return(pWHvRunVirtualProcessor_Real(Partition, VpIndex, ExitContext, ExitContextSizeInBytes));
}

HRESULT WINAPI WHvCancelRunVirtualProcessor(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_ UINT32 Flags
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvCancelRunVirtualProcessor\n", strlen("WHvCancelRunVirtualProcessor\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvCancelRunVirtualProcessor_Real(Partition, VpIndex, Flags));
}

HRESULT WINAPI WHvGetVirtualProcessorRegisters(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames,
    _In_ UINT32 RegisterCount,
    _Out_writes_(RegisterCount) WHV_REGISTER_VALUE* RegisterValues
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvGetVirtualProcessorRegisters\n", strlen("WHvGetVirtualProcessorRegisters\n"), NULL, NULL);
#endif
    return(pWHvGetVirtualProcessorRegisters_Real(Partition, VpIndex, RegisterNames, RegisterCount, RegisterValues));
}

HRESULT WINAPI WHvSetVirtualProcessorRegisters(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_reads_(RegisterCount) const WHV_REGISTER_NAME* RegisterNames,
    _In_ UINT32 RegisterCount,
    _In_reads_(RegisterCount) const WHV_REGISTER_VALUE* RegisterValues
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvSetVirtualProcessorRegisters\n", strlen("WHvSetVirtualProcessorRegisters\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvSetVirtualProcessorRegisters_Real(Partition, VpIndex, RegisterNames, RegisterCount, RegisterValues));
}

HRESULT WINAPI WHvSuspendPartitionTime(_In_ WHV_PARTITION_HANDLE Partition) 
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvSuspendPartitionTime\n", strlen("WHvSuspendPartitionTime\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvSuspendPartitionTime_Real(Partition));
}

HRESULT WINAPI WHvRequestInterrupt(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ const WHV_INTERRUPT_CONTROL* Interrupt,
    _In_ UINT32 InterruptControlSize
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvRequestInterrupt\n", strlen("WHvRequestInterrupt\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvRequestInterrupt_Real(Partition, Interrupt, InterruptControlSize));
}

HRESULT WINAPI WHvGetVirtualProcessorInterruptControllerState2(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _Out_writes_bytes_to_(StateSize, *WrittenSize) VOID* State,
    _In_ UINT32 StateSize,
    _Out_opt_ UINT32* WrittenSize
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvGetVirtualProcessorInterruptControllerState2\n", strlen("WHvGetVirtualProcessorInterruptControllerState2\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvGetVirtualProcessorInterruptControllerState2_Real(Partition, VpIndex, State, StateSize, WrittenSize));
}

HRESULT WINAPI WHvSetVirtualProcessorInterruptControllerState2(
    _In_ WHV_PARTITION_HANDLE Partition,
    _In_ UINT32 VpIndex,
    _In_reads_bytes_(StateSize) const VOID* State,
    _In_ UINT32 StateSize   
)
{
#ifdef DEBUG
    WriteFile(G_hLogFile, "WHvSetVirtualProcessorInterruptControllerState2\n", strlen("WHvSetVirtualProcessorInterruptControllerState2\n"), NULL, NULL);
#endif // DEBUG
    return(pWHvSetVirtualProcessorInterruptControllerState2_Real(Partition, VpIndex, State, StateSize));
}

VOID Init()
{
    pWHvGetCapability_Real = (WHvGetCapability_Real)GetProcAddress(G_RealLibrary, "WHvGetCapability");
    pWHvCreatePartition_Real = (WHvCreatePartition_Real)GetProcAddress(G_RealLibrary, "WHvCreatePartition");
    pWHvSetupPartition_Real = (WHvSetupPartition_Real)GetProcAddress(G_RealLibrary, "WHvSetupPartition");
    pWHvDeletePartition_Real = (WHvDeletePartition_Real)GetProcAddress(G_RealLibrary, "WHvDeletePartition");
    pWHvGetPartitionProperty_Real = (WHvGetPartitionProperty_Real)GetProcAddress(G_RealLibrary, "WHvGetPartitionProperty");
    pWHvSetPartitionProperty_Real = (WHvSetPartitionProperty_Real)GetProcAddress(G_RealLibrary, "WHvSetPartitionProperty");
    pWHvMapGpaRange_Real = (WHvMapGpaRange_Real)GetProcAddress(G_RealLibrary, "WHvMapGpaRange");
    pWHvUnmapGpaRange_Real = (WHvUnmapGpaRange_Real)GetProcAddress(G_RealLibrary, "WHvUnmapGpaRange");
    pWHvTranslateGva_Real = (WHvTranslateGva_Real)GetProcAddress(G_RealLibrary, "WHvTranslateGva");
    pWHvCreateVirtualProcessor_Real = (WHvCreateVirtualProcessor_Real)GetProcAddress(G_RealLibrary, "WHvCreateVirtualProcessor");
    pWHvDeleteVirtualProcessor_Real = (WHvDeleteVirtualProcessor_Real)GetProcAddress(G_RealLibrary, "WHvDeleteVirtualProcessor");
    pWHvRunVirtualProcessor_Real = (WHvRunVirtualProcessor_Real)GetProcAddress(G_RealLibrary, "WHvRunVirtualProcessor");
    pWHvCancelRunVirtualProcessor_Real = (WHvCancelRunVirtualProcessor_Real)GetProcAddress(G_RealLibrary, "WHvCancelRunVirtualProcessor");
    pWHvGetVirtualProcessorRegisters_Real = (WHvGetVirtualProcessorRegisters_Real)GetProcAddress(G_RealLibrary, "WHvGetVirtualProcessorRegisters");
    pWHvSetVirtualProcessorRegisters_Real = (WHvSetVirtualProcessorRegisters_Real)GetProcAddress(G_RealLibrary, "WHvSetVirtualProcessorRegisters");
    // "Supplemental functions"
    pWHvSuspendPartitionTime_Real = (WHvSuspendPartitionTime_Real)GetProcAddress(G_RealLibrary, "WHvSuspendPartitionTime");
    pWHvRequestInterrupt_Real = (WHvRequestInterrupt_Real)GetProcAddress(G_RealLibrary, "WHvRequestInterrupt");
    pWHvGetVirtualProcessorInterruptControllerState2_Real = (WHvGetVirtualProcessorInterruptControllerState2_Real)GetProcAddress(G_RealLibrary, "WHvGetVirtualProcessorInterruptControllerState2");
    pWHvSetVirtualProcessorInterruptControllerState2_Real = (WHvSetVirtualProcessorInterruptControllerState2_Real)GetProcAddress(G_RealLibrary, "WHvSetVirtualProcessorInterruptControllerState2");

    G_bListenForReply = TRUE;
    InitializeCriticalSection(&G_CriticalSection);
    G_hPipe = CreateNamedPipeA("\\\\.\\pipe\\whp_hook", PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE, PIPE_TYPE_BYTE, PIPE_UNLIMITED_INSTANCES, 0x100, 0x1000, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CommandProcessing, G_hPipe, 0, NULL);

#ifdef DEBUG
    G_hLogFile = CreateFileA("C:\\Windows\\Temp\\whp_hook_log.txt", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
#endif // DEBUG


}

BOOL WINAPI DllMain(HINSTANCE Dll, DWORD Reason, LPVOID Reserved)  
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        G_RealLibrary = LoadLibrary(L"C:\\Windows\\System32\\WinHvPlatform.dll");
        if (!G_RealLibrary)
        {
            MessageBox(0, L"Unable to load the real WinHvPlatform.dll", L"Error", MB_ICONERROR);
        }
        else
        {
            Init();
        }
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        if (G_hLogFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(G_hLogFile);
        }
        break;
    }
    return(TRUE);
}
