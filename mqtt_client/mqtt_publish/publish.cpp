#include "mosquitto.h"
#include "LogManager.h"
#include <thread>
#include <string>
#include <stdio.h>
#include <atomic>

//static std::atomic<bool> isRunning = true;
std::atomic_bool isRunning(true);

void my_message_callback(struct mosquitto* mosq, void* userdata, const struct mosquitto_message* message)
{
	if (message->payloadlen) 
	{
		LOG << "topic: " << message->topic << ", payload: " << static_cast<char*>(message->payload);
	}
	else {
		LOG << "topic: " << message->topic << ", payload: (null)";
	}
}

void my_connect_callback(struct mosquitto* mosq, void* userdata, int result)
{
	if (!result) 
		/* Subscribe to broker information topics on successful connect. */
		mosquitto_subscribe(mosq, NULL, "TEST", 2);
	else 
		LOG << "Connect failed";
}

void my_subscribe_callback(struct mosquitto* mosq, void* userdata, int mid, int qos_count, const int* granted_qos)
{
	LOG << "subscribed, mid: " << mid << ", granted_qos[0]: " << granted_qos[0] << ", qos_count: " << qos_count;
	for (int i = 1; i < qos_count; i++)
		LOG << i << ": " << granted_qos[i];
}

void my_log_callback(struct mosquitto* mosq, void* userdata, int level, const char* str)
{
	/* Pring all log messages regardless of level. */
	LOG << str << "log level: " << level;
}

void on_disconnect(struct mosquitto* mosq, void* userdata, int)
{
	LOG << "disconnect, reconnectting...";
	int ret = 1;
	//mosquitto_reconnect(mosq);
	while (ret && isRunning)
	{
		ret = mosquitto_reconnect(mosq);
		if (ret)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		else
			LOG << "reconnect success";
	}
}

void on_publish(struct mosquitto* mosq, void* userdata, int mid)
{
	LOG << "message id : " << mid;
}

void SetMQTTCallBack(struct mosquitto* mosq)
{
	//mosquitto_log_callback_set(mosq, my_log_callback);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
	mosquitto_disconnect_callback_set(mosq, on_disconnect);
	//mosquitto_publish_callback_set(mosq, on_publish);
}

void mqtt_publish_test()
{
	LOG << "publish start";
	bool clean_session = true;
	int ret = 1;
	struct mosquitto* mosq;
	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, clean_session, NULL);
	if (!mosq)
	{
		LOG << "mosquitto_new error";
		return;
	}

	SetMQTTCallBack(mosq);

	//mosquitto_socks5_set(mosq, "192.168.10.37", 1883, "root", "123456");
	//user name set
	mosquitto_username_pw_set(mosq, "root", "123456");

	//tls set
	//ret = mosquitto_tls_set(mosq, cfg->cafile, cfg->capath, cfg->certfile, cfg->keyfile, NULL);
	//ret = mosquitto_tls_set(mosq, ".\\ca\\ca.crt", "", ".\\server\\server.crt", ".\\server\\server.key", NULL);
	const char* cafile = "../../openssl_generate/ca/ca.crt";
	const char* server_crt = "../../openssl_generate/client/client.crt";
	const char* server_key = "../../openssl_generate/client/client.key";
	//const char* cafile = ".\\ca\\ca.crt";
	//const char* server_crt = "C:\\Users\\lenovo\\Desktop\\client\\client.crt";
	//const char* server_key = "C:\\Users\\lenovo\\Desktop\\client\\client.key";


	ret = mosquitto_tls_set(mosq, cafile, "../../openssl_generate/ca", server_crt, server_key, NULL);
	if (ret) 
	{
		if (ret == MOSQ_ERR_INVAL) {
			LOG << "Error: Problem setting TLS options: File not found.";
		}
		else {
			LOG << "Error: Problem setting TLS options: " <<  mosquitto_strerror(ret);
		}
		mosquitto_lib_cleanup();
		return;
	}
	ret = 1;

	while (ret)
	{
		//ret = mosquitto_connect(mosq, "192.168.10.37", 1883, 10);
		ret = mosquitto_connect(mosq, "192.168.10.37", 1883, 10);
		std::cout << "connect ret: " << ret << std::endl;
		if (ret)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	
	std::thread t = std::thread([&]() {
		while (isRunning)
		{
			ret = mosquitto_loop(mosq, 15, 1);
			if (ret)
			{
				LOG << "mosquitto_loop error, ret: " << ret;
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		});

	std::string s;
	while ((std::cout << "please enter: ") && std::getline(std::cin, s))
	{
		if (s == "q")
		{
			isRunning = false;
			break;
		}
		ret = mosquitto_publish(mosq, NULL, "TEST", s.size(), s.c_str(), 0, false);
		s.clear();
		LOG << "mosquitto_publish result: " << ret;
	}

	t.join();
	mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return;
}

int main()
{
	mqtt_publish_test();
	return 0;
}
