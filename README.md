# Задачи проекта

## Основные задачи:

**Получение данных об устройстве:**
- Список процессов
- Полные пути к исполняемым файлам процессов
- Хеш исполняемого файла процесса
- Список открытых портов

**Дополнительно собранные данные:**
- Полное имя устройства
- Список IP-адресов устройства
- MAC-адрес устройства
- Программы, использующие порты
- Состояние портов (активен/закрыт и т. д.)
- Тип протокола подключения порта (TCP/UDP и др.)
- Список сертификатов на устройстве

**Регламент отправки данных:**
Все собранные данные (основные и дополнительные) отправляются на сервер Kafka раз в час.

## Разработка приложения

### 1. Сборка и управление зависимостями
- **Система сборки:** CMake
- **Пакетный менеджер:** vcpkg  
  **Преимущества:**
  - Автоматическое разрешение зависимостей
  - Прямая совместимость с CMake

- **Формат передачи данных:** JSON

**Ключевые зависимости**

| Библиотека        | Версия  | Назначение              |
|-------------------|---------|-------------------------|
| modern-cpp-kafka  | 1.1.0   | Интеграция с Apache Kafka |
| nlohmann-json     | 3.11.2  | Обработка JSON          |

### 2. Получение информации о портах:
- `GetExtendedTcpTable` – WinAPI-функция для получения таблицы TCP-соединений, включая локальные/удаленные адреса, порты, состояния и PID процессов.
- `GetExtendedUdpTable` – аналогичная функция для получения таблицы UDP-портов (локальные адреса, порты и PID процессов).

**Пример кода:**
```cpp
if ((dwRetVal = GetExtendedUdpTable(pUdpTable, &dwSize, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0)) == NO_ERROR)
{
    for (i = 0; i < (int)pUdpTable->dwNumEntries; i++)
    {
        IpAddr.S_un.S_addr = (u_long)pUdpTable->table[i].dwLocalAddr;
        strcpy_s(szLocalAddr, sizeof(szLocalAddr), inet_ntoa(IpAddr));

        DWORD pid = pUdpTable->table[i].dwOwningPid;

        res.push_back(PortInfo{
           "UDP",  // type
           std::string(szLocalAddr) + ":" + std::to_string(ntohs((u_short)pTcpTable->table[i].dwLocalPort)), // addr
           std::to_string(pid) // pid
        });
    }
}
```

3. Работа с процессами:
EnumProcesses – WinAPI-функция для получения списка идентификаторов процессов (PID), работающих в системе. Возвращает массив PID и их количество.

**Пример кода:**
```c++
DWORD aProcesses[1024], cbNeeded, cProcesses;

if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
{
   ///…
}

cProcesses = cbNeeded / sizeof(DWORD);

return std::vector<DWORD>(aProcesses, aProcesses + cProcesses);
```

4. Получение хэша (CryptoAPI)
CryptoAPI — это набор функций Windows, предоставляющих криптографические услуги, такие как хэширование, шифрование, подпись и работа с сертификатами. В данном проекте CryptoAPI используется для вычисления хэшей исполняемых файлов (например, SHA-1, SHA-256, MD5).

SCP (Service Control Manager, SCM) — компонент Windows, управляющий службами (сервисами).

Алгоритм работы:

1. Получаем список процессов (через EnumProcesses).

2. Находим исполняемые файлы (через OpenProcess + GetModuleFileNameEx).

3. Вычисляем их хэши (CryptoAPI):

    - Открываем файл (CreateFile + ReadFile)

    - Инициализируем CSP (CryptAcquireContext)

    - Создаём хэш-объект (CryptCreateHash)

    - Читаем файл и хэшируем его (CryptHashData в цикле)

    - Получаем итоговый хэш (CryptGetHashParam)

    - Освобождаем ресурсы (CryptDestroyHash, CryptReleaseContext)
