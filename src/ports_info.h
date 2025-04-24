#pragma once

//Based on, but modified: https://docs.microsoft.com/en-us/windows/desktop/api/iphlpapi/nf-iphlpapi-gettcptable
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#include <stdio.h>
#include <string>

#include <iostream>

#include <vector>

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

struct PortInfo
{
    std::string type;
    std::string localAddr;
    std::string remoteAddr;
    std::string state;
    std::string pid;
};


std::vector<PortInfo> getPorts()
{
    std::vector<PortInfo> res;

    PMIB_TCPTABLE_OWNER_PID pTcpTable;
    PMIB_UDPTABLE_OWNER_PID pUdpTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;
    char szLocalAddr[128];
    char szRemoteAddr[128];
    struct in_addr IpAddr;
    int i;

    // TCP

    pTcpTable = (MIB_TCPTABLE_OWNER_PID*)MALLOC(sizeof(MIB_TCPTABLE_OWNER_PID));
    if (pTcpTable == NULL) {
        std::cout << "malloc error\n";
        throw;
    }

    dwSize = sizeof(MIB_TCPTABLE_OWNER_PID);
    if (ERROR_INSUFFICIENT_BUFFER == (dwRetVal = GetExtendedTcpTable(pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0)))
    {
        FREE(pTcpTable);
        pTcpTable = (MIB_TCPTABLE_OWNER_PID*)MALLOC(dwSize);
        if (pTcpTable == NULL)
        {
            std::cout << "malloc error\n";
            throw;
        }
    }
    if (NO_ERROR == (dwRetVal = GetExtendedTcpTable(pTcpTable, &dwSize, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0)))
    {
        for (i = 0; i < (int)pTcpTable->dwNumEntries; i++)
        {
            IpAddr.S_un.S_addr = (u_long)pTcpTable->table[i].dwLocalAddr;
            strcpy_s(szLocalAddr, sizeof(szLocalAddr), inet_ntoa(IpAddr));

            DWORD pid = pTcpTable->table[i].dwOwningPid;
            
            IpAddr.S_un.S_addr = (u_long)pTcpTable->table[i].dwRemoteAddr;
            strcpy_s(szRemoteAddr, sizeof(szRemoteAddr), inet_ntoa(IpAddr));

            std::string state = "";
            switch (pTcpTable->table[i].dwState)
            {
            case MIB_TCP_STATE_CLOSED:
                state = "CLOSED";
                break;
            case MIB_TCP_STATE_LISTEN:
                state = "LISTEN";
                break;
            case MIB_TCP_STATE_SYN_SENT:
                state = "SYN-SENT";
                break;
            case MIB_TCP_STATE_SYN_RCVD:
                state = "SYN-RECEIVED";
                break;
            case MIB_TCP_STATE_ESTAB:
                state = "ESTABLISHED";
                break;
            case MIB_TCP_STATE_FIN_WAIT1:
                state = "FIN-WAIT-1";
                break;
            case MIB_TCP_STATE_FIN_WAIT2:
                state = "FIN-WAIT-2";
                break;
            case MIB_TCP_STATE_CLOSE_WAIT:
                state = "CLOSE-WAIT";
                break;
            case MIB_TCP_STATE_CLOSING:
                state = "CLOSING";
                break;
            case MIB_TCP_STATE_LAST_ACK:
                state = "LAST-ACK";
                break;
            case MIB_TCP_STATE_TIME_WAIT:
                state = "TIME-WAIT\n";
                break;
            case MIB_TCP_STATE_DELETE_TCB:
                state = "DELETE-TCB\n";
                break;
            default:
                state = "UNKNOWN";
                break;
            }

            /*
            printf("\nTCP\t %s:%d\t%s:%d\t%s\t%d",
                szLocalAddr,
                ntohs((u_short)pTcpTable->table[i].dwLocalPort),
                szRemoteAddr,
                ntohs((u_short)pTcpTable->table[i].dwRemotePort),
                state.c_str(),
                pid);
            */

            res.push_back(PortInfo{
                "TCP",  // type
                std::string(szLocalAddr) + ":" + std::to_string(ntohs((u_short)pTcpTable->table[i].dwLocalPort)), // local
                std::string(szRemoteAddr) + ":" + std::to_string(ntohs((u_short)pTcpTable->table[i].dwRemotePort)), // remote
                state,// state
                std::to_string(pid) // pid
                });
        }
    }
    else
    {
        FREE(pTcpTable);
        std::cout << "GetExtendedTcpTable error\n";
        throw;
        //return dwRetVal;
    }

    // UDP

    pUdpTable = (MIB_UDPTABLE_OWNER_PID*)MALLOC(sizeof(MIB_UDPTABLE_OWNER_PID));
    if (pUdpTable == NULL) {
        std::cout << "malloc error\n";
        throw;
    }
    dwSize = sizeof(MIB_UDPTABLE_OWNER_PID);
    if ((dwRetVal = GetExtendedUdpTable(pUdpTable, &dwSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0)) == ERROR_INSUFFICIENT_BUFFER)
    {
        FREE(pUdpTable);
        pUdpTable = (MIB_UDPTABLE_OWNER_PID*)MALLOC(dwSize);
        if (pUdpTable == NULL)
        {
            std::cout << "malloc error\n";
            throw;
        }
    }
    if ((dwRetVal = GetExtendedUdpTable(pUdpTable, &dwSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0)) == NO_ERROR)
    {
        for (i = 0; i < (int)pUdpTable->dwNumEntries; i++)
        {
            IpAddr.S_un.S_addr = (u_long)pUdpTable->table[i].dwLocalAddr;
            strcpy_s(szLocalAddr, sizeof(szLocalAddr), inet_ntoa(IpAddr));

            DWORD pid = pUdpTable->table[i].dwOwningPid;
            
            /*
            printf("\nUDP\t %s:%d\t%d",
                szLocalAddr,
                ntohs((u_short)pUdpTable->table[i].dwLocalPort),
                pid);
            */

            res.push_back(PortInfo{
               "UDP",  // type
               std::string(szLocalAddr) + ":" + std::to_string(ntohs((u_short)pTcpTable->table[i].dwLocalPort)), // local
               "", // remote
               "", // state
               std::to_string(pid) // pid
            });
        }
    }
    else
    {
        FREE(pTcpTable);
        std::cout << "GetExtendedUdpTable error\n";
        throw;
    }

    // CLEANUP

    if (pTcpTable != NULL)
    {
        FREE(pTcpTable);
        pTcpTable = NULL;
    }
    if (pUdpTable != NULL)
    {
        FREE(pUdpTable);
        pUdpTable = NULL;
    }


    return res;
}
