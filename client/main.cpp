#include "websocket_client.h"
#include "semaphore.h"
#include <sys/stat.h>

#ifndef WIN32
#include <signal.h>
#endif

int main(int argc, char* argv[])
{
	std::string hostname = "127.0.0.1", port = "9002", ca_file = "ca.pem";
	if (argc == 4) {
		hostname = argv[1];
		port = argv[2];
		ca_file = argv[3];
	} else {
		std::cout << "Usage: " << argv[0] << " [hostname] [port] [ca_file]" << std::endl;
		return -1;
	}

	WSClient client;
	std::thread t = std::thread([&]() {
		try {

			client.start(hostname, port, ca_file);
		}
		catch (websocketpp::exception const& e) {
			std::cout << e.what() << std::endl;
		}
		catch (const std::exception& e) {
			std::cout << e.what() << std::endl;
		}
		catch (...) {
			std::cout << "unknown error" << std::endl;
		}
		});

#ifndef WIN32
	static Semaphore sem;
	signal(SIGINT, [](int) { sem.post(); });
	sem.wait();
	client.stop();
#endif
	
	if (t.joinable())
		t.join();
#ifdef WIN32
	system("pause");
#endif
	return 0;
}