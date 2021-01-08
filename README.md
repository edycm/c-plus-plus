# mqtt_test



    ./mqtt_publish ./conf/mqttCA/ca/ca.crt ./conf/mqttCA/server/server.crt ./conf/mqttCA/server/server.key ./conf/mqttCA/ca 127.0.0.1 1883
    
    ./mqtt_consumer ./conf/mqttCA/ca/ca.crt ./conf/mqttCA/ca ./conf/mqttCA/client/client.crt ./conf/mqttCA/client/client.key 127.0.0.1 1883
