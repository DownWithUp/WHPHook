#include <Windows.h>
#include <stdio.h>
#include <WinHvPlatformDefs.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "VM.h"
#include "CommunicationTypes.h"


BOOL G_bListenForReply = FALSE;
CRITICAL_SECTION G_CriticalSection;

void ClientReadRoutine(HANDLE Pipe)
{
    COMMS_PACKET commsPacket;

    while (TRUE)
    {
        ZeroMemory(&commsPacket, sizeof(commsPacket));
        if (G_bListenForReply)
        {
            if (ReadFile(Pipe, &commsPacket, sizeof(commsPacket), NULL, NULL))
            {
                switch (commsPacket.PacketType)
                {
                case StringPacket:
                    printf("StringPacket data: %s\n", commsPacket.Packet.String);
                    break;
                case StopAndInfoPacket:
                    printf("Received a stop and info packet. Handle is: %I64X\n", commsPacket.Packet.StopAndInfo.PartitionHandle);
                    printf("Received a stop and info packet. VCPU Count: %X\n", commsPacket.Packet.StopAndInfo.vCPUCount);
                    break;
                case StopAndTranslatePacket:
                    printf("Received a stop and translate packet.ResultCode : %X\n", commsPacket.Packet.StopAndTranslate.ResultCode);
                    printf("Received a stop and translate packet. ResultAddress: %I64X\n", commsPacket.Packet.StopAndTranslate.ResultAddress);
                    break;
                case StopAndDumpGPRegsPacket:
                    printf("Received a stop and dump register packet for vCPU #%d:\n", commsPacket.Packet.StopAndDumpGPRegsPacket.vCPUIndex);
                    printf("RAX: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterRax].Reg64);
                    printf("RCX: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterRcx].Reg64);
                    printf("RDX: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterRdx].Reg64);
                    printf("RBX: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterRbx].Reg64);
                    printf("RSP: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterRsp].Reg64);
                    printf("RBP: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterRbp].Reg64);
                    printf("RSI: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterRsi].Reg64);
                    printf("RDI: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterRdi].Reg64);
                    printf("R8: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterR8].Reg64);
                    printf("R9: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterR9].Reg64);
                    printf("R10: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterR10].Reg64);
                    printf("R11: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterR11].Reg64);
                    printf("R12: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterR12].Reg64);
                    printf("R13: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterR13].Reg64);
                    printf("R14: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterR14].Reg64);
                    printf("R15: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterR15].Reg64);
                    printf("Rip: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterRip].Reg64);
                    printf("Rflags: %I64X\n", commsPacket.Packet.StopAndDumpGPRegsPacket.GPRegs[WHvX64RegisterRflags].Reg64);
                    break;
                default:
                    printf("[!] Unknown packet response\n");
                    break;
                }

                EnterCriticalSection(&G_CriticalSection);
                G_bListenForReply = FALSE;
                LeaveCriticalSection(&G_CriticalSection);
            }
            else
            {
                printf("[!] ReadFile Error. GLE: %X\n", GetLastError());
                Sleep(100);
            }
        }
    }
}


std::vector<std::string> split(const std::string str, char delim)
{
    std::vector<std::string> result;
    std::istringstream ss{ str };
    std::string token;
    while (std::getline(ss, token, delim)) {
        if (!token.empty()) {
            result.push_back(token);
        }
    }
    return result;
}

void main()
{
    HANDLE hClientPipe;
    HANDLE hListenThread;
    COMMS_PACKET commsPacket;
    VM VM;
    std::vector<std::string>arguments;

    InitializeCriticalSection(&G_CriticalSection);
    printf("[i] Started client...\n");

    while (TRUE)
    {

        VM.Initialize(&hClientPipe);
        if (hClientPipe != INVALID_HANDLE_VALUE)
        {
            hListenThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ClientReadRoutine, hClientPipe, 0, 0);
            printf("[i] Connected. Handle is: %I64X\n", hClientPipe);
            break;
        }
        else
        {
            printf("[!] Error: %X\n", GetLastError());
        }
        Sleep(1000);
        printf("Waiting to connect to client....\n");
    }

    while (TRUE)
    {
        ZeroMemory(&commsPacket, sizeof(commsPacket));
        std::string input;

        std::cout << "> ";
        std::getline(std::cin, input);
        arguments = split(input, ' ');

        if (!arguments.empty())
        {
            if (!strcmp(arguments[0].c_str(), "info"))
            {
                commsPacket.PacketType = StopAndInfoPacket;
                if (!G_bListenForReply)
                {
                    if (!WriteFile(hClientPipe, &commsPacket, sizeof(commsPacket), NULL, NULL))
                    {
                        printf("[!] WriteFile GLE: %X\n", GetLastError());
                    }
                    else
                    {
                        EnterCriticalSection(&G_CriticalSection);
                        G_bListenForReply = TRUE;
                        LeaveCriticalSection(&G_CriticalSection);
                    }
                }
                else
                {
                    printf("[!] In Waiting for reply mode\n");
                }
            }
            else if (!strcmp(arguments[0].c_str(), "vtop"))
            {
                if (arguments.size() == 2)
                {
                    unsigned long long address = std::strtoull(arguments[1].c_str(), nullptr, 16);
                    commsPacket.PacketType = StopAndTranslatePacket;
                    commsPacket.Packet.StopAndTranslate.InputVA = (WHV_GUEST_VIRTUAL_ADDRESS)address;
                    if (!G_bListenForReply)
                    {
                        if (!WriteFile(hClientPipe, &commsPacket, sizeof(commsPacket), NULL, NULL))
                        {
                            printf("WriteFile GLE: %X\n", GetLastError());
                        }
                        else
                        {
                            EnterCriticalSection(&G_CriticalSection);
                            G_bListenForReply = TRUE;
                            LeaveCriticalSection(&G_CriticalSection);
                        }
                    }
                    else
                    {
                        printf("[!] In Waiting for reply mode\n");
                    }
                }
                else
                {
                    printf("[!] Usage: vtop [virtual address]\n");
                }
            }
            else if (!strcmp(arguments[0].c_str(), "dump"))
            {
                if (arguments.size() == 2)
                {
                    unsigned long vCPUIndex = std::strtoul(arguments[1].c_str(), nullptr, 16);
                    memset(&commsPacket, 0x00, sizeof(commsPacket));
                    commsPacket.PacketType = (PACKET_TYPE)2;
                    commsPacket.Packet.StopAndDumpGPRegsPacket.vCPUIndex = vCPUIndex;
                    if (!G_bListenForReply)
                    {
                        if (!WriteFile(hClientPipe, &commsPacket, sizeof(commsPacket), NULL, NULL))
                        {
                            printf("WriteFile GLE: %X\n", GetLastError());
                        }
                        else
                        {
                            EnterCriticalSection(&G_CriticalSection);
                            G_bListenForReply = TRUE;
                            LeaveCriticalSection(&G_CriticalSection);
                        }
                    }
                    else
                    {
                        printf("[!] In Waiting for reply mode\n");
                    }
                }
                else
                {
                    printf("[!] Usage: dump [vCPU index]\n");
                }
            }
        } 
    }
}
