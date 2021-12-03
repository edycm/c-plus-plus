#ifndef _C_KAFKA_PRODUCER_H_
#define _C_KAFKA_PRODUCER_H_

#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include "CKafkaConsumer.h"
#include "CKafkaCb.h"
#include "rdkafkacpp.h"

using namespace std;

class CKafkaProducer {
public:
    /**
     *构造函数
     */
    CKafkaProducer();
    /**
     *析构函数
     */
    ~CKafkaProducer();

    /**
     * @brief 开启线程拉取消息 
     */
    virtual void start();

    /**
     * @brief 停止拉取消息线程
     */
    virtual void Terminate();
	
    /**
     * @brief 初始化生产者
     *
     * @param brokers broker ip端口信息，格式为<ip>:<port>,<ip>:<port>
     *
     * @returns true: 初始化成功， false: 初始化失败
     */
    virtual bool init(string brokers = "192.168.11.245:9092");

    /**
     * @brief 生产者发送消息
     *
     * @param topicName 主题名称
     * @param strKey key值
     * @param payload 发送的消息数据
     * @param len 发送的消息数据长度
     */
    virtual RdKafka::ErrorCode send(const std::string topicName, string strKey, void *payload, size_t len);
public:
    bool m_isPollThrStop;
private:

    KafkaDeliveryReportCb m_DeliveryReportCb;
    KafkaEventCb m_kfkEvtCb;            // kafka事件回调对象
    RdKafka::Producer* m_pProducer;       // kafka生产者
    RdKafka::Conf *m_conf;              // kafka全局配置对象
    int m_nPartition;
    std::atomic<bool> m_isRunning;
    std::thread m_workThread;
};

#endif
