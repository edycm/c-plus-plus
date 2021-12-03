#ifndef _C_KAFKA_CONSUMER_CPP_H_
#define _C_KAFKA_CONSUMER_CPP_H_

#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include "rdkafkacpp.h"
#include "CKafkaCb.h"


using namespace std;

class CKafkaConsumer {
public:
    CKafkaConsumer();
    ~CKafkaConsumer();   

    /**
     * @brief 开启线程拉取消息 
     */
    virtual void start();

    /**
     * @brief 停止拉取消息线程
     */
    virtual void Terminate();

    /**
     * @brief 初始化消费者
     *
     * @param brokers broker ip端口信息，格式为<ip>:<port>,<ip>:<port>
     * @param topic 需要订阅的主题，格式为<topic1>,<topic2>
     * @param groupId 组id
     *
     * @returns true: 初始化成功， false: 初始化失败
     */
    virtual bool init(string brokers, string topic, string groupId);

    /**
     * @brief 处理拉取到的消息
     *
     * @param msg 拉取到的数据
     */
    virtual void DealData(RdKafka::Message* msg);
private:
    /**
     * @brief 内部处理拉取到的消息
     *
     * @param msg 拉取到的数据
     */
    void OnMessage(RdKafka::Message *msg);
private:
    RdKafka::KafkaConsumer *m_consumer;   // kafka消费者对象
    RdKafka::Conf *m_conf;                // kafka全局配置对象
    RdKafka::Conf *m_tconf;               // kafka主题配置对象
    KafkaEventCb m_kfkEvtCb;              // kafka事件回调对象
    KafkaRebalanceCb m_kfkRebCb;          // kafka重平衡回调对象
    KafkaOffsetCommitCb m_OffsetCommitCb; // kafka偏移量提交回调对象
    std::atomic<bool> m_isRunning;
    std::thread m_workThread;
};

#endif