#include "websocket_client.h"
#include "semaphore.h"
#include <sys/stat.h>

#ifndef WIN32
#include <signal.h>
#endif

int main(int argc, char* argv[])
{
	WSClient client;
	std::string hostname = "127.0.0.1", port = "9002", ca_file = "ca.pem";
	if (argc == 4) {
		hostname = argv[1];
		port = argv[2];
		ca_file = argv[3];
	}
	/*
	else
	{
		std::cout << "Usage: ./argv[0] [uri] [ca_file]\nuri eg: ws://localhost:9002" << std::endl;
		return -1;
	}
	std::cout << "ca_file: " << ca_file << std::endl;
	*/
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

	system("pause");
	return 0;
}