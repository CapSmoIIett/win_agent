#pragma once

#include <windows.h>
#include <iptypes.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <string>

std::string getDeviceName()
{
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);

    if (!GetComputerNameA(computerName, &size))
    {
        std::cout << "Failed to get device name." << std::endl;
        throw;
    }

    return std::string(computerName, size);
}

std::string getHostName()
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    char hostname[MAX_HOSTNAME_LEN + 1];
    if (0 != gethostname(hostname, sizeof(hostname))) {
        std::cout << "Failed to get host name." << std::endl;
        std::cout << GetLastError() << std::endl;
        WSACleanup();
        return {};
    }

    WSACleanup();

    return std::string(hostname);
}

std::vector<std::string> getIPs()
{
    std::vector<std::string> res;
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    struct addrinfo hints {}, *info;
    hints.ai_family = AF_INET; // IPv4


    getaddrinfo(getHostName().data(), nullptr, &hints, &info);

    for (struct addrinfo* p = info; p != nullptr; p = p->ai_next) 
    {
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &((struct sockaddr_in*)p->ai_addr)->sin_addr, ip, INET_ADDRSTRLEN);
        res.push_back(ip);
    }

    freeaddrinfo(info);
    WSACleanup();

    return res;
}


std::string getMacAdress()
{
    ULONG bufferSize = sizeof(IP_ADAPTER_INFO);
    IP_ADAPTER_INFO* adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
    if (!adapterInfo) return "";

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_BUFFER_OVERFLOW) {
        free(adapterInfo);
        adapterInfo = (IP_ADAPTER_INFO*)malloc(bufferSize);
        if (!adapterInfo) return "";
    }

    std::string macAddress;

    if (GetAdaptersInfo(adapterInfo, &bufferSize) == NO_ERROR) {
        IP_ADAPTER_INFO* adapter = adapterInfo;
        if (adapter) {
            std::ostringstream oss;
            for (UINT i = 0; i < adapter->AddressLength; i++) {
                if (i != 0)
                    oss << "-";
                oss << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << (int)adapter->Address[i];
            }
            macAddress = oss.str();
        }
    }

    free(adapterInfo);
    return macAddress;
}