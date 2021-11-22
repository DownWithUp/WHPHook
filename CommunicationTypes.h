#pragma once

typedef enum _PACKET_TYPE
{
    StopAndInfoPacket,
    StopAndTranslatePacket,
    StopAndDumpGPRegsPacket,
    StringPacket
} PACKET_TYPE;

typedef struct _COMMS_PACKET
{
    PACKET_TYPE PacketType;
    union
    {
        struct
        {
            WHV_PARTITION_HANDLE PartitionHandle;
            ULONG vCPUCount;
        } StopAndInfo;
        struct
        {
            WHV_GUEST_VIRTUAL_ADDRESS InputVA;
            WHV_TRANSLATE_GVA_RESULT_CODE ResultCode;
            WHV_GUEST_PHYSICAL_ADDRESS ResultAddress;
        } StopAndTranslate;
        struct
        {
            WHV_REGISTER_VALUE GPRegs[WHvX64RegisterRflags + 1]; // Up to 17 regs
            ULONG vCPUIndex;
        } StopAndDumpGPRegsPacket;
        CHAR String[256];
    } Packet;
} COMMS_PACKET, * PCOMMS_PACKET;
