#include "websocket_server.h"
#include "semaphore.h"

#ifndef WIN32
#include <signal.h>
#endif

int main(int argc, char* argv[]) {
    std::string cert_file = "server.pem", dh_file = "dh.pem", port = "9002";
    if (argc == 4) {
        cert_file = argv[1];
        dh_file = argv[2];
        port = argv[3];
    } else {
        std::cout << "Usage: " << argv[0] << " [cert_file] [dh_file] [port]" << std::endl;
        return -1;
    }

    WSServer server;

    std::thread t = std::thread([&]() {
        try {
            server.run(port.c_str(), cert_file.c_str(), dh_file.c_str());
        }
        catch (websocketpp::exception const& e) {
            std::cout << e.what() << std::endl;
        }
        catch (...) {
            std::cout << "other exception" << std::endl;
        }
        });

#ifndef WIN32
    static Semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });
    sem.wait();
    server.stop();
#endif
    if(t.joinable())
        t.join();
    return 0;
}