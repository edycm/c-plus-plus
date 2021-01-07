# websocket_test

cd websocket_test

mkdir build

cd build

make


server
./bin/websocket_server ../ca/server.pem ../ca/dh.pem 9002


client
./bin/websocket_client 192.168.10.37 9002 ../ca/ca.pem
