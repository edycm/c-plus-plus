#include "CKafkaProducer.h"
#include <iostream>

int main() {
    
    CKafkaProducer producer;
    if (!producer.init("192.168.11.245:9092")) {
        return -1;
    }

    producer.start();

    int i = 0;
    std::string sendData;
    while (1) {
        sendData = std::to_string(i++);
        producer.send("test", "", const_cast<char*>(sendData.data()), sendData.size());
        producer.send("test1", "", const_cast<char*>(sendData.data()), sendData.size());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}