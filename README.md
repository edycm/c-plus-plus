# coap_test
本例为ubuntu16.04版本，若在其他环境运行，请自行替换lib和include为相应版本，主要依赖json、obgm/libcoap、ssl

编译

  cd websocket_test

  mkdir build

  cd build

  cmake ..

  make
  

运行

  server ./server
  
  client ./client/coap_client -m get coap://127.0.0.1/
