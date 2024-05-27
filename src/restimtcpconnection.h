#pragma once

#include "restimconnection.h"

#include <queue>
#include <string>
#include <mutex>

#ifdef _WIN32
#include <WinSock2.h>
#endif

class RestimTCPConnection :public RestimConnection
{
public:
	RestimTCPConnection();
	~RestimTCPConnection();

	struct RestimTCPConnectionParameters :public IThread::ThreadParameters
	{
		std::string m_host{ "" };
		int32_t m_port{ -1 };
	};

	bool SendTCode(const std::string& tcode);

	bool IsConnected();

private:

	void Run(const ThreadParameters* threadparameters);

	bool Connect();
	bool Disconnect();
	void UpdateSocket();
	void SocketSend();
	void SocketRecv();

#ifndef _WIN32
	static const int INVALID_SOCKET = -1;
#endif

	std::mutex m_socketmutex;
#ifdef _WIN32
	SOCKET m_socket;
#else
	int m_socket;
#endif
	std::string m_host;
	int32_t m_port;
	std::mutex m_tcodebuffermutex;
	std::queue<std::string> m_tcodebuffer;
	std::atomic<bool> m_connected;

};
