#include <iostream>
#include <vector>
#include <string>
#include <time.h>
#include "websocket_server.h"

// Define a callback to handle incoming messages
WSServer::WSServer() {
    // Create a server endpoint
    //Server echo_server;

    // Set logging settings
    //m_server.set_access_channels(websocketpp::log::alevel::all);
    //m_server.clear_access_channels(websocketpp::log::alevel::frame_payload);
    m_server.set_access_channels(websocketpp::log::alevel::none);
    m_server.clear_access_channels(websocketpp::log::alevel::none);
    // Initialize Asio
    m_server.init_asio();
    // Register our message handler
    m_server.set_message_handler(std::bind(&WSServer::on_message, this, &m_server, std::placeholders::_1, std::placeholders::_2));
    m_server.set_open_handler(std::bind(&WSServer::on_open, this, &m_server, std::placeholders::_1));
    m_server.set_close_handler(std::bind(&WSServer::on_close, this, &m_server, std::placeholders::_1));

    m_server.set_reuse_addr(true);

    m_is_running = true;
}

WSServer::~WSServer() {

}

void WSServer::on_message(Server* s, websocketpp::connection_hdl hdl, Server::message_ptr msg) {
    std::unique_lock<std::mutex> lck(m_mutex);
    std::cout << "on_message called with hdl: " << hdl.lock().get()
        << " and message: " << msg->get_payload()
        << std::endl;
    m_request.emplace_back(Request(REQUEST_TYPE::MESSAGE, hdl, msg));
    m_cond.notify_one();
}

void WSServer::on_open(Server* server, websocketpp::connection_hdl hdl) {
    //vec_hdl.emplace_back(hdl);
    std::unique_lock<std::mutex> lck(m_mutex);
    std::cout << "have client connected, hdl: " << hdl.lock().get() << std::endl;
    m_request.emplace_back(Request(REQUEST_TYPE::SUBSCRIBE, hdl, Server::message_ptr()));
    m_cond.notify_one();
}

void WSServer::on_close(Server* server, websocketpp::connection_hdl hdl) {
    std::unique_lock<std::mutex> lck(m_mutex);
    std::cout << "have client closed, hdl: " << hdl.lock().get() << std::endl;
    m_request.emplace_back(Request(REQUEST_TYPE::UNSUBSCRIBE, hdl, Server::message_ptr()));
    m_cond.notify_one();
}

void WSServer::on_push() {
    int i = 1;
    while (m_is_running)
    {
        if (!(i++ % 10)) {
            std::cout << "vec_hdl size: " << m_set_hdl.size() << std::endl;
            i = 1;
            std::unique_lock<std::mutex> lck(m_mutex);
            std::for_each(m_set_hdl.begin(), m_set_hdl.end(), \
                [&](std::set< websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>>::value_type value) {
                    m_server.send(value, get_current_time() + ": server Timed push!", websocketpp::frame::opcode::text);
                });
            //lck.unlock();
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void WSServer::on_progress() {
    while (m_is_running)
    {
        //std::cout << "m_request.size(): " << m_request.size() << std::endl;
        std::unique_lock<std::mutex> lck(m_mutex);
        m_cond.wait(lck, [&]() {
            return !m_is_running || !m_request.empty();
            });
        if (!m_is_running)
            break;
        Request req = m_request.front();
        m_request.pop_front();
        lck.unlock();
        //std::cout << "req.m_type: " << (int)req.m_type << std::endl;
        switch (req.m_type)
        {
            case REQUEST_TYPE::SUBSCRIBE:
            {
                m_set_hdl.insert(req.m_hdl);
            }
            break;
            case REQUEST_TYPE::UNSUBSCRIBE:
            {
                m_set_hdl.erase(req.m_hdl);
            }
            break;
            case REQUEST_TYPE::MESSAGE:
            {
                std::lock_guard<std::mutex> grard(m_mutex);
                std::for_each(m_set_hdl.begin(), m_set_hdl.end(), \
                    [&](std::set< websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>>::value_type value) {
                        std::cout << "send message, hdl: " << value.lock().get() << ", message: " << req.m_message.get()->get_payload() << std::endl;
                        if(value.lock() != req.m_hdl.lock())
                            m_server.send(value, req.m_message.get()->get_payload(), websocketpp::frame::opcode::text);
                    });
            }
            break;
            default:
            std::cerr << "error req type: " << (int)req.m_type << std::endl;
            break;
        }
    }
}

#ifdef TLS
ContextPtr WSServer::on_tls_init(const char* cert_file, const char* dh_file, tls_mode mode, websocketpp::connection_hdl hdl) {
    namespace asio = websocketpp::lib::asio;

    std::cout << "on_tls_init called with hdl: " << hdl.lock().get() << std::endl;
    std::cout << "using TLS mode: " << (mode == tls_mode::MOZILLA_MODERN ? "Mozilla Modern" : "Mozilla Intermediate") << std::endl;

    //ContextPtr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);
    ContextPtr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);

    try {
#if 1
        if (mode == tls_mode::MOZILLA_MODERN) {
            // Modern disables TLSv1
            ctx->set_options(asio::ssl::context::default_workarounds |
                asio::ssl::context::no_sslv2 |
                asio::ssl::context::no_sslv3 |
                asio::ssl::context::no_tlsv1 |
                asio::ssl::context::single_dh_use);
        }
        else {
            ctx->set_options(asio::ssl::context::default_workarounds |
                asio::ssl::context::no_sslv2 |
                asio::ssl::context::no_sslv3 |
                asio::ssl::context::single_dh_use);
        }
#else
        ctx->set_options(
            boost::asio::ssl::context::default_workarounds
            | boost::asio::ssl::context::no_sslv2
            | boost::asio::ssl::context::single_dh_use);
#endif
        //ctx->set_password_callback([]() {return "test"; });
        ctx->set_password_callback(bind(&WSServer::get_password, this));

        ctx->use_certificate_chain_file(cert_file);
        ctx->use_private_key_file(cert_file, asio::ssl::context::pem);

        // Example method of generating this file:
        // `openssl dhparam -out dh.pem 2048`
        // Mozilla Intermediate suggests 1024 as the minimum size to use
        // Mozilla Modern suggests 2048 as the minimum size to use.
        ctx->use_tmp_dh_file(dh_file);

        std::string ciphers;
        if (mode == tls_mode::MOZILLA_MODERN) {
            ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";
        }
        else {
            ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";
        }

        if (SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) != 1) {
            std::cout << "Error setting cipher list" << std::endl;
    }
    }
    catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
#ifndef WIN32
        raise(SIGINT);
#endif
    }
    return ctx;
}
#endif

void WSServer::run(const char* port, const char* cert_file, const char* dh_file) {
    // Listen on port 9002
    if (port)
        m_server.listen("0.0.0.0", port);
    else
        m_server.listen("0.0.0.0", "9002");
    // Start the server accept loop
#ifdef TLS
    m_server.set_tls_init_handler(std::bind(&WSServer::on_tls_init, this, cert_file, dh_file, tls_mode::MOZILLA_INTERMEDIATE, std::placeholders::_1));
#endif

    m_server.start_accept();

    m_process = std::thread(std::bind(&WSServer::on_progress, this));
    m_push = std::thread(std::bind(&WSServer::on_push, this));

    // Start the ASIO io_service run loop
    m_server.run();
}

void WSServer::stop() {
    std::cout << "Shutdown..." << std::endl;
    m_is_running = false;
    m_cond.notify_all();

    if (m_process.joinable())
        m_process.join();
    if (m_push.joinable())
        m_push.join();

    if (m_server.is_listening())
        m_server.stop_listening();
    m_server.stop();
    //server.stopped();
}