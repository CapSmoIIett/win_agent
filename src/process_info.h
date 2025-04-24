#pragma once

#include <Wincrypt.h>
#include <psapi.h>

#include <tchar.h>

#include <vector>

#include <string>
#include <string_view>

#include <filesystem>


struct ProcessInfo
{
    std::string path;
    std::string hash_MD5;
    std::string pid;
};


std::vector<BYTE> getHash(std::string_view filepath, ALG_ID alg)
{
    // Инициализация криптографического контекста
    HCRYPTPROV hCryptProv = 0;

    if (!CryptAcquireContext(
        &hCryptProv,
        NULL,
        NULL,
        PROV_RSA_FULL,
        0))
    {
        if (GetLastError() != NTE_BAD_KEYSET)
        {
            std::cout << "Error initializing cryptographic context." << std::endl;
            throw;
        }

        if (!CryptAcquireContext(
            &hCryptProv,
            NULL,
            NULL,
            PROV_RSA_FULL,
            CRYPT_NEWKEYSET))
        {
            std::cout << "Could not create the default key container.\n";
        }

        std::cout << "CryptAcquireContext new keyset succeeded.\n";
    }

    // Открытие файла
    HANDLE hFile = CreateFileA(
        filepath.data(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (hFile == INVALID_HANDLE_VALUE) {
        std::cout << "Ошибка при открытии файла." << std::endl;
        CryptReleaseContext(hCryptProv, 0);
        throw;
    }


    // Создание хэш-объекта

    HCRYPTHASH hHash = 0;

    if (!CryptCreateHash(hCryptProv, alg, 0, 0, &hHash))
    {
        std::cout << "Ошибка при создании хэш-объекта." << GetLastError() << std::endl;
        CloseHandle(hFile);
        CryptReleaseContext(hCryptProv, 0);
        throw;
    }

    // Чтение файла и вычисление хэша
    const DWORD bufferSize = 4096;
    BYTE buffer[bufferSize];
    DWORD bytesRead;

    while (ReadFile(hFile, buffer, bufferSize, &bytesRead, NULL))
    {
        if (!CryptHashData(hHash, buffer, bytesRead, 0))
        {
            std::cout << "Ошибка при обновлении хэша." << std::endl;
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            CryptReleaseContext(hCryptProv, 0);
            throw;
        }
        if (bytesRead < bufferSize) break; // конец файла
    }

    // Получение окончательного хэша
    DWORD hashSize = 32; // SHA256 хэш имеет размер 32 байта
    //DWORD hashSize = 20; // SHA1 хэш имеет размер 20 байта
    BYTE* hashBytes = new BYTE[hashSize];


    if (!CryptGetHashParam(hHash, HP_HASHVAL, hashBytes, &hashSize, 0)) {
        std::cout << "Ошибка при получении хэша." << std::endl;
        delete[] hashBytes;
        CryptDestroyHash(hHash);
        CloseHandle(hFile);
        CryptReleaseContext(hCryptProv, 0);
        throw;
    }

    // Вывод хэша
    for (DWORD i = 0; i < hashSize; ++i) {
        std::cout << std::hex << std::uppercase << (int)(hashBytes[i]);
        //printf("%x", hashBytes[i]);
    }
    std::cout << std::endl;

    std::vector<BYTE> res(hashBytes, hashBytes + hashSize);

    // Освобождение ресурсов
    delete[] hashBytes;
    CryptDestroyHash(hHash);
    CloseHandle(hFile);
    CryptReleaseContext(hCryptProv, 0);

    return res;
}

std::wstring getProcessName(DWORD processID)
{
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processID);
    if (hProcess == NULL) {
        DWORD errorCode = GetLastError();
        //std::cout << "Не удалось открыть процесс" << std::endl;
        // TODO: Log error
        return { L"!!!NO NAME!!!" };
    }

    std::wstring buffer;
    buffer.resize(MAX_PATH);

    while (true) {
        DWORD neededSize = static_cast<DWORD>(buffer.size());
        BOOL result = QueryFullProcessImageNameW(hProcess, 0, &buffer[0], &neededSize);
        if (!result) {
            DWORD errorCode = GetLastError();
            if (errorCode != ERROR_INSUFFICIENT_BUFFER) {
                // TODO: Log error
                CloseHandle(hProcess);
                return {};
            }
        }

        if (neededSize < buffer.size()) {
            buffer.resize(neededSize);
            buffer.shrink_to_fit();
            CloseHandle(hProcess);
            return buffer;
        }

        buffer.resize(buffer.size() * 2);
    }
    return buffer;
}


std::vector<DWORD> getProcessIDs()
{
    DWORD aProcesses[1024], cbNeeded, cProcesses;

    // Get the list of process identifiers.
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
    {
        //std::cout << "Не удалось перечислить процессы" << std::endl;
        throw;
    }

    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);

    return std::vector<DWORD>(aProcesses, aProcesses + cProcesses);
}

bool checkFileExists(const std::string& path)
{
    return std::filesystem::exists(path);
}

std::string wstrToStr(std::wstring wstr)
{
    char mbstr[256];
    size_t num_can;
    wcstombs_s(&num_can, mbstr, sizeof(mbstr), wstr.c_str(), sizeof(mbstr) - 1);
    return std::string(mbstr, mbstr + num_can - 1);
}


std::string hexToString(const std::vector<BYTE>& vec)
{
    std::stringstream ss;
    ss << std::hex << std::uppercase; // для вывода в верхнем регистре (A-F)

    for (size_t i = 0; i < vec.size(); ++i) 
    {
        ss << std::hex << static_cast<int>(vec[i]); // 8 символов для DWORD (32 бита)
    }

    return ss.str();
}


std::vector<ProcessInfo> getProcessesInfo()
{
    std::vector<ProcessInfo> res;

    auto ids = getProcessIDs();

    for (auto id : ids)
    {
        auto path = wstrToStr(getProcessName(id).data());

        if (!checkFileExists(path)) continue;

        auto vec = getHash(path, CALG_MD5);

        res.push_back(ProcessInfo{ path, hexToString(vec), std::to_string(id) });
    }

    return res;
}
