/**
 * Copyright (c) 2020. All rights reserved.
 * Connect session for different server, like tcp server, websocket server
 * @author  tesion
 * @date    2020-08-19
 */
#ifndef NET_SESSION_H_
#define NET_SESSION_H_
#include <cstdint>
#include <ctime>

namespace net {

class Server;

class Session {
public:
    Session() {
        m_sessionID = 0;
        m_createTime = 0;
        m_param1 = nullptr;
        m_param2 = nullptr;
        m_server = nullptr;
    }

    Session(uint64_t sessionID, time_t createTime) 
        : m_sessionID(sessionID), m_createTime(createTime) {
        m_param1 = nullptr;
        m_param2 = nullptr;
        m_server = nullptr;
    }

    virtual ~Session() = default;

    void SetSessionID(uint64_t id) { m_sessionID = id; }

    uint64_t GetSessionID() const { return m_sessionID; }

    void SetCreateTime(const time_t tm) { m_createTime = tm; }

    time_t GetCreateTime() const { return m_createTime; }

    void SetServer(Server *server) { m_server = server; }

    Server *GetServer() { return m_server; }

    virtual void Reset() {
        m_sessionID = 0;
        m_createTime = 0;
        m_param1 = nullptr;
        m_param2 = nullptr;
        m_server = nullptr;
    }

    // param1 and  param2 is used for upper application
    void SetParam1(void *param) { m_param1 = param; }

    void *GetParam1() { return m_param1; }

    void SetParam2(void *param) { m_param2 = param; }

    void *GetParam2() { return m_param2; }

private:
    uint64_t    m_sessionID;
    time_t      m_createTime;
    void*       m_param1;
    void*       m_param2;
    Server*     m_server;
};

}  // namespace net

#endif  // NET_SESSION_H_
