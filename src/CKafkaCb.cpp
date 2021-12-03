#include <sstream>
#include "CKafkaCb.h"
#include "Log.h"

using namespace std;

void KafkaDeliveryReportCb::dr_cb(RdKafka::Message &message) {
    //注释为了减轻高并发的IO压力
    //LOGINFO("Message delivery for (" << message.len() << " bytes): " << message.errstr());
    // if (message.key())
    //     LOGINFO("Key: " << *(message.key()) << ";" );
}

void KafkaEventCb::event_cb(RdKafka::Event &event) {
    switch (event.type()) {
        case RdKafka::Event::EVENT_ERROR: {
            string errLevel = "ERROR(";
            if (event.fatal()) {
                errLevel = "FATAL(";
            }
            LOGERROR(errLevel << RdKafka::err2str(event.err()) << "): " << event.str() << "");
        }
        break;
        case RdKafka::Event::EVENT_STATS:
            LOGERROR("STATS: " << event.str());
        break;
        case RdKafka::Event::EVENT_LOG:
            LOGERROR("LOG-" << event.severity() << "-" << event.fac() << ": " << event.str() << "");
        break;
        default:
            LOGERROR("EVENT " << event.type() << " (" << RdKafka::err2str(event.err()) << "): " << event.str() << "");
        break;
    }
}

string part_list(const std::vector<RdKafka::TopicPartition*>&partitions) {
    std::ostringstream os;

    for (unsigned int i = 0 ; i < partitions.size() ; i++) {
        os << partitions[i]->topic() << "[" << partitions[i]->partition() << "], ";
    }
    return os.str();
}

void KafkaRebalanceCb::rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
    vector<RdKafka::TopicPartition*> &partitions) {
    //RdKafka::ERR__ASSIGN_PARTITIONS这个消息要用分区的已提交位置，重置消费位置。
    LOGWARN("RebalanceCb: " << RdKafka::err2str(err) << ": " << part_list(partitions) << "");

    RdKafka::Error *error = NULL;
    RdKafka::ErrorCode ret_err = RdKafka::ERR_NO_ERROR;

    if (err == RdKafka::ERR__ASSIGN_PARTITIONS) {
        if (consumer->rebalance_protocol() == "COOPERATIVE") {
            error = consumer->incremental_assign(partitions);
        }
        else {
            ret_err = consumer->assign(partitions);
        }
    } 
    else {
        if (consumer->rebalance_protocol() == "COOPERATIVE") {
            error = consumer->incremental_unassign(partitions);
        }
        else {
            ret_err = consumer->unassign();
        }
    }
    // LOGWARN("RebalanceCb1: " << RdKafka::err2str(err) << ": " << part_list(partitions) << "");

    if (error) {
        LOGERROR("incremental assign failed: " << error->str());
        delete error;
    }
    else if (ret_err) {
        LOGERROR("assign failed: " << RdKafka::err2str(ret_err));
    }
}


void KafkaOffsetCommitCb::offset_commit_cb(RdKafka::ErrorCode err, vector<RdKafka::TopicPartition*>&offsets) {
    LOGINFO("offset_commit_cb: " << RdKafka::err2str(err) << ": " << part_list(offsets) << "");
}