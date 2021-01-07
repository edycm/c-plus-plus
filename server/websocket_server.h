#ifndef _WEBSOCKET_SERVER_H_
#define _WEBSOCKET_SERVER_H_

#include <websocketpp/server.hpp>
#include <mutex>
#include <condition_variable>
#include <list>
#include <set>

#ifndef TLS
#include <websocketpp/config/asio_no_tls.hpp>
typedef websocketpp::server<websocketpp::config::asio> Server;
#else
#include <websocketpp/config/asio.hpp>
typedef websocketpp::server<websocketpp::config::asio_tls> Server;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> ContextPtr;
enum class tls_mode:char {
    MOZILLA_INTERMEDIATE = 1,
    MOZILLA_MODERN = 2
};
#endif

enum class REQUEST_TYPE {
	SUBSCRIBE,
	UNSUBSCRIBE,
	MESSAGE
};

struct Request {
	Request(const REQUEST_TYPE& type, const websocketpp::connection_hdl& hdl, \
		const Server::message_ptr& message):\
		m_type(type), m_hdl(hdl), m_message(message) {}
	~Request() {}

	REQUEST_TYPE m_type;
	websocketpp::connection_hdl m_hdl;
	Server::message_ptr m_message;
};

class WSServer {
public:
	WSServer();
	~WSServer();
	void on_message(Server* s, websocketpp::connection_hdl hdl, \
		Server::message_ptr msg);
	void on_open(Server* server, websocketpp::connection_hdl hdl);
	void on_close(Server* server, websocketpp::connection_hdl hdl);
	void on_progress();
#ifdef TLS
	std::string get_password() {
		return "test";
		//return "";
	}
	ContextPtr on_tls_init(const char* cert_file, const char* dh_file, tls_mode mode, websocketpp::connection_hdl hdl);
#endif
	void run(const char* port, const char* cert_file, const char* dh_file);
	void on_push();
	void stop();
private:
	std::mutex m_mutex;
	std::condition_variable m_cond;
	std::list<Request> m_request;
	Server m_server;
	std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> m_set_hdl;
	std::atomic<bool> m_is_running;
	std::thread m_process;
	std::thread m_push;
};

inline std::string get_current_time() {
	//std::string current_time;
	time_t t;
	t = time(NULL);
	tm tt = *localtime(&t);
	char c_time[20] = { 0 };
	sprintf(c_time, "%d-%02d-%02d %02d:%02d:%02d", tt.tm_year + 1900, tt.tm_mon, tt.tm_mday, tt.tm_hour, tt.tm_min, tt.tm_sec);

	return std::string(c_time);
}

#endif