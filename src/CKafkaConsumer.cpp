#include <iostream>
#include <atomic>
#include <stdint.h>
#include <ctime>
#include <stdlib.h>

#include "CKafkaConsumer.h"
#include "Log.h"
#include "CKafkaCb.h"

void split(const string& s, vector<string>& tokens, const string& delimiters) {
    string::size_type lastPos = s.find_first_not_of(delimiters, 0);
    string::size_type pos = s.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos) {
        tokens.push_back(s.substr(lastPos, pos - lastPos));
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
}

CKafkaConsumer::CKafkaConsumer()    
    :m_isRunning(false),
    m_consumer(nullptr), 
    m_conf(nullptr), 
    m_tconf(nullptr) {

}

CKafkaConsumer::~CKafkaConsumer() {
    LOGINFO("CKafkaConsumer delete!" ); 
    if (m_consumer) {
        delete m_consumer;
        m_consumer = nullptr;
    }

    if (m_conf) {
        delete m_conf;
        m_conf = nullptr;
    }

    if (m_tconf) {
        delete m_tconf;
        m_tconf = nullptr;
    }
}

bool CKafkaConsumer::init(string brokers, string topic, string groupId) {
    string errstr;
    m_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

    if (m_conf->set("metadata.broker.list", brokers, errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set metadata.broker.list failed :" << errstr.c_str());
        return false;
    }
    if (m_conf->set("group.id", groupId, errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set group.id failed :" << errstr.c_str());
        return false;
    }

    if (m_conf->set("event_cb", &m_kfkEvtCb, errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set event_cb failed :" << errstr.c_str());
        return false;
    }

    if (m_conf->set("rebalance_cb", &m_kfkRebCb, errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set rebalance_cb failed :" << errstr.c_str());
        return false;
    }
    if (m_conf->set("offset_commit_cb", &m_OffsetCommitCb, errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set offset_commit_cb failed :" << errstr.c_str());
        return false;
    }

    if (m_conf->set("client.id", "0", errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set client.id failed :" << errstr.c_str());
        return false;
    }

    //roundrobin/range/cooperative-sticky
    if (m_conf->set("partition.assignment.strategy", "range", errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set auto.offset.reset failed :" << errstr.c_str());
        return false;
    }

    if (m_conf->set("enable.auto.commit", "false", errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set enable.auto.commit failed :" << errstr.c_str());
        return false;
    }
    if (m_conf->set("auto.commit.interval.ms", "2000", errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set auto.commit.interval.ms failed :" << errstr.c_str());
        return false;
    }

    m_tconf = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);

    if (m_tconf->set("auto.offset.reset", "earliest", errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set auto.offset.reset failed :" << errstr.c_str());
        return false;
    }
    
    if (m_conf->set("default_topic_conf", m_tconf, errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set default_topic_conf failed :" << errstr.c_str());
        return false;
    }
    
    m_consumer = RdKafka::KafkaConsumer::create(m_conf, errstr);
    if (m_consumer == nullptr) {
        LOGERROR("RdKafka::KafkaConsumer::create fail: " << errstr );
        return false;
    }

    vector<string> vTopic;
    split(topic, vTopic, ",");
    LOGINFO("RdKafka Message : topic: " << topic );
    RdKafka::ErrorCode errCode = m_consumer->subscribe(vTopic);
    if (errCode != RdKafka::ERR_NO_ERROR) {
        LOGERROR("RdKafka::KafkaConsumer::subscribe fail: " << RdKafka::err2str(errCode));
        return false;
    }

    LOGINFO("kafka consumer init finish.");
    return true;
}


void CKafkaConsumer::start() {
    m_workThread = std::thread([&]{
        LOGINFO("kafka consumer start consume messages. m_isRunning: " << m_isRunning);

        while (!m_isRunning) {
            RdKafka::Message *msg = m_consumer->consume(300);
            OnMessage(msg);
            delete msg; 
        }

        m_consumer->close();
        LOGINFO("kafka consumer close.");
    });
}

void CKafkaConsumer::OnMessage(RdKafka::Message *msg)
{
    const RdKafka::Headers *headers;
    string topicName = msg->topic_name();
    // LOGDEBUG("RdKafka Message consumer! topicName" << topicName);
    static int m_nRecCnt = 0;
    switch (msg->err()) {
        case RdKafka::ERR__TIMED_OUT:
            // //LOGDEBUG("RdKafka Message: Timeout!");
        break;
        case RdKafka::ERR_NO_ERROR:
            /* Real message */
            LOGDEBUG("RdKafka Message: topic[" << msg->topic_name() << "] partion["<< msg->partition() << "] offset[" << msg->offset() << "]");
            if (msg->key()) {
                LOGDEBUG("RdKafka Message Key : " << *msg->key());
            }
            headers = msg->headers();
            if (headers) {
                vector<RdKafka::Headers::Header> hdrs = headers->get_all();
                for (size_t i = 0 ; i < hdrs.size() ; i++) {
                    const RdKafka::Headers::Header hdr = hdrs[i];
                    if (hdr.value() != NULL) {
                        LOGDEBUG("RdKafka Message Header : " << hdr.key() << " = \"" << (char*)hdr.value() << "\"");
                    }
                    else {
                        LOGDEBUG("RdKafka Message Header : " << hdr.key() << " = NULL.");
                    }
                }
            }

            //DealData(msg->payload(), msg->len());
            DealData(msg);

            //提交offset比实际靠后，后续修改。
            // m_consumer->commitSync();
            // 异步可在回调中看到结果
            m_consumer->commitAsync();
        break;
        case RdKafka::ERR__PARTITION_EOF:
            /* Last message */
            LOGDEBUG("RdKafka Message : Partion eof!");
        break;
        case RdKafka::ERR__UNKNOWN_TOPIC:
        case RdKafka::ERR__UNKNOWN_PARTITION:
            LOGERROR("RdKafka Message : Consume failed: " << msg->errstr());
        break;
        default:
            /* Errors */
            LOGERROR("RdKafka Message : Consume failed: " << msg->errstr());
    }
}

void CKafkaConsumer::Terminate() {
    m_isRunning = false;
    
    if (m_workThread.joinable()) {
        m_workThread.join();       
    }
}

void CKafkaConsumer::DealData(RdKafka::Message* msg) {
    LOGINFO("data: " << std::string((char*)msg->payload(), msg->len()));
}