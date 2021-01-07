#ifndef NET_SERVER_H_
#define NET_SERVER_H_
#include <string>
#include "Session.h"
#include <functional>

namespace net {

enum class ServerStatus : char {
    UNINIT,
    INITED,
    CONNECTED,
    RUNNING,
    STOP,
    INVALID,
};

class Server {
public:
    typedef std::function<void(Session*)> OnConnectHandler;
    typedef std::function<void(Session*)> OnCloseHandler;
    typedef std::function<void(Session*, 
                               const int, 
                               const char*)> OnErrorHandler;

    typedef std::function<int(Session*, 
                              const char*, 
                              const size_t)> OnReadHandler;

    typedef std::function<int(Session*, 
                              const char*, 
                              const size_t)> OnWriteHandler;

    struct MsgHandler {
        OnConnectHandler connFunc;
        OnCloseHandler closeFunc;
        OnErrorHandler errorFunc;
        OnReadHandler readFunc;

        MsgHandler() {
            connFunc = nullptr;
            closeFunc = nullptr;
            errorFunc = nullptr;
            readFunc = nullptr;
        }
    };

    Server() = default;

    virtual ~Server() = default;

    void SetName(const char *name) { m_name = name; }

    const char *GetName() const { return m_name.c_str(); }

    void SetOnConnectHandler(OnConnectHandler connFunc) { 
        m_handler.connFunc = connFunc;
    }

    void SetOnCloseHandler(OnCloseHandler closeFunc) { 
        m_handler.closeFunc = closeFunc;
    }

    void SetOnErrorHandler(OnErrorHandler errorFunc) { 
        m_handler.errorFunc = errorFunc;
    }

    void SetOnReadHandler(OnReadHandler readFunc) { 
        m_handler.readFunc = readFunc;
    }

    virtual bool Init(void *param) = 0;

    virtual void Start() = 0;

    virtual void Stop() = 0;

    virtual void OnConnect(Session *session) = 0;

    virtual void OnClose(Session *session) = 0;

    virtual void OnError(Session *session, 
                         const int errCode, const char *errMsg) = 0;

    virtual int OnRead(Session *session, 
                       const char *buf, const size_t len) = 0;

    virtual int OnWrite(Session *session, 
                        const char *buf, const size_t len) = 0;

protected:
    std::string     m_name;
    MsgHandler      m_handler;
};

}  // namespace net

#endif  // NET_Server_H_
