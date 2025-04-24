

#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <string_view>

#include "ports_info.h"
#include "process_info.h"
#include "pc_Info.h"
#include "kafka_helper.h"
#include "certificate.h"
#include "multicast_search.h"

#include <nlohmann/json.hpp>


nlohmann::json read_config() {
    // Создаём пустой JSON объект
    nlohmann::json config;

    // Открываем файл для чтения
    std::ifstream file("config.txt");

    // Проверяем успешность открытия
    if (!file.is_open()) {
        throw std::runtime_error("Ошибка открытия файла config.txt");
    }

    try {
        // Читаем JSON данные из файла
        file >> config;
    }
    catch (const nlohmann::json::parse_error& e) {
        // Обрабатываем ошибки парсинга
        throw std::runtime_error("Ошибка парсинга JSON: " + std::string(e.what()));
    }

    return config;
}

int main()
{
    setlocale(LC_ALL, "RU");

    // Запускаем два потока: один для приёма сообщений, другой для отправки
    
    /*
    std::thread recvThread(receiver, [](std::string_view ip, std::string_view msg)
        {
            nlohmann::json pc_info;

            pc_info["mac"] = getMacAdress();
            pc_info["visible_ip"] = ip;


            sendKafkaMessage("172.17.110.42:9092", "test", pc_info.dump());
        });
    //

    //std::thread sendThread(sender);

    // Ждём завершения потоков (на самом деле они работают бесконечно)
    //sendThread.join();
    recvThread.detach();

    sender();
    */
    
    
    auto config = read_config();
    
    /*
    nlohmann::json config = {
        {"kafka", {
            {"ip", "172.17.110.42"},
            {"port", "9092"}
        }},
        {"break", {
            {"hour", "1"}
        }}
    };
    */
    
    nlohmann::json pc_info;

    pc_info["mac"] = getMacAdress();
    pc_info["full_device_name"] = getDeviceName();
    pc_info["host_name"] = getHostName();
    pc_info["ip_list"] = getIPs();


    sendKafkaMessage(config["kafka"]["ip"].get<std::string>() + ":" + config["kafka"]["port"].get<std::string>(), "devices", pc_info.dump());
    
    
    while (true)
    {
        auto ports = getPorts();

        for (auto port : ports)
        {
            std::cout << port.type << " " << port.localAddr << " " << port.remoteAddr << " "
                << port.state << " " << port.pid << "\n";

            nlohmann::json port_info;
            port_info["mac"] = getMacAdress();
            port_info["protocol"] = port.type;
            port_info["local_addr"] = port.localAddr;
            port_info["remote_addr"] = port.remoteAddr;
            port_info["state"] = port.state;
            port_info["pid"] = port.pid;

            sendKafkaMessage(config["kafka"]["ip"].get<std::string>() + ":" + config["kafka"]["port"].get<std::string>(), "ports", port_info.dump());
        }


        for (auto store : logicalStores)
        {
            auto certs = GetCertificateSubjects(store);

            for (const auto& name : certs)
            {
                std::cout << "Сертификат: " << name << std::endl;

                nlohmann::json cert_info;
                cert_info["mac"] = getMacAdress();
                cert_info["cert_name"] = name;
                cert_info["store"] = store;

                sendKafkaMessage(config["kafka"]["ip"].get<std::string>() + ":" + config["kafka"]["port"].get<std::string>(), "certs", cert_info.dump());
            }
        }

        auto processes = getProcessesInfo();

        for (auto process : processes)
        {
            std::cout << process.path << " (" << process.pid << ") " << process.hash_MD5 << "\n";

            nlohmann::json process_info;
            process_info["mac"] = getMacAdress();
            process_info["file_path"] = process.path;
            process_info["pid"] = process.pid;
            process_info["hashes"] = { {"MD5", process.hash_MD5} };

            sendKafkaMessage(config["kafka"]["ip"].get<std::string>() + ":" + config["kafka"]["port"].get<std::string>(), "processes", process_info.dump());
        }  

        std::this_thread::sleep_for(std::chrono::hours(config["break"]["hour"].get<int>()));
    }
    //*/

    getchar();

    isEnd = false;

    return 0;
}
