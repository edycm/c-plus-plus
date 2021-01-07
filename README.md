简介

    基于websocketpp测试ssl加密通信


依赖

    需要openssl和boost


编译

    cd websocket_test

    mkdir build

    cd build

    make


运行

    server

    ./bin/websocket_server ../ca/server.pem ../ca/dh.pem 9002


    client

    ./bin/websocket_client 192.168.10.37 9002 ../ca/ca.pem
