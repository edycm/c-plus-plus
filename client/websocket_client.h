#ifndef _WEBSOCKET_CLIENT_H_
#define _WEBSOCKET_CLIENT_H_

#include <websocketpp/client.hpp>

#include <mutex>
#include <condition_variable>

#ifndef TLS
#include <websocketpp/config/asio_no_tls_client.hpp>
typedef websocketpp::client<websocketpp::config::asio_client> Client;
#else
#include <websocketpp/config/asio_client.hpp>
typedef websocketpp::client<websocketpp::config::asio_tls_client> Client;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> ContextPtr;
#endif

class WSClient {
public:
	WSClient();
	~WSClient();
	void on_open(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl,\
		Client::message_ptr);
	void on_close(websocketpp::connection_hdl);
	void on_fail(websocketpp::connection_hdl hdl);
#ifdef TLS
	bool verify_subject_alternative_name(const char* hostname, X509* cert);
	bool verify_common_name(char const* hostname, X509* cert);
	bool verify_certificate(const char* hostname, bool preverified, boost::asio::ssl::verify_context& ctx);
	ContextPtr on_tls_init(const char* hostname, const char* ca_file, websocketpp::connection_hdl);
#endif
	void start(const std::string& hostname, const std::string& port, std::string ca_file);
	void stop();
	void send_message();

private:
	Client m_client;
	websocketpp::connection_hdl m_hdl;

	std::atomic<bool> m_connect_state;
	std::atomic<bool> m_is_running;
	std::mutex m_mutex;
	std::condition_variable m_cond;

	std::thread m_t_send_message;
};


#endif