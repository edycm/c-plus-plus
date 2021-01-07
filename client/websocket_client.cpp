#include "websocket_client.h"
#include <openssl/asn1.h>

WSClient::WSClient():m_hdl(websocketpp::connection_hdl()){
	m_client.set_access_channels(websocketpp::log::alevel::none);
	m_client.clear_access_channels(websocketpp::log::alevel::none);

	m_client.init_asio();

	m_client.set_open_handler(std::bind(&WSClient::on_open, this, std::placeholders::_1));
	m_client.set_message_handler(std::bind(&WSClient::on_message, this, std::placeholders::_1, std::placeholders::_2));
	m_client.set_close_handler(std::bind(&WSClient::on_close, this, std::placeholders::_1));

	m_client.set_reuse_addr(true);

	m_connect_state = false;
	m_is_running = true;
}

WSClient::~WSClient() {
	m_is_running = false;
	m_connect_state = false;
	m_cond.notify_all();

	m_client.stop();
	if (m_t_send_message.joinable()) {
		m_t_send_message.detach();
	}
}

void WSClient::on_open(websocketpp::connection_hdl hdl) {
	std::cout << "on_open, hdl: " << hdl.lock().get() << std::endl;
	m_connect_state = true;
	m_hdl = hdl;
	std::unique_lock<std::mutex> lck(m_mutex);
	m_cond.notify_one();
}

void WSClient::on_message(websocketpp::connection_hdl hdl,\
	Client::message_ptr msg) {
	std::cout << "on_message success, hdl: " << hdl.lock().get() << ", message: " << msg.get()->get_payload() << std::endl;
}

void WSClient::on_close(websocketpp::connection_hdl hdl) {
	std::cout << "on_close, hdl: " << hdl.lock().get() << std::endl;
	m_connect_state = false;
}

void WSClient::on_fail(websocketpp::connection_hdl hdl) {
	std::cout << "on_fail, hdl: " << hdl.lock().get() << std::endl;
}

#ifdef TLS
bool WSClient::verify_subject_alternative_name(const char* hostname, X509* cert) {
	STACK_OF(GENERAL_NAME)* san_names = NULL;

	san_names = (STACK_OF(GENERAL_NAME)*) X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
	if (san_names == NULL) {
		return false;
	}

	int san_names_count = sk_GENERAL_NAME_num(san_names);

	bool result = false;

	for (int i = 0; i < san_names_count; i++) {
		const GENERAL_NAME* current_name = sk_GENERAL_NAME_value(san_names, i);

		if (current_name->type != GEN_DNS) {
			continue;
		}

#ifdef WIN32
		char const* dns_name = (char const*)ASN1_STRING_get0_data(current_name->d.dNSName);
#else
		char const* dns_name = (char const*)ASN1_STRING_data(current_name->d.dNSName);
#endif
		// Make sure there isn't an embedded NUL character in the DNS name
		if (ASN1_STRING_length(current_name->d.dNSName) != strlen(dns_name)) {
			break;
		}
		// Compare expected hostname with the CN
		std::cout << "hostname: " << hostname << ", dns_name: " << dns_name << std::endl;
#ifdef WIN32
		result = (stricmp(hostname, dns_name) == 0);
#else
		result = (strcasecmp(hostname, dns_name) == 0);
#endif
	}
	sk_GENERAL_NAME_pop_free(san_names, GENERAL_NAME_free);

	return result;
}

/// Verify that the certificate common name matches the given hostname
bool WSClient::verify_common_name(char const* hostname, X509* cert) {
	// Find the position of the CN field in the Subject field of the certificate
	int common_name_loc = X509_NAME_get_index_by_NID(X509_get_subject_name(cert), NID_commonName, -1);
	if (common_name_loc < 0) {
		return false;
	}

	// Extract the CN field
	X509_NAME_ENTRY* common_name_entry = X509_NAME_get_entry(X509_get_subject_name(cert), common_name_loc);
	if (common_name_entry == NULL) {
		return false;
	}

	// Convert the CN field to a C string
	ASN1_STRING* common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
	if (common_name_asn1 == NULL) {
		return false;
	}

#ifdef WIN32
	char const* common_name_str = (char const*)ASN1_STRING_get0_data(common_name_asn1);
#else
	char const* common_name_str = (char const*)ASN1_STRING_data(common_name_asn1);
#endif

	// Make sure there isn't an embedded NUL character in the CN
	if (ASN1_STRING_length(common_name_asn1) != strlen(common_name_str)) {
		return false;
	}

	// Compare expected hostname with the CN
	std::cout << "hostname: " << hostname << ", common_name_str: " << common_name_str << std::endl;
#ifdef WIN32
	return(stricmp(hostname, common_name_str) == 0);
#else
	return (strcasecmp(hostname, common_name_str) == 0);
#endif
}


bool WSClient::verify_certificate(const char* hostname, bool preverified, boost::asio::ssl::verify_context& ctx) {
	int depth = X509_STORE_CTX_get_error_depth(ctx.native_handle());
	if (depth == 0 && preverified) {
		X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());

		char subject_name[256];
		X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
		std::cout << "Verifying " << subject_name << "\n";

		if (verify_subject_alternative_name(hostname, cert)) {
			return true;
		}
		else if (verify_common_name(hostname, cert)) {
			return true;
		}
		else {
			return false;
		}
	}

	return preverified;
}

ContextPtr WSClient::on_tls_init(const char* hostname, const char* ca_file, websocketpp::connection_hdl) {
	namespace asio = websocketpp::lib::asio;
	ContextPtr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23);

	try {
		ctx->set_options(boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2 |
			boost::asio::ssl::context::no_sslv3 |
			boost::asio::ssl::context::single_dh_use);

		ctx->set_verify_mode(boost::asio::ssl::verify_peer);
		//ctx->set_verify_mode(boost::asio::ssl::verify_none);

		ctx->set_verify_callback(bind(&WSClient::verify_certificate, this, hostname, std::placeholders::_1, std::placeholders::_2));
		
		// Here we load the CA certificates of all CA's that this client trusts.
		ctx->load_verify_file(ca_file);
	}
	catch (std::exception& e) {
		std::cout << e.what() << std::endl;
	}
	return ctx;
}

#endif

void WSClient::send_message() {
	while (true)
	{
		std::unique_lock<std::mutex> lck(m_mutex);
		m_cond.wait(lck, [&]() {
			return !m_is_running || m_connect_state;
			});
		lck.unlock();
		if (!m_is_running)
			break;

		std::string user_input;
		std::cout << "Please enter the message you want to send: " << std::endl;
		std::getline(std::cin, user_input);
		m_client.send(m_hdl, user_input, websocketpp::frame::opcode::text);
	}
}

void WSClient::start(const std::string& hostname, const std::string& port, std::string ca_file) {
	std::string uri;
#ifdef TLS
	uri = "wss://" + hostname + ":" + port;
	//uri = "wss://192.168.10.37:9002";
	if (ca_file.empty())
		ca_file = "ca.pem";
	m_client.set_tls_init_handler(std::bind(&WSClient::on_tls_init, this, hostname.c_str(), ca_file.c_str(), std::placeholders::_1));
#else
	uri = "ws://" + hostname + ":" + port;
	//uri = "ws://192.168.10.79:8081";
#endif

	websocketpp::lib::error_code ec;
	Client::connection_ptr con = m_client.get_connection(uri, ec);
	if (ec) {
		m_client.get_alog().write(websocketpp::log::alevel::app, ec.message());
		return;
	}

	m_client.connect(con);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));
	m_t_send_message = std::thread(std::bind(&WSClient::send_message, this));

	m_client.run();
}

void WSClient::stop() {
	std::cout << "Shutdown..." << std::endl;
	m_is_running = false;
	m_connect_state = false;
	m_cond.notify_all();

	m_client.stop();
	m_t_send_message.detach();
	/*
	if (m_t_send_message.joinable())
		m_t_send_message.join();
	*/
}
