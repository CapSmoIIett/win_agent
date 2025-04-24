#pragma once

#include <windows.h>
#include <wincrypt.h>
#include <softpub.h>
#include <wintrust.h>
#include <iostream>

#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

// список логических хранилищ. жестко задан и не меняется
const char* logicalStores[] = { "MY", "Root", "CA", "TrustedPeople", "AuthRoot", "Trust" };

bool GetCertificateFromFile(LPCWSTR filePath) {
    WINTRUST_FILE_INFO fileInfo = { 0 };
    fileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    fileInfo.pcwszFilePath = filePath;
    fileInfo.hFile = NULL;
    fileInfo.pgKnownSubject = NULL;

    WINTRUST_DATA winTrustData = { 0 };
    winTrustData.cbStruct = sizeof(WINTRUST_DATA);
    winTrustData.dwUIChoice = WTD_UI_NONE;
    winTrustData.fdwRevocationChecks = WTD_REVOKE_NONE;
    winTrustData.dwUnionChoice = WTD_CHOICE_FILE;
    winTrustData.pFile = &fileInfo;
    winTrustData.dwStateAction = WTD_STATEACTION_VERIFY;
    winTrustData.hWVTStateData = NULL;
    winTrustData.dwProvFlags = WTD_SAFER_FLAG;
    winTrustData.dwUIContext = 0;

    GUID policyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

    LONG status = WinVerifyTrust(NULL, &policyGUID, &winTrustData);
    if (status == ERROR_SUCCESS) {
        std::wcout << L"File is signed and trusted: " << filePath << std::endl;
        // Здесь можно дополнительно получить сертификат из winTrustData.hWVTStateData
        // и вывести информацию о нем.
        return true;
    }
    else {
        std::wcout << L"File is not signed or not trusted: " << filePath << std::endl;
        return false;
    }
}


std::vector<std::string> GetCertificateSubjects(const char* storeName)
{
    std::vector<std::string> certSubjects;

    HCERTSTORE hStore = CertOpenSystemStoreA(NULL, storeName);
    if (!hStore)
    {
        std::cerr << "Не удалось открыть хранилище: " << storeName << ", ошибка: " << GetLastError() << std::endl;
        return certSubjects; // пустой вектор
    }

    PCCERT_CONTEXT pCertContext = NULL;

    while ((pCertContext = CertEnumCertificatesInStore(hStore, pCertContext)) != NULL)
    {
        char subjectName[256] = { 0 };

        if (CertGetNameStringA(
            pCertContext,
            CERT_NAME_SIMPLE_DISPLAY_TYPE,
            0,
            NULL,
            subjectName,
            sizeof(subjectName)))
        {
            certSubjects.emplace_back(subjectName);
        }
        else
        {
            std::cerr << "Не удалось получить имя сертификата." << std::endl;
        }
    }

    CertCloseStore(hStore, 0);

    return certSubjects;
}

