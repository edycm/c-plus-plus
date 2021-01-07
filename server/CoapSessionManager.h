#ifndef _COAP_SESSION_MANAGER_H_
#define _COAP_SESSION_MANAGER_H_

#include "Session.h"
#include <ctime>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <assert.h>

using net::Session;

class CoapSession :public Session
{
public:
	CoapSession():m_lastActive(time(nullptr)){}
	~CoapSession() = default;

	virtual void Reset() override {
		Session::Reset();
		m_key.clear();
		m_lastActive = 0;
	}

	void SetKey(const std::string& key) { m_key = key; }

	const std::string& GetKey() const { return m_key; }

	void SetLastActiveTime(time_t active) { m_lastActive = active; }

	time_t GetLastActiveTime() const { return m_lastActive; }
private:
	time_t          m_lastActive;
	std::string     m_key;
};

class CoapSessionManager {
public:
    using SessionMap = std::unordered_map<uint64_t, Session*>;
    using IdleSet = std::unordered_set<uint64_t>;

    enum class Error : short {
        OK,
        INVALID_SESSION,
        UNKNOWN_SESSION,
        TOO_MANY_CONNS,
        IDLE_SESSION_EXIST,
    };

    CoapSessionManager(const size_t maxConnNum = 0, const size_t maxIdleNum = 0)
        :m_inited(false)
        , m_maxConn(maxConnNum)
        , m_maxIdle(maxIdleNum)
        , m_sessionCounter(0)
        , m_errCode(Error::OK) { }

    ~CoapSessionManager() {
        for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
            delete it->second;
            it = m_sessions.erase(it);
        }
    }

    bool Init(bool preAllocate = true) {
        if (preAllocate) {
            for (size_t i = 0; i < m_maxConn / 2; ++i) {
                Session* session = dynamic_cast<Session*>(new CoapSession());
                assert(session != nullptr && "session can't be null");
                session->SetSessionID(++m_sessionCounter);
                m_idles.insert(session->GetSessionID());
                m_sessions[session->GetSessionID()] = session;
            }
        }

        return m_inited = true;
    }

    void SetMaxConn(int maxConnNum) {
        if (!m_inited) { m_maxConn = maxConnNum; }
    }

    size_t GetMaxConn() const { return m_maxConn; }

    void SetMaxIdle(int maxIdleNum) {
        if (!m_inited) { m_maxIdle = maxIdleNum; }
    }

    int GetMaxIdle() const { return m_maxIdle; }

    Session* Find(const uint64_t sessionID) {
        auto it = m_sessions.find(sessionID);
        return it != m_sessions.end() ? it->second : nullptr;
    }

    Session* Get() {
        Session* session = nullptr;
        m_errCode = Error::OK;
        if (m_idles.empty()) {
            if (m_sessions.size() < m_maxConn) {
                session = dynamic_cast<Session*>(new CoapSession());
                assert(session);
                session->SetSessionID(++m_sessionCounter);
                m_sessions[session->GetSessionID()] = session;
            }
            else {
                m_errCode = Error::TOO_MANY_CONNS;
                return nullptr;
            }
        }
        else {
            uint64_t sessionID = *m_idles.begin();
            assert(sessionID != 0);
            m_idles.erase(sessionID);
            session = Find(sessionID);
            assert(session != nullptr);
        }

        if (session == nullptr) m_errCode = Error::INVALID_SESSION;
        return session;
    }

    bool Release(Session* session) {
        if (session == nullptr) {
            m_errCode = Error::INVALID_SESSION;
            return false;
        }

        uint64_t ssid = session->GetSessionID();
        if (0 == m_sessions.count(ssid)) {
            m_errCode = Error::UNKNOWN_SESSION;
            return false;
        }
        if (0 != m_idles.count(ssid)) {
            m_errCode = Error::IDLE_SESSION_EXIST;
            return false;
        }

        auto resPair = m_idles.insert(ssid);
        assert(resPair.second == true);
        session->Reset();
        session->SetSessionID(ssid);
        return true;
    }

    size_t GetActiveCount() { return m_sessions.size() - m_idles.size(); }

    size_t GetIdelCount() const { return m_idles.size(); }

    Error GetErrorCode() const { return m_errCode; }
private:
    CoapSessionManager(const CoapSessionManager&) = delete;
    CoapSessionManager& operator=(const CoapSessionManager&) = delete;

private:
    bool                m_inited;
    size_t              m_maxConn;
    size_t              m_maxIdle;
    uint64_t            m_sessionCounter;
    Error               m_errCode;
    SessionMap          m_sessions;
    IdleSet             m_idles;
};


#endif