# mqtt_test


    ./build.sh

    cd bin
    
    ./generate_ca.sh

    ./mqtt_publish ./ca/ca.crt ./ca/server.crt ./ca/server.key ./ca/ca 127.0.0.1 1883
    
    ./mqtt_consumer ./ca/ca.crt ./ca ./ca/client.crt ./ca/client.key 127.0.0.1 1883
