#include "CKafkaProducer.h"
#include "Log.h"

using namespace std;

/////////////////////////////////////////////////////
CKafkaProducer::CKafkaProducer()    
    :m_isRunning(false),
    m_pProducer(nullptr),
    m_conf(nullptr),
    m_nPartition(RdKafka::Topic::PARTITION_UA) {

}

CKafkaProducer::~CKafkaProducer() {    
    if(m_pProducer != NULL) {
        delete m_pProducer;       // kafka生产者
    }
    if(m_conf != NULL) {
        delete m_conf;              // kafka全局配置对象 
    } 
    RdKafka::wait_destroyed(500);  
}

bool CKafkaProducer::init(string brokers) {
    std::string errstr;
 
    m_conf = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

    if (m_conf->set("metadata.broker.list", brokers, errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set brokerlist failed :" << errstr.c_str());
        return false;
    }
 
    //m_conf->set("bootstrap.servers", brokers, errstr);
    if (m_conf->set("event_cb", &m_kfkEvtCb, errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set event_cb failed :" << errstr.c_str());
        return false;
    }

    if (m_conf->set("dr_cb", &m_DeliveryReportCb, errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set dr_cb failed :" << errstr.c_str());
        return false;
    }

    if (m_conf->set("queue.buffering.max.messages", "100000", errstr) != RdKafka::Conf::CONF_OK) {
		LOGERROR("RdKafka conf set queue.buffering.max.messages failed :" << errstr.c_str());
        return false;
	}

    if (m_conf->set("linger.ms", "10", errstr) != RdKafka::Conf::CONF_OK) {
		LOGERROR("RdKafka conf set linger.ms failed :" << errstr.c_str());
        return false;
	}

    if (m_conf->set("request.required.acks", "1", errstr) != RdKafka::Conf::CONF_OK) {
		LOGERROR("RdKafka conf set request.required.acks failed :" << errstr.c_str());
        return false;
	}

    if (m_conf->set("retries", "1", errstr) != RdKafka::Conf::CONF_OK) {
		LOGERROR("RdKafka conf set retries failed :" << errstr.c_str());
        return false;
	}
    if (m_conf->set("retry.backoff.ms", "1000", errstr) != RdKafka::Conf::CONF_OK) {
		LOGERROR("RdKafka conf set retry.backoff.ms failed :" << errstr.c_str());
        return false;
	}

    
    m_pProducer = RdKafka::Producer::create(m_conf, errstr);
    if (!m_pProducer) {
        LOGERROR("CKafkaProducer::init Failed to create producer:" << errstr);
        return false;
    }
    
    LOGINFO("CKafkaProducer::init finish!" << m_pProducer->name());
    return true;
}

RdKafka::ErrorCode CKafkaProducer::send(const std::string topicName, string strKey, void *payload, size_t len)
{
    RdKafka::ErrorCode err = m_pProducer->produce(topicName, m_nPartition, RdKafka::Producer::RK_MSG_COPY,
                     payload, len, strKey.c_str(), strKey.length(), 0, NULL);
    if (err) {
        LOGERROR("CKafkaProducer::produce failed: " << RdKafka::err2str(err));
    }
    return err;
}

void CKafkaProducer::start()
{
    m_isRunning = true;
    m_workThread = std::thread([&]{
        while (m_isRunning) {        
            if(m_pProducer != NULL) {
                m_pProducer->poll(10);
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        LOGINFO("CKafkaProducer poll thread close!");
    });
}

void CKafkaProducer::Terminate() {    
    m_isRunning = false;
    if(m_workThread.joinable()) {
        m_workThread.join();    
    }
    LOGINFO("CKafkaProducer poll thread Terminate!");
}