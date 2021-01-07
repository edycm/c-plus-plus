#include "CoapServer.h"
//#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


namespace iot {
#define INDEX "This is a coap server\n"

inline std::string MakeDeviceTopicFromKey(const std::string& key,
    const std::string action) {
    std::ostringstream oss;
    oss << "device." << key << ".iot." << action;
    return oss.str();
}

static Json::Reader reader(Json::Features::strictMode());

bool CoapServer::Init(void* param)
{
    uint16_t cache_ignore_options[] = { COAP_OPTION_BLOCK1,
                                    COAP_OPTION_BLOCK2,
        /* See https://tools.ietf.org/html/rfc7959#section-2.10 */
        COAP_OPTION_MAXAGE,
        /* See https://tools.ietf.org/html/rfc7959#section-2.10 */
        COAP_OPTION_IF_NONE_MATCH };

    CoapServerCfg* cfg = static_cast<CoapServerCfg*>(param);
    m_checkTime = cfg->checkTime;
    m_timeout = cfg->timeout;
    m_sessionMgr.SetMaxConn(cfg->maxConn);
    m_sessionMgr.SetMaxIdle(cfg->maxIdle);
    if (!m_sessionMgr.Init(cfg->preAlloc)) {
        coap_log(LOG_ERR, "mq session manager init fail");
        return false;
    }

    char* group = NULL;

    m_isRunning = true;
    m_logLevel = LOG_DEBUG;

    coap_startup();
    coap_dtls_set_log_level(m_logLevel);
    coap_set_log_level(m_logLevel);

    m_ctx = GetContext(cfg->host.c_str(), cfg->service.c_str());
    if (!m_ctx)
        return -1;

    //set url
    InitResources(m_ctx);

    m_ctx->app = static_cast<void *>(this);

    coap_cache_ignore_options(m_ctx, cache_ignore_options, sizeof(cache_ignore_options) / sizeof(cache_ignore_options[0]));

    if (group)
    {
        coap_join_mcast_group(m_ctx, group);
    }

    return true;
}

void CoapServer::Start()
{
    m_worker = std::thread([&]() {
        int coap_fd, nfds = 0;
        fd_set m_readfds;
        unsigned wait_ms;

        coap_fd = coap_context_get_coap_fd(m_ctx);
        if (coap_fd != -1)
        {
            /* if coap_fd is -1, then epoll is not supported within libcoap */
            FD_ZERO(&m_readfds);
            FD_SET(coap_fd, &m_readfds);
            nfds = coap_fd + 1;
        }

        wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;

        while (m_isRunning)
        {
            int result;
            if (coap_fd != -1)
            {
                /*
                    * Using epoll.  It is more usual to call coap_io_process() with wait_ms
                    * (as in the non-epoll branch), but doing it this way gives the
                    * flexibility of potentially working with other file descriptors that
                    * are not a part of libcoap.
                    */
                fd_set readfds = m_readfds;
                struct timeval tv;
                coap_tick_t begin, end;

                coap_ticks(&begin);

                tv.tv_sec = wait_ms / 1000;
                tv.tv_usec = (wait_ms % 1000) * 1000;
                /* Wait until any i/o takes place or timeout */
                result = select(nfds, &readfds, NULL, NULL, &tv);
                if (result == -1)
                {
                    if (errno != EAGAIN)
                    {
                        coap_log(LOG_DEBUG, "select: %s (%d)\n", coap_socket_strerror(), errno);
                        break;
                    }
                }
                if (result > 0)
                {
                    if (FD_ISSET(coap_fd, &readfds))
                    {
                        result = coap_io_process(m_ctx, COAP_IO_NO_WAIT);
                    }
                }
                if (result >= 0)
                {
                    coap_ticks(&end);
                    /* Track the overall time spent in select() and coap_io_process() */
                    result = (int)(end - begin);
                }
            }
            else
            {
                /*
                    * epoll is not supported within libcoap
                    *
                    * result is time spent in coap_io_process()
                    */
                result = coap_io_process(m_ctx, wait_ms);
            }
            if (result < 0)
            {
                break;
            }
            else if (result && (unsigned)result < wait_ms)
            {
                /* decrement if there is a result wait time returned */
                wait_ms -= result;
            }
            else
            {
                /*
                    * result == 0, or result >= wait_ms
                    * (wait_ms could have decremented to a small value, below
                    * the granularity of the timer in coap_io_process() and hence
                    * result == 0)
                    */
                wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
            }
        }
        });

    m_checkThread = std::thread(std::bind(&CoapServer::CheckSession, this));

    return;
}

void CoapServer::Stop()
{
    m_isRunning = false;
    if (m_worker.joinable())
        m_worker.join();
    if (m_checkThread.joinable())
        m_checkThread.join();
    coap_free_context(m_ctx);
    coap_cleanup();
    return;
}


void CoapServer::OnConnect(Session* session)
{
    if (!m_handler.connFunc)
        return;
    m_handler.connFunc(session);
    return;
}

void CoapServer::OnClose(Session* session)
{
    if (!m_handler.closeFunc)
        return;
    m_handler.closeFunc(session);
    return;
}

void CoapServer::OnError(Session* session, const int errCode, const char* errMsg)
{
    if (!m_handler.errorFunc)
        return;
    m_handler.errorFunc(session, errCode, errMsg);
    return;
}

int CoapServer::OnRead(Session* session, const char* buf, const size_t len)
{
    // process request
    if (!m_handler.readFunc)
        return 0;
    m_handler.readFunc(session, buf, len);
    return len;
}

int CoapServer::OnWrite(Session* session, const char* buf, const size_t len)
{
    //send data to device
    std::unique_lock<std::mutex> locker(m_sessionMtx);
    
    if (!m_isRunning) {
        coap_log(LOG_ERR, "mq producer not connect in MQServer");
        return -1;
    }

    Session* pSession = GetSession(session->GetSessionID());
    int eventType = *static_cast<int*>(session->GetParam1());
    if (pSession == nullptr) {
        coap_log(LOG_ERR, "can't find session(%lu)", session->GetSessionID());
        return -2;
    }
    /*
        int code = DEV_AUTH_OK;
        MQSession* mqSession = dynamic_cast<MQSession*>(pSession);
        if (eventType != MSG_IOT_AUTH_DEV_RSP) {
            if (pSession->GetCreateTime() != session->GetCreateTime()) {
                LOGW("session(%lu) not match with time", session->GetSessionID());
                return -3;
            }
        }
        else {
            Json::Value root;
            Json::Reader reader;
            if (!reader.parse(buf, buf + len, root)) {
                LOGE("parse dev auth rsp json fail");
                return -4;
            }
            if (!root.isMember("code") || !root["code"].isInt()
                || !root.isMember("data") || !root["data"].isObject()
                || !root["data"].isMember("productKey")
                || !root["data"]["productKey"].isString()
                || !root["data"].isMember("deviceSN")
                || !root["data"]["deviceSN"].isString()) {
                LOGE("dev auth rsp parameter check fail");
                return -4;
            }

            code = root["code"].asInt();
            if (code == DEV_AUTH_OK) {
                // bind key and session when device auth successful
                std::string key = root["data"]["productKey"].asString() + ".";
                key += root["data"]["deviceSN"].asString();
                m_sessionKeys.insert({ key, session->GetSessionID() });
            }
        }

        if (code == DEV_AUTH_OK) {
            assert(m_sessionKeys.count(mqSession->GetKey()) == 1);
        }

        std::string topic = MakeRspTopic(eventType, mqSession->GetKey());
        if (topic.empty()) {
            LOGE("invalid topic, event(%d), key(%s)",
                eventType, mqSession->GetKey().c_str());
            return -4;
        }

        if (!m_producer.Produce(topic.c_str(), buf, len)) {
            LOGE("[%lu] mq produce message fail, code(%d)",
                mqSession->GetSessionID(), m_producer.GetErrorCode());
            if (m_producer.GetErrorCode() == MOSQ_ERR_ERRNO) {
                LOGE("system err(%d): %s", errno, strerror(errno));
            }
        }

        if (eventType == MSG_IOT_GW_DEV_LOGOUT) {
            m_sessionKeys.erase(mqSession->GetKey());
        }
        */
    return len;
}

CoapSession* CoapServer::GetSession(uint64_t sessionID)
{
    return dynamic_cast<CoapSession*>(m_sessionMgr.Find(sessionID));
}

CoapSession* CoapServer::GetSession(const std::string& key)
{
    auto it = m_sessionKeys.find(key);
    if (it == m_sessionKeys.end()) {
        return nullptr;
    }

    CoapSession* session = dynamic_cast<CoapSession*>(m_sessionMgr.Find(it->second));
    assert(session != nullptr && "session must in manager");
    assert(session->GetKey() == key);
    return session;
    // auto iter = m_sessions.find(it->second);
    // return iter == m_sessions.end() ? nullptr : iter->second;
}

void CoapServer::ReleaseSession(const std::string& key)
{
    std::unique_lock<std::mutex> locker(m_sessionMtx);
    m_sessionKeys.erase(key);
    return;
}

bool CoapServer::IsSessionExist(const std::string& key)
{
    std::unique_lock<std::mutex> locker(m_sessionMtx);
    return m_sessionKeys.count(key);
}

bool CoapServer::CheckValidation(const std::string& key)
{
    std::unique_lock<std::mutex> locker(m_sessionMtx);
    auto* session = GetSession(key);
    if (session == nullptr) 
    {
        return false;
    }
    return true;
}

void CoapServer::CheckSession()
{
    time_t curTime = time(nullptr);
    while (true) 
    {
        if (!m_isRunning) 
        {
            break;
        }

        {
            std::unique_lock<std::mutex> locker(m_sessionMtx);
            auto it = m_sessionKeys.begin();
            while (it != m_sessionKeys.end())
            {
                CoapSession* session = GetSession(it->second);
                assert(session != nullptr);
                assert(it->first == session->GetKey());
                if (session->GetLastActiveTime() + m_timeout >= curTime)
                {
                    OnClose(session);
                    if (!m_sessionMgr.Release(session))
                    {
                        coap_log(LOG_ERR, "release session(%lu) error, code: %d",
                            it->second, m_sessionMgr.GetErrorCode());
                        assert("release error" == nullptr);
                    }

                    coap_log(LOG_INFO, "release mq session(%lu), key(%s)",
                        it->second, it->first.c_str());
                    it = m_sessionKeys.erase(it);
                }
                else
                {
                    ++it;
                }

            }
        }
        //sleep(m_checkTime);
        std::this_thread::sleep_for(std::chrono::seconds(m_checkTime));
    }
}

coap_context_t* CoapServer::GetContext(const char* node, const char* port)
{
    coap_context_t* ctx = NULL;
    int s;
    struct addrinfo hints;
    struct addrinfo* result, * rp;

    ctx = coap_new_context(NULL);
    if (!ctx)
    {
        return NULL;
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Coap uses UDP */
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

    s = getaddrinfo(node, port, &hints, &result);
    if (s != 0)
    {
        coap_log(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(s));
        coap_free_context(ctx);
        return NULL;
    }

    /* iterate through results until success */
    for (rp = result; rp != NULL; rp = rp->ai_next)
    {
        coap_address_t addr, addrs;
        coap_endpoint_t* ep_udp = NULL, * ep_dtls = NULL;

        if (rp->ai_addrlen <= sizeof(addr.addr))
        {
            coap_address_init(&addr);
            addr.size = (socklen_t)rp->ai_addrlen;
            memcpy(&addr.addr, rp->ai_addr, rp->ai_addrlen);
            addrs = addr;
            if (addr.addr.sa.sa_family == AF_INET)
            {
                uint16_t temp = ntohs(addr.addr.sin.sin_port) + 1;
                addrs.addr.sin.sin_port = htons(temp);
            }
            else if (addr.addr.sa.sa_family == AF_INET6)
            {
                uint16_t temp = ntohs(addr.addr.sin6.sin6_port) + 1;
                addrs.addr.sin6.sin6_port = htons(temp);
            }
            else
            {
                freeaddrinfo(result);
                return ctx;
            }
            //
            ep_udp = coap_new_endpoint(ctx, &addr, COAP_PROTO_UDP);
            if (!ep_udp)
            {
                coap_log(LOG_CRIT, "cannot create UDP endpoint\n");
                continue;
            }
            else
            {
                freeaddrinfo(result);
                return ctx;
            }
        }
    }

    coap_log(LOG_ERR, "no context available for interface '%s'\n", node);
    coap_free_context(ctx);
    ctx = NULL;

    freeaddrinfo(result);
    return ctx;
}

bool CoapServer::SendLoginReq(const std::string& key, const char* payload, const size_t len) 
{
    std::unique_lock<std::mutex> locker(m_sessionMtx);
    CoapSession* session = dynamic_cast<CoapSession*>(m_sessionMgr.Get());
    if (session == nullptr) 
    {
        auto code = m_sessionMgr.GetErrorCode();
        if (code == CoapSessionManager::Error::TOO_MANY_CONNS) 
        {
            /*
            coap_log(LOG_WARNING, "mq manager error, too many connections");
            Json::Value root;
            root["code"] = -2;
            root["msg"] = "too many connections";
            //std::string topic = MakeRspTopic(MSG_IOT_AUTH_DEV_RSP, key);
            std::string rsp = Json::FastWriter().write(root);
            */
            return true;
        }
    }

    assert(session != nullptr && "new session can't be null");
    session->SetKey(key);
    OnConnect(session);
    int event = MSG_IOT_AUTH_DEV;
    session->SetParam1(&event);
    int ret = OnRead(session, payload, len);
    if (0 >= ret)
    {
        coap_log(LOG_ERR, "coap server send login request, ret=%d", ret);
        return false;
    }
    return true;
}

bool CoapServer::PushRequest(const std::string& key, int msgType, const void* payload, int len) 
{
    std::unique_lock<std::mutex> locker(m_sessionMtx);
    CoapSession* session = GetSession(key);
    if (session == nullptr) 
    {
        //LOGE("can't find session with key(%s)", key.c_str());
        coap_log(LOG_ERR, "can't find session with key(%s)", key.c_str());
        return false;
    }

    session->SetLastActiveTime(time(nullptr));
    session->SetParam1(&msgType);
    int ret = OnRead(session, static_cast<const char*>(payload), len);
    if (0 > ret) 
    {
        //LOGE("device(%s) write error, ret=%d", key.c_str(), ret);
        coap_log(LOG_ERR, "device(%s) write error, ret=%d", key.c_str(), ret);
        return false;
    }
    return true;
}

bool CoapServer::DoPushRequest(int type, const void* payload, int len) 
{
    Json::Value root;
    Json::Reader reader;
    const char* buf = static_cast<const char*>(payload);
    if (!reader.parse(buf, buf + len, root)) 
    {
        //LOGE("parse json error, payload(%s)", payload);
        coap_log(LOG_ERR, "parse json error, payload(%s)", payload);
        return false;
    }

    if (!root.isObject()
        || !root.isMember("topic")
        || !root["topic"].isString()) 
    {
        //LOGE("check topic error, payload(%s)", payload);
        coap_log(LOG_ERR, "check topic error, payload(%s)", payload);
        return false;
    }

    std::string pk, sn;
    if (!ParseTopic(root["topic"].asString(), &pk, &sn)) 
    {
        //LOGE("parse topic fail");
        coap_log(LOG_ERR, "parse topic fail", payload);
        // TODO: response error to device?
        return false;
    }

    std::string key = pk + "." + sn;
    if (!CheckValidation(key)) 
    {
        //LOGE("session with key(%s) not exist", key.c_str());
        coap_log(LOG_ERR, "session with key(%s) not exist", key.c_str());
        return false;
    }

    if (!PushRequest(key, type, payload, len))
    {
        return false;
    }

    return true;
}

void CoapServer::InitResources(coap_context_t* ctx)
{
    coap_resource_t* r;

    int resource_flags = COAP_RESOURCE_FLAGS_NOTIFY_CON;

    //default
    r = coap_resource_init(NULL, 0);
    coap_register_handler(r, COAP_REQUEST_GET, CoapServer::HndGetIndex);
    coap_add_attr(r, coap_make_str_const("ct"), coap_make_str_const("0"), 0);
    coap_add_attr(r, coap_make_str_const("title"), coap_make_str_const("\"General Info\""), 0);
    coap_add_resource(ctx, r);

    //LogIn
    r = coap_resource_init(coap_make_str_const(GW_LOGIN_UP_TOPIC), resource_flags);
    coap_register_handler(r, COAP_REQUEST_POST, HndLogIn);
    coap_resource_set_get_observable(r, 1);
    coap_add_attr(r, coap_make_str_const("ct"), coap_make_str_const("0"), 0);
    coap_add_attr(r, coap_make_str_const("title"), coap_make_str_const("\"LogIn\""), 0);
    //coap_add_attr(r, coap_make_str_const("rt"), coap_make_str_const("\"ticks\""), 0);
    //coap_add_attr(r, coap_make_str_const("if"), coap_make_str_const("\"clock\""), 0);
    coap_add_resource(ctx, r);

    //Event
    r = coap_resource_init(coap_make_str_const(GW_EVENT_UP_TOPIC), resource_flags);
    coap_register_handler(r, COAP_REQUEST_POST, HndEvent);
    coap_resource_set_get_observable(r, 1);
    coap_add_attr(r, coap_make_str_const("ct"), coap_make_str_const("0"), 0);
    coap_add_attr(r, coap_make_str_const("title"), coap_make_str_const("\"Event\""), 0);
    coap_add_resource(ctx, r);

    //LogOut
    r = coap_resource_init(coap_make_str_const(GW_LOGOUT_UP_TOPIC), resource_flags);
    coap_register_handler(r, COAP_REQUEST_POST, HndLogOut);
    coap_resource_set_get_observable(r, 1);
    coap_add_attr(r, coap_make_str_const("ct"), coap_make_str_const("0"), 0);
    coap_add_attr(r, coap_make_str_const("title"), coap_make_str_const("\"Register\""), 0);
    coap_add_resource(ctx, r);

    return;
}

void CoapServer::HndGetIndex(coap_context_t* ctx UNUSED_PARAM, struct coap_resource_t* resource, coap_session_t* session, \
    coap_pdu_t* request, coap_binary_t* token, coap_string_t* query UNUSED_PARAM, coap_pdu_t* response)
{
    coap_add_data_blocked_response(resource, session, request, response, token, COAP_MEDIATYPE_TEXT_PLAIN, 0x2ffff, \
        strlen(INDEX), (const uint8_t*)INDEX);
    //ctx->app;
    return;
}

void CoapServer::SendResp(coap_pdu_t* response, const int& code, const std::string& msg)
{
    Json::Value resp_value(Json::objectValue);
    resp_value["code"] = code;
    resp_value["msg"] = msg;
    std::string rsp = Json::FastWriter().write(resp_value);

    unsigned char buf[3];
    response->code = COAP_RESPONSE_CODE(205);
    coap_add_option(response, COAP_OPTION_CONTENT_FORMAT, coap_encode_var_safe(buf, sizeof(buf), COAP_MEDIATYPE_APPLICATION_JSON), buf);
    coap_add_data(response, rsp.size(), reinterpret_cast<const uint8_t*>(rsp.c_str()));
    return;
}

void CoapServer::HndLogIn(coap_context_t* ctx UNUSED_PARAM, struct coap_resource_t* resource, coap_session_t* session, \
    coap_pdu_t* request, coap_binary_t* token, coap_string_t* query UNUSED_PARAM, coap_pdu_t* response)
{
    size_t size;
    unsigned char* data;
    (void)coap_get_data(request, &size, &data);

    //assert(ctx->app);
    CoapServer* server = static_cast<CoapServer*>(ctx->app);

    Json::Value root;
    char* req_data = reinterpret_cast<char*>(data);
    if (!reader.parse(req_data, req_data + size, root)) 
    {
        coap_log(LOG_ERR, "parse json error, payload(%s)", req_data);
        return;
    }

    std::string key;
    if (!server->GetKeyFromTopic(root, &key)) 
    {
        coap_log(LOG_ERR, "get key fail, data(%s)", root.toStyledString().c_str());
        return;
    }
    // TODO: make sure whether send error or not when device relogin
    //coap_log(LOG_INFO, "IsSessionExist");
    if (server->IsSessionExist(key)) 
    {
        server->SendResp(response, -1, "device has already logined");
        return;
    }
    
    if (!server->SendLoginReq(key, req_data, size)) 
    {
        coap_log(LOG_ERR, "send login request fail");
        server->SendResp(response, -1, "send login request fail");
        return;
    }
   
    server->SendResp(response, 0, "log in success");

    coap_log(LOG_INFO, "recv data : %s", req_data);

    return;
}

void CoapServer::HndEvent(coap_context_t* ctx UNUSED_PARAM, struct coap_resource_t* resource, coap_session_t* session, \
    coap_pdu_t* request, coap_binary_t* token, coap_string_t* query UNUSED_PARAM, coap_pdu_t* response)
{
    size_t size;
    unsigned char* data;
    (void)coap_get_data(request, &size, &data);

    CoapServer* server = static_cast<CoapServer*>(ctx->app);

    const char* req_data = reinterpret_cast<const char*>(data);

    int event = MSG_IOT_GW_REPORT_EVENT;
    if (!server->DoPushRequest(event, req_data, size))
    {
        server->SendResp(response, -1, "push request failed");
        return;
    }

    coap_log(LOG_INFO, "recv data : %s", req_data);
    server->SendResp(response, 0, "handle event success");

    return;
}

void CoapServer::HndLogOut(coap_context_t* ctx UNUSED_PARAM, struct coap_resource_t* resource, coap_session_t* session, \
    coap_pdu_t* request, coap_binary_t* token, coap_string_t* query UNUSED_PARAM, coap_pdu_t* response)
{
    size_t size;
    unsigned char* data;
    (void)coap_get_data(request, &size, &data);

    CoapServer* server = static_cast<CoapServer*>(ctx->app);

    Json::Value root;
    Json::Reader reader(Json::Features::strictMode());
    const char* req_data = reinterpret_cast<const char*>(data);
    if (!reader.parse(req_data, req_data + size, root)) 
    {
        coap_log(LOG_ERR, "parse json error, payload(%s)", req_data);
        server->SendResp(response, -1, "parse json error");
        return;
    }

    // for new login protocol
    std::string key;
    if (!server->GetKeyFromTopic(root, &key)) 
    {
        coap_log(LOG_ERR, "get key fail, data(%s)", root.toStyledString().c_str());
        server->SendResp(response, -1, "get key fail");
        return;
    }

    if (!server->IsSessionExist(key)) 
    {
        server->SendResp(response, -2, "device not exist");
        return;
    }

    Json::Value jsPayload;
    root.clear();
    root["topic"] = MakeDeviceTopicFromKey(key, "event.state");

    auto time_duration = std::chrono::system_clock::now().time_since_epoch();
    root["timestamp"] = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(time_duration).count());
    jsPayload["state"] = "offline";
    root["payload"] = jsPayload;

    std::string req = Json::FastWriter().write(root);
    //LOGI("send logout request: %s", req.c_str());
    coap_log(LOG_INFO, "send logout request: %s", req.c_str());
    // logout
    if (!server->PushRequest(key, MSG_IOT_GW_DEV_LOGOUT, req.c_str(), req.size())) {
        //LOGE("send logout fail, key(%s)", key.c_str());
        coap_log(LOG_ERR, "send logout fail, key(%s)", key.c_str());
        server->SendResp(response, -1, "send logout fail");
        return;
    }

    coap_log(LOG_INFO, "recv data : %s", req_data);
    server->SendResp(response, 0, "log out success");

    return;
}

}