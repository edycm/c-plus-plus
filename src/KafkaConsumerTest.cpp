#include "CKafkaConsumer.h"
#include "Log.h"

class MyKafkaConsumer : public CKafkaConsumer {
public:
    virtual void DealData(RdKafka::Message* msg) {
        LOGINFO("MyKafkaConsumer deal data!");
    }
private:
};

int main() {
    LOGINFO(RdKafka::version_str());
    MyKafkaConsumer consumer;
    if (!consumer.init("192.168.11.245:9092", "test,test1", "group1")) {
        return -1;
    }

    consumer.start();

    while (1) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }


    return 0;
}