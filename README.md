[TOC]

## 版本信息
> - linux系统版本： CentOS 7.8.2003
> - librdkafka版本：1.6.0

## C++客户端使用

#### 生产者

##### 封装类结构

```cpp
class CKafkaProducer
{
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
     * @param strKey key值，相同key值的数据会发送到同一分区中，统一由一个消费者进行消费，可确保消息按顺序消费
     * @param payload 发送的消息数据
     * @param len 发送的消息数据长度
     */
    virtual RdKafka::ErrorCode send(const std::string topicName, string strKey, void *payload, size_t len);
};
```

##### 使用说明

- 使用示例

```cpp
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
        //发送数据
        producer.send("test", "", const_cast<char*>(sendData.data()), sendData.size());
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
```

- 初始化说明

  使用`CKafkaProducer::init`初始化配置项及回调函数

  - 添加配置项方法

    按如下方式设置：

  ```cpp
  if (m_conf->set("metadata.broker.list", brokers, errstr) != RdKafka::Conf::CONF_OK) {
      LOGERROR("RdKafka conf set metadata.broker.list failed :" << errstr.c_str());
      return false;
  }
  ```

  - 设置回调函数

    回调函数定义和实现在`CKafkaCb.cpp`、`CKafkaCb.h`中，主要是继承RdKafka命名空间中的类，可在`rdkafkacpp.h`查看其他可设置的回调函数，主要设置项：

    - `event_cb`：设置事件回调函数，事件包含错误、统计信息及日志等

    - `dr_cb`：设置生产消息的回调函数

  ```cpp
  if(m_conf->set("event_cb", &m_kfkEvtCb, errstr) != RdKafka::Conf::CONF_OK){
      LOGERROR("RdKafka conf set event_cb failed :" << errstr.c_str());
      return false;
  }
  ```

- 生产消息说明

  主要为`CKafkaProducer::send`函数，发送消息需指定主题，`strKey`可为空，代码如下：

  ```cpp
  RdKafka::ErrorCode CKafkaProducer::send(const std::string topicName, string strKey, void *payload, size_t len)
  {
      RdKafka::ErrorCode err = m_pProducer->produce(topicName, m_nPartition, RdKafka::Producer::RK_MSG_COPY,payload, len, strKey.c_str(), strKey.length(), 0, NULL);
      if (err) {
          LOGERROR("CKafkaProducer::produce failed: " << RdKafka::err2str(err));
      }
      return err;
  }
  ```

  **注意**：若存在需要顺序消费的消息，可在发送消息时设置相同的`strKey`参数，`strKey`值相同的消息数据会发送到同一分区中，然后由同一个消费者消费数据，保证消息按照顺序消费

#### 消费者

##### 封装类结构

```cpp
//class CKafkaConsumer
class CKafkaConsumer
{
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
};
```

##### 使用说明

- 使用示例

  ```cpp
  //继承CKafkaConsumer实现消息处理函数
  class MyKafkaConsumer : public CKafkaConsumer {
  public:
      virtual void DealData(RdKafka::Message* msg) {
          LOGINFO("MyKafkaConsumer deal data!");
      }
  private:
  };
  
  int main() {
      
      MyKafkaConsumer consumer;
      //初始化消费者, 可修改在init函数中添加、修改和删除配置项
      //"192.168.11.245:9092" ： broker ip端口
      //"test,test1" : 主题test和test，多个使用逗号分隔
      //"group1" ：消费者组
      if (!consumer.init("192.168.11.245:9092", "test,test1", "group1")) {
          return -1;
      }
      consumer.start();
  
      while (1) {
          std::this_thread::sleep_for(std::chrono::seconds(10));
      }
      return 0;
  }
  ```

- 初始化说明

  使用`CKafkaConsumer::init`初始化配置项及回调函数

  - 添加配置项方法

    按如下方式设置：

    ```cpp
    if (m_conf->set("metadata.broker.list", brokers, errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set metadata.broker.list failed :" << errstr.c_str());
        return false;
    }
    ```

  - 设置回调函数

    回调函数定义和实现在`CKafkaCb.cpp`、`CKafkaCb.h`中，主要是继承`RdKafka`命名空间中的类，可在`rdkafkacpp.h`查看其他可设置的回调函数，主要设置项：

    - `event_cb`：设置事件回调函数，事件包含错误、统计信息及日志等

    - `rebalance_cb`：设置消费者组重平衡回调函数，可看到分配策略为`range`、`roundrobin` 重新分配后消费者持有的分区，`cooperative-sticky`在回调中看不到分配后的结果，可在`offset_commit_cb`回调中查看
    - `offset_commit_cb`：设置消费者偏移量回调函数，可获取偏移量提交结果，可看到当前消费者持有分区列表

    ```cpp
    if (m_conf->set("event_cb", &m_kfkEvtCb, errstr) != RdKafka::Conf::CONF_OK) {
        LOGERROR("RdKafka conf set event_cb failed :" << errstr.c_str());
        return false;
    }
    ```

- 手动提交偏移量

  支持同步提交和异步提交，可在`CKafkaConsumer::OnMessage `调用完`DealData`后提交，异步提交的话可设置偏移量提交的回调函数，在回调函数中判读是否提交成功

  ```cpp
  //RdKafka::KafkaConsumer *m_consumer
  //RdKafka::Message *msg
  m_consumer->commitSync(msg);
  //异步提交 可在回调函数中看到结果
  m_consumer->commitAsync(msg);
  ```

## 使用场景

  ### 生产者


  - **保证消息的高可靠性**
  
    会影响高并发的性能
  
    **配置项：**
  
    ​	`request.required.acks`：可设置成1，也可设置成 -1或all，保证broker端接收到了消息
  
    ​	`retries`：重试次数
  
    ​	`retry.backoff.ms`：重试间隔时间
  
  - **节约网络带宽，提高吞吐量**
  
    会增加CPU的使用率，最好与broker端的压缩格式一致，不然会导致broker CPU负载增加（需转换压缩格式进行存储）
  
    **配置项：**

  ​			`compression.type`：压缩消息进行发送，可以根据实际网络带宽、磁盘空间和CPU性能进行考虑

  - **增加高并发的性能**
  
    会滞后消息的发送
  
    **配置项：**

  ​			`batch.size`：可根据实际消息大小及并发量进行配置

  ​			`linger.ms`：如10ms，不建议设置太大，越大消息滞后时间越长

  ### 消费者

  - **增加高并发的性能**
  
    可能会出现上一条的情况
  
    **配置项：**
  
    ​	`enable.auto.commit`：设置为true，不用在处理完消息后手动提交
  
    ​	`auto.commit.interval.ms`：减少自动提交的间隔，避免消息处理了但偏移量还没提交
  
  - **消费者加入新的消费者组时，可定义从何处开始消费**
  
    **配置项：**
  
    ​	`auto.offset.reset`：具体参考配置说明
  
  - **分布式服务含有多个消费者处理不同分区数据时，尽量保证负载均衡**
  
    **配置项：**
  
    ​	`partition.assignment.strategy`：具体参考配置说明
  
    ​	`auto.offset.reset`：具体参考配置说明
  
  - **分布式服务含有多个消费者处理不同分区数据时，尽量保证负载均衡**
  
    **配置项：**
  
    ​	`partition.assignment.strategy`：具体参考配置说明

## 配置参数说明

### 生产者配置参数

- `bootstrap.servers`或`metadata.broker.list`：设置kafka broker ip端口，可设置一个或多个，格式为`host1:port1,host2:port2`

- `retries`：生产者发送消息到broker失败后的重试次数，默认值为0

- `retry.backoff.ms`：发送消息失败后到重新发送消息的时间间隔，默认值为100 ms

- `buffer.memory`：设置RecordAccumulator缓存大小，默认值为32 MB

- `batch.size`：数据累积到`batch.size`大小后才会发送数据，默认值为16 KB

- `linger.ms`：数据没有达到`batch.size`大小但已超过`linger.ms`时，立即发送消息，默认值为0 ms；提高并发量时可适当增加，如10 ms


- `max.block.ms`：最大阻塞时间，RecordAccumulator缓存不足时或者元数据不可用时，发送数据到broker会阻塞或返回异常，此参数的默认值为60000 ms

- `request.timeout.ms`：等待请求响应的超时时间，默认值为30000 ms

- `request.required.acks`：指定分区中成功写入消息的副本数量
  - `0`：发送消息后立即返回成功，不等待broker端响应结果

  - `1`：发送消息后broker端分区leader成功写入后就会得到响应

  - `-1`或`all`：发送消息后不仅要等待broker端分区leader成功写入，还要等待该分区所有ISR（同步副本集）成功写入，才会得到响应
  
- `max.request.size`：发送消息的最大大小，默认值为1 MB

- `compression.type`：指定消息的压缩算法，默认值为`none`，可以配置为`gzip`、`snappy`、`lz4`、`zstd`(kafka 2.1.0开始支持，需对应的库文件)。

  网上收集的数据（可实际测试kafka支持的各压缩算法的性能）：

|  Compressor name   |  Ratio  | Compression | Decompress  |
| :----------------: | :-----: | :---------: | :---------: |
|  `zstd 1.3.4 -1`   | `2.877` | `470 MB/s`  | `1380 MB/s` |
|  `zlib 1.2.11 -1`  | `2.743` | `110 MB/s`  | `400 MB/s`  |
| `brotli 1.0.2 -0`  | `2.701` | `410 MB/s`  | `430 MB/s`  |
| `quicklz 1.5.0 -1` | `2.238` | `550 MB/s`  | `710 MB/s`  |
|   `lzo1x2.09 -1`   | `2.108` | `650 MB/s`  | `830 MB/s`  |
|    `lz4 1.8.1`     | `2.101` | `750 MB/s`  | `3700 MB/s` |
|   `snappy 1.1.4`   | `2.091` | `530 MB/s`  | `1800 MB/s` |
|    `lzf 3.6 -1`    | `2.077` | `400 MB/s`  | `860 MB/s`  |

### 消费者配置参数

- `bootstrap.servers`或`metadata.broker.list`：设置kafka broker ip端口，可设置一个或多个，格式为`host1:port1,host2:port2`

- `group.id`: 消费者组名称

- `enable.auto.commit`：是否自动提交，默认值为true

- `auto.commit.interval.ms`：自动提交的时间间隔，默认5000 ms

- `auto.offset.reset`：找不到记录的偏移量或记录的偏移量发生越界时，设置此参数确定从何处开始消费

  - `earliest`：当各分区下存在已提交的 offset 时，从提交的 offset 开始消费；无提交的 offset 时，从头开始消费。
  - `latest`：当各分区下存在已提交的 offset 时，从提交的 offset 开始消费；无提交的 offset 时，消费该分区下新产生的数据。
  - `none`：topic 各分区都存在已提交的 offset 时，从 offset 后开始消费；只要有一个分区不存在已提交的offset，则抛出异常。

- `fetch.min.bytes`：一次拉取数据请求中数据的最小大小，默认1 B

- `fetch.max.bytes`：一次拉取数据请求中数据的最大大小，默认52428800 B，即50 MB

- `fetch.max.wait.ms`：一次拉取数据请求中最大等待时间，默认500 ms

- `max.partition.fetch.bytes`：一次拉取数据请求中每个分区数据的最大大小，默认1048576 B, 即1 MB

- `max.poll.records`：一次拉取数据请求中数据的最大数量，默认500条

- `request.timeout.ms`：等待请求响应的超时时间，默认值为30000 ms

- `partition.assignment.strategy`：配置分区分配策略，默认策略range；多个消费者订阅一个主题时，建议采用range；多个消费者订阅相同列表主题时，建议采用roundrobin；多个消费者订阅不同的主题列表时，建议采用cooperative-sticky
  `tips`：消费者的顺序可通过配置`client.id`参数，如设置"0"、"1"、"2"，消费者排序就是c0、c1、c2
  - `range`：针对每个主题，对消费者、分区进行排序，按分区数是否除以消费者数进行区分：

    - 当分区数除以消费者没有余数时，按顺序依次分配，如6个分区，3个消费者：c1消费0、1分区，c2消费2、3，c3消费4、5分区
      ![在这里插入图片描述](https://img-blog.csdnimg.cn/a5094729520c46f7893813fce6b8cdc4.png)

    - 当分区数除以消费者有余数时，那么排在前面的消费者会多分配一个分区，如8个分区，3个消费者：c1消费0、1、2分区，c2消费3、4、5，c3消费6、7分区
      ![在这里插入图片描述](https://img-blog.csdnimg.cn/a147bc1227b645de8a9f7ed47b367d2f.png)


  - `roundrobin`：将所有主题的分区进行排序，然后按照消费者的排序轮询分配，如果存在消费者未订阅对应主题，即消费者的topic列表不同，在轮询时直接跳过，

    - 消费者topic列表相同，如2个topic（t0、t1）,每个topic 4个分区，三个消费者，所有主题的分区排序为t0-0、t0-1、t0-2、t0-3、t1-0、t1-1、t1-2、t1-3，消费者排序：c1、c2、c3：c1消费t0-0、t0-3、t1-2分区，c2消费t0-1、t1-0、t1-3，c3消费t0-2、t1-1分区
      	![在这里插入图片描述](https://img-blog.csdnimg.cn/d8ed64f4d6b5427bae801b59d19c151c.png)

    - 消费者topic列表不同，如2个topic（t0、t1）,每个topic 4个分区，三个消费者，所有主题的分区排序为t0-0、t0-1、t0-2、t0-3、t1-0、t1-1、t1-2、t1-3，消费者排序为c1、c2、c3，c1订阅t0，c2订阅t1，c3订阅t0、t1，则分配情况为：c1消费t0-0、t0-2分区，c2消费t1-0、t1-2，c3消费t0-1、t0-3、t1-1、t1-3分区
      ![在这里插入图片描述](https://img-blog.csdnimg.cn/d6314722b00d411f982ac2e2a326bc81.png)

  - `cooperative-sticky`：执行新的分区分配之前，尽量在上一次分配上做最小改动；目标：

    - 分区的分配尽量的均衡

    - 每一次重分配的结果尽量与上一次分配结果保持一致

      **注意**：**两个目标发生冲突时优先保证第一个目标**

      - 消费者topic列表不同，如2个topic（t0、t1）,每个topic 4个分区，三个消费者，所有主题的分区排序为t0-0、t0-1、t0-2、t0-3、t1-0、t1-1、t1-2、t1-3，消费者排序为c1、c2、c3，c1订阅t0，c2订阅t1，c3订阅t0、t1，则分配情况为：c1消费t0-0、t0-2分区，c2消费t1-0、t1-1、t1-2，c3消费t0-1、t0-3、t1-3分区
        ![在这里插入图片描述](https://img-blog.csdnimg.cn/61ce3229f52d4330965d7004e486b1ea.png)
        此时将消费者1关闭，重平衡结果为：c2消费t1-0、t1-1、t1-2、t1-3，c3消费t0-1、t0-2、t0-3、t0-3分区
        ![在这里插入图片描述](https://img-blog.csdnimg.cn/6f796dfb28d04f8fbc30dc31434f99c0.png)
