#include "CoapServer.h"
#include <iostream>
#include "Semaphore.h"
#include <signal.h>

using namespace iot;

int main()
{
	CoapServer server;
	CoapServerCfg param = {"5683", "::", 5, 5, 20, 20, true};
	if (!server.Init(&param))
	{
		std::cout << "server init error" << std::endl;
		return -1;
	}

	server.Start();


	static Semaphore sem;
	signal(SIGINT, [](int) { sem.post(); });
	sem.wait();
	server.Stop();
	return 0;
}