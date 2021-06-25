#include <iostream>
#include <string>
#include <stdio.h>
#include <thread>
#include <atomic>

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include "md5.h"

#define FLAG_UDP_TRANS 0

const char* USER_AGENT = "rtsp player 1.0";
const char* RTSP_VERSION = "RTSP/1.0";
const char* CER = "WWW-Authenticate";
const char* TCP_TRANS = "RTP/AVP/TCP;unicast;interleaved=0-1";
const char* UDP_TRANS = "RTP/AVP;unicast;client_port=8001-8002";
const int MAX_SIZE = 1024 * 4;
static int CSeq = 1;

//rtp buff
uint32_t rtp_read = 0, rtp_write = 0, rtp_size = 16 * 1024 * 1024;
uint8_t* rtp_content = NULL;

std::atomic<bool> isRunning = false;

typedef struct RtspCntHeader {
	uint8_t  magic;        // 0x24
	uint8_t  channel;
	uint16_t length;
	uint8_t  payload[0];   // >> RtpHeader
} RtspCntHeader_st;

typedef struct RtpCntHeader {
#if 0
	uint8_t  version : 2;
	uint8_t  padding : 1;
	uint8_t  externsion : 1;
	uint8_t  CSrcId : 4;
	uint8_t  marker : 1;
	uint8_t  pt : 7;          // payload type
#else
	uint8_t  exts;
	uint8_t  type;
#endif
	uint16_t seqNo;         // Sequence number
	uint32_t ts;            // Timestamp
	uint32_t SyncSrcId;     // Synchronization source (SSRC) identifier
							// Contributing source (SSRC_n) identifier
	uint8_t payload[0];     // Frame data
} RtpCntHeader_st;

#define _rtsp_remaining ((size_t)(rtp_read>rtp_write?(rtp_size - (rtp_read - rtp_write)):(rtp_write - rtp_read)))

int rtsp_read(const uint8_t* buff, int rcvs) {
	//rtsp_dump(buff, rcvs);
	// 移动缓冲区剩余内容到页首
	if (rtp_read != rtp_write) {
		if (rtp_read != 0) {
			memmove(rtp_content, rtp_content + rtp_read, rtp_write - rtp_read);
			rtp_write -= rtp_read;
			rtp_read = 0;
		}
	}
	else {
		rtp_read = rtp_write = 0;
	}
	// 复制到缓冲区
	memcpy(rtp_content + rtp_write, buff, rcvs);
	rtp_write += rcvs;

	return rcvs;
}

bool rtsp_packet(uint8_t* buf, int& size)
{
	RtspCntHeader_st* rtspH = (RtspCntHeader_st*)(rtp_content + rtp_read);

	if (0x24 != rtspH->magic) {
		printf("Magic number error. %02x\n", rtspH->magic);
		// Magic number ERROR, discarding all the data
		rtp_read = rtp_write = 0;
		return false;
	}
	size_t rtsplen = ntohs(rtspH->length);
	if (rtsplen > _rtsp_remaining - sizeof(RtspCntHeader_st)) {
		printf("No enough data. %lu|%lu\n", rtsplen, _rtsp_remaining);
		// No enough data, try next loop
		return false;
	}

	RtpCntHeader_st* rtpH = (RtpCntHeader_st*)rtspH->payload;
	if (0x60 != (rtpH->type & 0x7f)) {
		printf("No video stream %02x.\n", rtpH->type);
		// 不是RTP视频数据，不处理
		rtp_read += (sizeof(RtspCntHeader_st) + rtsplen);
		return false;
	}

	// 将数据复制到packet buffer中，数据足够时使用mpp解码
	uint8_t h1 = rtpH->payload[0];
	uint8_t h2 = rtpH->payload[1];
	uint8_t nal = h1 & 0x1f;
	uint8_t flag = h2 & 0xe0;
	size_t paylen = rtsplen - sizeof(RtpCntHeader_st);
	size_t index = 0;

	if (0x1c == nal) {
		if (0x80 == flag) {
			buf[index++] = 0;
			buf[index++] = 0;
			buf[index++] = 0;
			buf[index++] = 1;
			buf[index++] = ((h1 & 0xe0) | (h2 & 0x1f));
		}
		memcpy(buf + index, &(rtpH->payload[2]), paylen - 2);
		index += (paylen - 2);
	}
	else {
		buf[index++] = 0;
		buf[index++] = 0;
		buf[index++] = 0;
		buf[index++] = 1;
		memcpy(buf + index, rtpH->payload, paylen);
		index += paylen;
	}
	// Move read pointer
	rtp_read += (paylen + sizeof(struct RtpCntHeader) + sizeof(struct RtspCntHeader));
	// Got a packet
	size = index;
	return true;
}

bool rtp_packet(uint8_t* buf, int& size)
{
	RtpCntHeader_st* rtpH = (RtpCntHeader_st*)(buf);
	if (0x60 != (rtpH->type & 0x7f)) {
		printf("No video stream %02x.\n", rtpH->type);
		// 不是RTP视频数据，不处理
		return false;
	}

	uint8_t* tmpBuf = (uint8_t*)malloc(size);
	if (!tmpBuf)
		return false;
	memset(tmpBuf, 0, size);
	int index = 0;
	// 将数据复制到packet buffer中，数据足够时使用mpp解码
	uint8_t h1 = rtpH->payload[0];
	uint8_t h2 = rtpH->payload[1];
	uint8_t nal = h1 & 0x1f;
	uint8_t flag = h2 & 0xe0;
	size_t paylen = size - sizeof(RtpCntHeader_st);
	if (0x1c == nal) {
		if (0x80 == flag) {
			tmpBuf[index++] = 0;
			tmpBuf[index++] = 0;
			tmpBuf[index++] = 0;
			tmpBuf[index++] = 1;
			tmpBuf[index++] = ((h1 & 0xe0) | (h2 & 0x1f));
		}
		memcpy(tmpBuf + index, &(rtpH->payload[2]), paylen - 2);
		index += (paylen - 2);
	}
	else {
		tmpBuf[index++] = 0;
		tmpBuf[index++] = 0;
		tmpBuf[index++] = 0;
		tmpBuf[index++] = 1;
		memcpy(tmpBuf + index, rtpH->payload, paylen);
		index += paylen;
	}
	// Move read pointer
	// Got a packet
	memcpy(buf, tmpBuf, index);
	size = index;
	free(tmpBuf);
	return true;
}

DWORD WINAPI RecvThread(LPVOID lpParamter) {
	SOCKET sockTcpClient = *((SOCKET*)lpParamter);
	char buf[4096] = { 0 };
	int	err;
	while (true) {
		err = recv(sockTcpClient, buf, sizeof(buf), 0);
		if (err == INVALID_SOCKET) {
			break;
		}
		printf("recv buf: %s\n", buf);
		memset(buf, 0, sizeof(buf));
	}
	return 0L;
}

BOOL ConsoleHandler(DWORD CtrlType) {
	switch (CtrlType) {
	case CTRL_C_EVENT:
		isRunning = false;
		break;
	default:
		break;
	}
	return TRUE;
}

bool ParseRtsp(const std::string& strUrl, std::string& userName, std::string& passwd,\
			   std::string& uri, std::string& ip, std::string& port) {
	std::string protocol, userAndPasswd, tmpStr, path, ipAndPort;

	//rtsp://admin:admin@192.168.10.80/streaming/channels/stream1
	size_t pos = strUrl.find("://");
	if (pos == std::string::npos) {
		printf("Protocol is not found\n");
		return false;
	}
	protocol = strUrl.substr(0, pos);
	tmpStr = strUrl.substr(pos + 3);

	if (_stricmp(protocol.c_str(), "rtsp")) {
		return false;
	}

	pos = tmpStr.rfind("@");
	if (pos == std::string::npos) {
		printf("Account info is not found\n");
		return false;
	}
	userAndPasswd = tmpStr.substr(0, pos);
	tmpStr = tmpStr.substr(pos + 1);

	pos = userAndPasswd.find(":");
	if (pos == std::string::npos) {
		printf("Incorrect account information format");
		return false;
	}
	userName = userAndPasswd.substr(0, pos);
	passwd = userAndPasswd.substr(pos + 1, userAndPasswd.size());

	pos = tmpStr.find("/");
	if (pos != std::string::npos) {
		ipAndPort = tmpStr.substr(0, pos);
		path = tmpStr.substr(pos);
	}

	pos = ipAndPort.find(":");
	if (pos != std::string::npos) {
		ip = ipAndPort.substr(0, pos);
		port = ipAndPort.substr(pos + 1);
	}
	else {
		ip = ipAndPort;
		port = std::to_string(554);
	}

	uri = "rtsp://" + ip + ":" + port + path;
	printf("tmpStr: %s, protocol: %s, userName: %s, passwd: %s, uri: %s, ip: %s, port: %s\n", \
		tmpStr.c_str(), protocol.c_str(), userName.c_str(), passwd.c_str(), uri.c_str(), ip.c_str(), port.c_str());

	return true;
}

int ParseRes(const std::string& res) {
	size_t pos = res.find(RTSP_VERSION);
	//printf("rtsp version: %d\n", strlen(RTSP_VERSION));
	if (pos != std::string::npos) {
		std::string code = res.substr(pos + strlen(RTSP_VERSION) + 1, pos + strlen(RTSP_VERSION) + 4);
		//printf("code: %s\n", code.c_str());
		return atoi(code.c_str());
	} else {
		printf("Response message error\n");
	}

	return 0;
}

bool ParseRes(const std::string& res, int& port) {
	size_t pos;
	std::string tmpStr;

	pos = res.find("server_port=");
	if (pos == std::string::npos) {
		printf("Response message error\n");
		return false;
	}
	tmpStr = res.substr(pos + 12);
	pos = tmpStr.find("\r\n");
	if (pos == std::string::npos) {
		printf("Response message error\n");
		return false;
	}
	port = atoi(tmpStr.substr(0, pos).c_str());

	return true;
}

bool ParseRes(const std::string& res, std::string& session) {
	size_t pos;
	std::string tmpStr;

	pos = res.find("Session: ");
	if (pos == std::string::npos) {
		printf("Response message error\n");
		return false;
	}
	tmpStr = res.substr(pos + 9);
	pos = tmpStr.find("\r\n");
	if (pos == std::string::npos) {
		printf("Response message error\n");
		return false;
	}
	session = tmpStr.substr(0, pos);
	printf("session: %s\n", session.c_str());

	return true;
}

bool ParseRes(const std::string& res, std::string& realm, std::string& nonce) {
	size_t pos1, pos2;
	pos1 = res.find(CER);
	if (pos1 == std::string::npos) {
		printf("Type of Certification is not %s", CER);
		return false;
	}

	//realm
	pos1 = res.find("realm=\"") + 7;
	pos2 = res.find_last_of(",") - 1;
	if (pos2 < pos1) {
		printf("realm is not found\n");
		return false;
	}
	realm = res.substr(pos1, pos2 - pos1);
	

	pos1 = res.find("nonce=\"") + 7;
	pos2 = res.find_last_of("\"");
	if (pos2 < pos1) {
		printf("nonce is not found\n");
		return false;
	}
	nonce = res.substr(pos1, pos2 - pos1);

	//printf("realm: %s, nonce: %s\n", realm.c_str(), nonce.c_str());

	return true;
}

bool ParseSdp(const std::string& res, std::string& strPath) {
	std::string tmpStr;
	size_t pos;
	pos = res.find("m=video");
	if (pos == std::string::npos) 
		return false;
	tmpStr = res.substr(pos);
	//printf("tmpStr: %s\n", tmpStr.c_str());

	pos = tmpStr.find("a=control:");
	if (pos == std::string::npos)
		return false;
	tmpStr = tmpStr.substr(pos);

	pos = tmpStr.find("\r\n");
	if (pos == std::string::npos)
		return false;
	strPath = tmpStr.substr(10, pos - 10);
	return true;
}

void SaveToFile(const char* fileName, const uint8_t* buf, size_t size) {
	FILE* fd = fopen(fileName, "ab+");
	if (!fd) {
		printf("open file failed\n");
		return;
	}
	size_t s = fwrite(buf, size, 1, fd);
	//printf("fwrite  size: %d, s: %d\n", size, s);
	fclose(fd);
}

bool Conn(const std::string& strUrl) {
	std::string userName, passwd, uri, ip, port;
	if (!ParseRtsp(strUrl, userName, passwd, uri, ip, port))
		return -1;

	WORD	wVersionRequested;
	WSADATA wsaData;
	int		err, iLen;

	SOCKET sockTcpClient;
	SOCKADDR_IN addrServer;

#if FLAG_UDP_TRANS
	SOCKET sockUdpClientRtp, sockUdpClientRtcp;
	SOCKADDR_IN addrClient;
	std::thread rtpRecv, rtcpRecv;
#endif

	int code = 0, size = 0;
	char tmpBuf[MAX_SIZE] = { 0 };
	std::string publicMethod = "DESCRIBE";
	std::string realm, nonce, responce, control, url, transport, session;
	int udpServerPort = 0;
	char videoDataFileName[] = "1.mp4";

	wVersionRequested = MAKEWORD(2, 2);//create 16bit data
	//(1)Load WinSock
	err = WSAStartup(wVersionRequested, &wsaData);	//load win socket
	if (err != 0) {
		printf("Load WinSock Failed!\n");
		return false;
	}

	sockTcpClient = socket(AF_INET, SOCK_STREAM, 0);
	if (sockTcpClient == INVALID_SOCKET) {
		printf("Func socket error, error: %d\n", GetLastError());
		return false;
	}

	addrServer.sin_family = AF_INET;
	addrServer.sin_addr.s_addr = inet_addr(ip.c_str());
	addrServer.sin_port = htons(atoi(port.c_str()));

#if FLAG_UDP_TRANS
	sockUdpClientRtp = socket(AF_INET, SOCK_DGRAM, 0);
	sockUdpClientRtcp = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockUdpClientRtp == INVALID_SOCKET || sockUdpClientRtcp == INVALID_SOCKET) {
		printf("Func socket error, error: %d\n", GetLastError());
		return false;
	}
#endif

#if FLAG_UDP_TRANS
	addrClient.sin_family = AF_INET;
	addrClient.sin_addr.s_addr = 0;
	addrClient.sin_port = htons(8001);
	err = bind(sockUdpClientRtp, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
	if (err == INVALID_SOCKET) {
		printf("bind error, error: %d\n", GetLastError());
		goto Fail;
	}
	addrClient.sin_port = htons(8002);
	err = bind(sockUdpClientRtcp, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
	if (err == INVALID_SOCKET) {
		printf("bind error, error: %d\n", GetLastError());
		goto Fail;
	}
#endif

	err = connect(sockTcpClient, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
	if (err == INVALID_SOCKET) {
		printf("connect error\n");
		goto Fail;
	}

	//HANDLE hThread = CreateThread(NULL, 0, RecvThread, (LPVOID)&sockTcpClient, 0, NULL);
	//CloseHandle(hThread);

	//OPTION
	sprintf(tmpBuf, "OPTIONS %s %s\r\nCSeq: %d\r\nUser-Agent: %s\r\n\r\n\0", uri.c_str(),\
			RTSP_VERSION, CSeq++, USER_AGENT);
	send(sockTcpClient, tmpBuf, strlen(tmpBuf) + 1, 0);
	recv(sockTcpClient, tmpBuf, sizeof(tmpBuf), 0);
	code = ParseRes(tmpBuf);
	//printf("code: %d\n", code);
	if (code != 200) {
		printf("Error, recv: %s\n", tmpBuf);
		goto Fail;
	}

	//DESCRIBE
	sprintf(tmpBuf, "DESCRIBE %s %s\r\nCSeq: %d\r\nUser-Agent: %s\r\nAccept: application/sdp\r\n\r\n\0",\
			uri.c_str(), RTSP_VERSION, CSeq++, USER_AGENT);
	send(sockTcpClient, tmpBuf, strlen(tmpBuf) + 1, 0);
	recv(sockTcpClient, tmpBuf, sizeof(tmpBuf), 0);
	//printf("recv: %s\n", tmpBuf);
	code = ParseRes(tmpBuf);
	//printf("code: %d\n", code);
	if (code != 200) {
		if (code == 401) {
			//printf("Need certification\n");
			if (!ParseRes(tmpBuf, realm, nonce)) {
				printf("Parse WWW-Authenticate info error");
				goto Fail;
			}
		}
		else {
			printf("Error, recv: %s\n", tmpBuf);
			goto Fail;
		}
	}

	//RE DESCRIBE
	//calc md5
	//response = md5(md5(username:realm:password):nonce:md5(public_method:url))
	responce = GetMD5(GetMD5(userName + ":" + realm + ":" + passwd) + ":"
							 + nonce + ":" + GetMD5(publicMethod + ":" + uri));
	sprintf(tmpBuf, "DESCRIBE %s %s\r\nCSeq: %d\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nUser-Agent: %s\r\nAccept: application/sdp\r\n\r\n\0",\
			uri.c_str(), RTSP_VERSION, CSeq++, userName.c_str(), realm.c_str(), nonce.c_str(), uri.c_str(), responce.c_str(), USER_AGENT);
	send(sockTcpClient, tmpBuf, strlen(tmpBuf) + 1, 0);
	recv(sockTcpClient, tmpBuf, sizeof(tmpBuf), 0);
	//printf("recv: %s\n", tmpBuf);
	code = ParseRes(tmpBuf);
	//printf("code: %d\n", code);
	if (code != 200) {
		printf("Error, recv: %s\n", tmpBuf);
		goto Fail;
	}
	ParseSdp(tmpBuf, control);
	if (control.find("rtsp://") != std::string::npos) {
		url = control;
	}
	else {
		url = uri + "/" + control;
	}

	//SETUP
#if FLAG_UDP_TRANS
	transport = UDP_TRANS;
#else
	transport = TCP_TRANS;
#endif
	sprintf(tmpBuf, "SETUP %s %s\r\nCSeq: %d\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nUser-Agent: %s\r\nTransport: %s\r\n\r\n\0", \
		url.c_str(), RTSP_VERSION, CSeq++, userName.c_str(), realm.c_str(), nonce.c_str(), uri.c_str(), responce.c_str(), USER_AGENT, transport.c_str());
	send(sockTcpClient, tmpBuf, strlen(tmpBuf) + 1, 0);
	recv(sockTcpClient, tmpBuf, sizeof(tmpBuf), 0);
	//printf("recv: %s\n", tmpBuf);
	code = ParseRes(tmpBuf);
	//printf("code: %d\n", code);
	if (code != 200) {
		printf("Error, recv: %s\n", tmpBuf);
		goto Fail;
	}
	ParseRes(tmpBuf, session);

#if FLAG_UDP_TRANS
	if (!ParseRes(tmpBuf, udpServerPort)) {
		printf("Get server port error\n");
		goto Fail;
	}

	printf("tmpBuf: %s, port: %d\n", tmpBuf, udpServerPort);

	rtpRecv = std::thread([&]() {
		char buf[MAX_SIZE] = { 0 };
		addrServer.sin_port = htons(udpServerPort);
		int ret, size;
		ret = connect(sockUdpClientRtp, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
		if (ret == INVALID_SOCKET) {
			return;
		}

		while (isRunning) {
			size = recv(sockUdpClientRtp, buf, MAX_SIZE, 0);
			if (size > 0) {
				//SaveToFile("./1.rtp", (uint8_t*)buf, size);

				//rtsp_read((uint8_t*)tmpBuf, size);
				if (rtp_packet((uint8_t*)buf, size)) {
					SaveToFile(videoDataFileName, (uint8_t*)buf, size);
				}
			}
		}

		return;
		});
	rtcpRecv = std::thread([&]() {
		char buf[MAX_SIZE] = { 0 };
		addrServer.sin_port = htons(udpServerPort + 1);
		int ret, size;
		ret = connect(sockUdpClientRtcp, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
		if (ret == INVALID_SOCKET) {
			return;
		}

		while (isRunning) {
			size = recv(sockUdpClientRtcp, buf, MAX_SIZE, 0);
			printf("rtcp recv size: %d, buf: %s\n", size, buf);
			if (size > 0) {
				SaveToFile("./1.rtcp", (uint8_t*)buf, size);
			}
		}
		return;
		});
#endif

	//PLAY
	sprintf(tmpBuf, "PLAY %s %s\r\nCSeq: %d\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nUser-Agent: %s\r\nSession: %s\r\nRange: npt=0-\r\n\r\n\0", \
		url.c_str(), RTSP_VERSION, CSeq++, userName.c_str(), realm.c_str(), nonce.c_str(), uri.c_str(), responce.c_str(), USER_AGENT, session.c_str());
	send(sockTcpClient, tmpBuf, strlen(tmpBuf) + 1, 0);
	recv(sockTcpClient, tmpBuf, sizeof(tmpBuf), 0);
	//printf("recv: %s\n", tmpBuf);
	code = ParseRes(tmpBuf);
	//printf("code: %d\n", code);
	if (code != 200) {
		printf("Error, recv: %s\n", tmpBuf);
		goto Fail;
	}

	//
	while (isRunning) {
		size = recv(sockTcpClient, tmpBuf, sizeof(tmpBuf), 0);
		//printf("recv %d data\n", size);	
		if (size > 0) {
#if 1	
			rtsp_read((uint8_t*)tmpBuf, size);
			if (rtsp_packet((uint8_t*)tmpBuf, size)) {
				printf("save %d data\n", size);
				SaveToFile(videoDataFileName, (uint8_t*)tmpBuf, size);
			}
#else
		SaveFrame(videoDataFileName, (uint8_t*)tmpBuf, size);
#endif
		}
		memset(tmpBuf, 0, sizeof(tmpBuf));
	}

#if FLAG_UDP_TRANS
	if (rtpRecv.joinable()) {
		rtpRecv.join();
	}
	if (rtcpRecv.joinable()) {
		rtcpRecv.join();
	}
#endif

	//TEARDOWN
	sprintf(tmpBuf, "TEARDOWN %s %s\r\nCSeq: %d\r\nAuthorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\"\r\nUser-Agent: %s\r\nSession: %s\r\n\r\n\0", \
		url.c_str(), RTSP_VERSION, CSeq++, userName.c_str(), realm.c_str(), nonce.c_str(), uri.c_str(), responce.c_str(), USER_AGENT, session.c_str());
	//printf("send: %s\n", tmpBuf);
	send(sockTcpClient, tmpBuf, strlen(tmpBuf) + 1, 0);
	while (true) {
		memset(tmpBuf, 0, sizeof(tmpBuf));
		size = recv(sockTcpClient, tmpBuf, sizeof(tmpBuf), 0);
		if (size == SOCKET_ERROR) {
			break;
		}
	}


	//Sleep(2 * 1000);
Fail:
	closesocket(sockTcpClient);
	return true;
}


int main() {
	
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE)) {
		isRunning = true;
		std::string strUrl = "rtsp://admin:admin@192.168.10.80/streaming/channels/stream1";
		rtp_content = (uint8_t*)malloc(16 * 1024 * 1024);
		Conn(strUrl);
	}
	//system("pause");
	return 0;
}