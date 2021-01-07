#!/bin/bash

j=5
i=0

while [ ${i} -lt ${j} ]
do
   # echo "test"
    client/coap_client -m post -e "{\"msgId\":123,\"params\":{\"cleanssion\":\"true\",\"clientId\":\"1895aea0883749f39618e920968b26c7&deviceName12341895aea0883749f39618e920968b26c7&deviceName1234\",\"deviceSN\":\"deviceName1234\",\"productKey\":\"1895aea0883749f39618e920968b26c7\",\"sign\":\"406ab8e4cac6a7dac40f0e94aea6017a\",\"signMethod\":\"hmacmd5\",\"timestamp\":1597125586034},\"topic\":\"device.1895aea0883749f39618e920968b26c7.deviceName1234.iot.login\"}" coap://192.168.10.191:5683/dev.iot.login &
    let i++
done

sleep 5


