#ifndef _C_KAFKA_CB_H_
#define _C_KAFKA_CB_H_

#include "rdkafkacpp.h"


using namespace std;
class KafkaDeliveryReportCb : public RdKafka::DeliveryReportCb {
public:
    void dr_cb(RdKafka::Message &message);
};

class KafkaEventCb : public RdKafka::EventCb {
public:
    void event_cb(RdKafka::Event &event);
};

class KafkaRebalanceCb : public RdKafka::RebalanceCb {

public:
    void rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
        vector<RdKafka::TopicPartition*> &partitions);
};

class KafkaOffsetCommitCb : public RdKafka::OffsetCommitCb {
public:
    virtual void offset_commit_cb(RdKafka::ErrorCode err,
        vector<RdKafka::TopicPartition*>&offsets);
};

string part_list(const std::vector<RdKafka::TopicPartition*>&partitions);

#endif
