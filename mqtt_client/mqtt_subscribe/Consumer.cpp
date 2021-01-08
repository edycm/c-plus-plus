#include "mosquitto.h"
#include "LogManager.h"
#include <thread>
#include <string>

void my_message_callback(struct mosquitto* mosq, void* userdata, const struct mosquitto_message* message)
{
	if (message->payloadlen)
		LOG << "topic: " << message->topic << ", payload: " << static_cast<char*>(message->payload);
	else 
		LOG << "topic: " << message->topic << ", payload: (null)";
}


void my_subscribe_callback(struct mosquitto* mosq, void* userdata, int mid, int qos_count, const int* granted_qos)
{
	int i;
	LOG << "subscribed, mid: " << mid << ", granted_qos[0]: " << granted_qos[0] << ", qos_count: " << qos_count;
	for (i = 1; i < qos_count; i++) 
		LOG << i << ": " <<  granted_qos[i];
}

void my_connect_callback(struct mosquitto* mosq, void* userdata, int result)
{
	LOG << "result: " << result;
	if (!result)
	{
		LOG << "resubscribe";
		mosquitto_subscribe(mosq, NULL, "TEST", 2);
	}
	else
	{
		LOG << "Connect failed, result: " << result;
	}
}

void on_disconnect(struct mosquitto* mosq, void* userdata, int)
{
	LOG << "disconnect, reconnectting...";
	int ret = 1;	
	while (ret)
	{
		ret = mosquitto_reconnect(mosq);
		if (ret)
		{
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		/*
		else
		{
			LOG << "resubscribe";
			mosquitto_subscribe(mosq, NULL, "TEST", 0);
		}
		*/
	}
	LOG << "send reconnect message success";
}

void mqtt_consumer_test(const char* cafile, const char* capath, const char* certfile, const char* key, const char* ip, const char* port)
{
	LOG << "consumer start";
	bool clean_session = true;
	int ret = 1;
	struct mosquitto* mosq;
	bool isRunning = false;

	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, clean_session, NULL);
	if (!mosq)
	{
		LOG << "mosquitto_new error";
		return;
	}

	//
	//mosquitto_log_callback_set(mosq, my_log_callback);
	mosquitto_connect_callback_set(mosq, my_connect_callback);
	mosquitto_message_callback_set(mosq, my_message_callback);
	mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
	mosquitto_disconnect_callback_set(mosq, on_disconnect);
	mosquitto_username_pw_set(mosq, "root", "123456");

	ret = mosquitto_tls_set(mosq, cafile, capath, \
		certfile, key, NULL);
	if (ret)
	{
		if (ret == MOSQ_ERR_INVAL) {
			LOG << "Error: Problem setting TLS options: File not found.";
		}
		else {
			LOG << "Error: Problem setting TLS options: " << mosquitto_strerror(ret);
		}
		mosquitto_lib_cleanup();
		return;
	}

	while (true)
	{
		ret = mosquitto_connect_bind_v5(mosq, ip, atoi(port), 10, NULL, NULL);
		if (ret)
		{
			LOG << "connect error, ret: " << ret;
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}
		else
		{
			LOG << "connect success";
			break;
		}
	}
	//mosquitto_subscribe(mosq, NULL, "TEST", 0);
	while (true)
	{
		ret = mosquitto_loop(mosq, 15, 1);
		if (ret) //disconnect error is 7;
		{
			LOG << "mosquitto_loop error, ret: " << ret;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
		
	}

	mosquitto_disconnect(mosq);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return;
}

int main(int argc, char*argv[])
{
	if (argc != 7)
	{
		std::cout << "Usage: " << argv[0] << " [cafile] [capath] [certfile] [key] [ip] [port]" << std::endl;
		return 0;
	}

	mqtt_consumer_test(argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);
	return 0;
}
