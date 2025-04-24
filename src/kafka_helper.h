#pragma once


#include <include/kafka/KafkaProducer.h>

std::string getenv_s(std::string_view str)
{
    char* pValue = nullptr;
    size_t len;

    auto err = _dupenv_s(&pValue, &len, str.data());

    if (err) {
        std::cout << "Error getting environment variable." << std::endl;
        throw;
    }

    if (nullptr == pValue)
    {
        std::cout << "Error getting environment variable. String empty" << std::endl;
        throw;
    }

    return std::string(pValue, len);
}

#include "future"

void sendKafkaMessage(const std::string_view ip_port, 
    const std::string_view topic, 
    const std::string_view msg)
{
    using namespace kafka;
    using namespace kafka::clients::producer;

    // Prepare the configuration
    const Properties props({ 
        {"bootstrap.servers", std::string(ip_port)}
        //{"message.timeout.ms", std::string("5000")} 
        });

    // Create a producer
    KafkaProducer producer(props);

    ProducerRecord record(Topic(topic), Key("from c++"), Value(msg.data(), msg.size()));

    // Prepare delivery callback
    auto deliveryCb = [](const RecordMetadata& metadata, const Error& error) {
        if (!error) {
            std::cout << std::endl << "Message delivered: " << metadata.toString() << std::endl << std::endl;
        }
        else {
            std::cout << std::endl << "Message failed to be delivered: " << error.message() << std::endl << std::endl;
        }
        };

    // Send a message
    producer.send(record, deliveryCb);

    // Close the producer explicitly(or not, since RAII will take care of it)
    producer.flush(std::chrono::seconds(1));
    producer.close();
    //*/
}

/*
using namespace kafka;
using namespace kafka::clients::producer;

const std::string brokers = "172.17.110.42:9092";
const Topic topic = "metrics";

// Configuration with more options
const Properties props({
    {"bootstrap.servers", brokers},
    {"message.timeout.ms", std::string("5000")}, // 5 seconds timeout
    {"queue.buffering.max.ms", std::string("100")} // reduce latency
    });

KafkaProducer producer(props);

ProducerRecord record(topic, Key("from c++"), Value(msg.data(), msg.size()));

// Use a promise to wait for delivery
std::promise<void> deliveryPromise;
auto deliveryFuture = deliveryPromise.get_future();

auto deliveryCb = [&deliveryPromise](const RecordMetadata& metadata, const Error& error) {
    if (!error) {
        std::cout << "Message delivered: " << metadata.toString() << std::endl;
    }
    else {
        std::cerr << "Message failed to be delivered: " << error.message() << std::endl;
    }
    deliveryPromise.set_value();
    };

try {
    // Send and wait for delivery
    producer.send(record, deliveryCb);

    // Wait for delivery callback (with timeout)
    if (deliveryFuture.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
        std::cerr << "Timeout waiting for message delivery" << std::endl;
    }

    // Flush with reasonable timeout
    producer.flush(std::chrono::seconds(5));
}
catch (const KafkaException& e) {
    std::cerr << "Kafka error: " << e.what() << std::endl;
}

// Close will be called automatically by destructor
*/