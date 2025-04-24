#pragma once 

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")  // ���������� ���������� Winsock

// ����� � ���� ��� multicast
const char* MULTICAST_GROUP = "239.255.0.1";
const int MULTICAST_PORT = 30001;
const int BUFFER_SIZE = 256;

static bool isEnd = true;

// ������� �������� multicast-���������
void sender()
{
    // ������������� Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return;
    }

    // ������ UDP �����
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return;
    }

    // �������� loopback ��� multicast, ����� �������� ���� �� ��������� (����� ������, ���� �� �����)
    BOOL loopback = TRUE;
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char*)&loopback, sizeof(loopback)) == SOCKET_ERROR) {
        std::cerr << "setsockopt IP_MULTICAST_LOOP failed: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return;
    }

    // ����� ���������� � multicast ������ � ����
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(MULTICAST_PORT);
    inet_pton(AF_INET, MULTICAST_GROUP, &addr.sin_addr);

    const char* message = "AppInstanceAlive";  // ���������, ������������, ��� ��������� ���

    //while (isEnd) {
        // ���������� ��������� � multicast ������
        int sent = sendto(sock, message, (int)strlen(message), 0, (sockaddr*)&addr, sizeof(addr));
        if (sent == SOCKET_ERROR) {
            std::cerr << "sendto() failed: " << WSAGetLastError() << "\n";
        }
        else {
            std::cout << "[Sender] Sent multicast presence message\n";
        }
        // ��� 5 ������ ����� ��������� ���������
        std::this_thread::sleep_for(std::chrono::seconds(5));
    //}

    // ��������� ����� � ����������� ������� Winsock
    closesocket(sock);
    WSACleanup();
}

// ������� ����� multicast-���������
void receiver(std::optional<std::function<void(std::string_view, std::string_view)>> func = std::nullopt)
{
    // ������������� Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return;
    }

    // ������ UDP �����
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "socket() failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return;
    }

    // ����������� ����� � ���������� ������ � �����, ����� ��������� ��������� �� MULTICAST_PORT
    sockaddr_in localAddr{};
    localAddr.sin_family = AF_INET;
    localAddr.sin_port = htons(MULTICAST_PORT);
    localAddr.sin_addr.s_addr = htonl(INADDR_ANY);  // ��������� �� ���� �����������

    if (bind(sock, (sockaddr*)&localAddr, sizeof(localAddr)) == SOCKET_ERROR) {
        std::cerr << "bind() failed: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return;
    }

    // ������������� �� multicast ������
    ip_mreq mreq{};
    inet_pton(AF_INET, MULTICAST_GROUP, &mreq.imr_multiaddr);  // ����� multicast ������
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);            // ���������� ��� ����������

    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&mreq, sizeof(mreq)) == SOCKET_ERROR) {
        std::cerr << "setsockopt IP_ADD_MEMBERSHIP failed: " << WSAGetLastError() << "\n";
        closesocket(sock);
        WSACleanup();
        return;
    }

    char buffer[BUFFER_SIZE];

    while (isEnd) {
        sockaddr_in senderAddr{};
        int addrlen = sizeof(senderAddr);

        // �������� ������ �� ������ (����������� �����)
        int nbytes = recvfrom(sock, buffer, BUFFER_SIZE - 1, 0, (sockaddr*)&senderAddr, &addrlen);
        if (nbytes == SOCKET_ERROR) {
            std::cerr << "recvfrom() failed: " << WSAGetLastError() << "\n";
            break;
        }
        buffer[nbytes] = '\0';  // ��������� ����-���������� � ����� ������

        // ����������� IP ����� ����������� � ������ ��� ������
        char senderIp[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &senderAddr.sin_addr, senderIp, sizeof(senderIp));

        // ������� ���������� ��������� � IP �����������
        std::cout << "[Receiver] Received message from " << senderIp << ": " << buffer << std::endl;

        if (func)
        {
            (*func)(senderIp, buffer);
        }
    }

    // ��������� ����� � ����������� ������� Winsock
    closesocket(sock);
    WSACleanup();
}



struct sIP {
    std::string ip;
    std::string name;
};

std::vector<sIP> GetLocalIPs() {
    std::vector<sIP> IpList;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return IpList;
    }

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
        std::cerr << "gethostname failed\n";
        WSACleanup();
        return IpList;
    }

    struct hostent* host = gethostbyname(hostname);
    if (host == nullptr) {
        std::cerr << "gethostbyname failed\n";
        WSACleanup();
        return IpList;
    }

    for (int i = 0; host->h_addr_list[i] != nullptr; ++i) {
        struct in_addr addr;
        memcpy(&addr, host->h_addr_list[i], sizeof(struct in_addr));
        sIP ipEntry;
        ipEntry.ip = inet_ntoa(addr);
        ipEntry.name = "local interface";
        IpList.push_back(ipEntry);
    }

    WSACleanup();
    return IpList;
}