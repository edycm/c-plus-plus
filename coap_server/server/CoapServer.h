#ifndef _COAP_SERVER_H_
#define _COAP_SERVER_H_

#include "coap2/coap.h"
#include <thread>
#include "Server.h"
#include "CoapSessionManager.h"
#include <mutex>
#include <atomic>
#include "GwTopics.h"
#include <sstream>
#include <vector>
#include "json/json.h"
#include "CommDefines.h"
#include "MsgDefine.h"

namespace iot {

using namespace net;

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__ ((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif

#define ADD_MEMBER(name)\
	static void name(coap_context_t* ctx UNUSED_PARAM, struct coap_resource_t* resource, coap_session_t* session, \
		coap_pdu_t* request, coap_binary_t* token, coap_string_t* query UNUSED_PARAM, coap_pdu_t* response)


struct CoapServerCfg {
	std::string service;
	std::string host;
	//std::string topic;
	uint32_t timeout;
	uint32_t checkTime;
	uint64_t maxConn;
	uint64_t maxIdle;
	bool preAlloc;
};

class CoapServer : public Server
{
public:
	using SessionMap = std::unordered_map<uint64_t, Session*>;
	using SessionKeyMap = std::unordered_map<std::string, uint64_t>;

	CoapServer():m_ctx(nullptr), m_logLevel(LOG_ERR), m_isRunning(false), m_timeout(5), m_checkTime(5){}
	~CoapServer() {}
	virtual bool Init(void* param);
	coap_context_t* GetContext(const char* node, const char* port);
	virtual void Start();
	virtual void Stop();

	virtual void OnConnect(Session* session) override;
	virtual void OnClose(Session* session) override;
	virtual void OnError(Session* session, const int errCode, const char* errMsg) override;
	virtual int OnRead(Session* session, const char* buf, const size_t len) override;
	virtual int OnWrite(Session* session, const char* buf, const size_t len) override;

	CoapSession* GetSession(uint64_t sessionID);
	CoapSession* GetSession(const std::string& key);
	void ReleaseSession(const std::string& key);
	bool IsSessionExist(const std::string& key);
	bool CheckValidation(const std::string& key);
	void CheckSession();

	bool SendLoginReq(const std::string& key, const char* payload, const size_t len);
	bool DoPushRequest(int type, const void* payload, int len);
	bool PushRequest(const std::string& key, int msgType, const void* payload, int len);

	//
	void SendResp(coap_pdu_t* response, const int& code, const std::string& msg);

	bool GetKeyFromTopic(Json::Value& root, std::string* key) 
	{
		if (key == nullptr) return true;
		if (!root.isObject() ||
			!root.isMember("topic") ||
			!root["topic"].isString()) 
		{
			return false;
		}

		std::string sn;
		ParseTopic(root["topic"].asString(), key, &sn);
		key->append(".");
		key->append(sn);
		return true;
	}

	std::vector<std::string> Split(const std::string& s, const std::string& sep) 
	{
		std::vector<std::string> tokens;
		std::string::size_type lastPos = s.find_first_not_of(sep, 0);
		std::string::size_type pos = s.find_first_of(sep, lastPos);
		while (std::string::npos != pos || std::string::npos != lastPos) 
		{
			tokens.push_back(s.substr(lastPos, pos - lastPos));
			lastPos = s.find_first_not_of(sep, pos);
			pos = s.find_first_of(sep, lastPos);
		}
		return std::move(tokens);
	}

	bool ParseTopic(const std::string& topic, std::string* pk, std::string* sn) 
	{
		auto tokens = Split(topic, TOPIC_SEPERATOR);
		if (tokens.size() < TOPIC_FIELDS_NUM) 
		{
			coap_log(LOG_ERR, "topic fields num %d != %d, topic(%s)",
				tokens.size(), TOPIC_FIELDS_NUM, topic.c_str());
			return false;
		}
		if (pk) pk->assign(tokens[1]);
		if (sn) sn->assign(tokens[2]);
		return true;
	}

	//
	void InitResources(coap_context_t* ctx);
	ADD_MEMBER(HndGetIndex);
	ADD_MEMBER(HndLogIn);
	ADD_MEMBER(HndEvent);
	ADD_MEMBER(HndLogOut);

private:
	coap_context_t* m_ctx;
	coap_log_t m_logLevel;
	std::atomic<bool> m_isRunning;
	std::thread m_worker;
	std::thread m_checkThread;

	uint32_t m_timeout;
	uint32_t m_checkTime;

	CoapSessionManager	m_sessionMgr;
	SessionMap          m_sessions;
	SessionKeyMap       m_sessionKeys;

	std::mutex m_sessionMtx;
};

}
#endif