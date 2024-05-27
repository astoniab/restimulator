#include "restimtcpconnection.h"

#ifdef _WIN32
#include <WS2tcpip.h>
#pragma comment(lib,"ws2_32")
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include <array>

//debug
#include <iostream>

RestimTCPConnection::RestimTCPConnection() :RestimConnection(), m_socket(INVALID_SOCKET), m_connected(false)
{
#ifdef _WIN32
	WSAData wsadata;
	WSAStartup(MAKEWORD(2, 2), &wsadata);
#endif

}

RestimTCPConnection::~RestimTCPConnection()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

bool RestimTCPConnection::SendTCode(const std::string& tcode)
{
	if (IsConnected())
	{
		std::lock_guard<std::mutex> guard(m_tcodebuffermutex);
		m_tcodebuffer.push(tcode);
		return true;
	}
	return false;
}

void RestimTCPConnection::Run(const ThreadParameters* threadparameters)
{
	try
	{
		const RestimTCPConnectionParameters params = *(dynamic_cast<const RestimTCPConnectionParameters*>(threadparameters));
		m_host = params.m_host;
		m_port = params.m_port;

		while (!m_stop)
		{
			if (IsConnected() == false)
			{
				if (!Connect())
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				}
			}
			else
			{
				UpdateSocket();
			}
		}
	}
	catch (std::exception& e)
	{
		std::cout << "RestimTCPConnection::Run Caught " << e.what() << std::endl;
	}
	catch (...)
	{
		std::cout << "RestimTCPConnection::Run Caught unknown exception" << std::endl;
	}
}

bool RestimTCPConnection::Connect()
{
	int rval = -1;
	addrinfo hint, * result;
	std::string portstr{ std::to_string(m_port) };
	result = 0;
	std::memset(&hint, 0, sizeof(hint));
	hint.ai_socktype = SOCK_STREAM;

	rval = getaddrinfo(m_host.c_str(), portstr.c_str(), &hint, &result);

	if (rval == 0 && result)
	{
		for (addrinfo* current = result; current != nullptr && m_socket == INVALID_SOCKET; current = current->ai_next)
		{
			{
				std::lock_guard<std::mutex> guard(m_socketmutex);
				m_socket = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
			}

			if (m_socket != INVALID_SOCKET)
			{
				{
					std::lock_guard<std::mutex> guard(m_socketmutex);
					rval = connect(m_socket, current->ai_addr, current->ai_addrlen);
				}
				if (rval != 0)
				{
					Disconnect();
				}
				else
				{
					{
						std::lock_guard<std::mutex> guard(m_socketmutex);
						int yes = 1;
						setsockopt(m_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&yes, sizeof(int));
					}

					// clear any existing data in buffer
					std::lock_guard<std::mutex> guard(m_tcodebuffermutex);
					while (!m_tcodebuffer.empty())
					{
						m_tcodebuffer.pop();
					}
					m_connected = true;
				}
			}
			else
			{
				m_connected = false;
			}
		}

		freeaddrinfo(result);
	}
	else if (result)
	{
		freeaddrinfo(result);
	}

	return (rval == 0);
}

bool RestimTCPConnection::Disconnect()
{
	if (IsConnected())
	{
		std::lock_guard<std::mutex> guard(m_socketmutex);
#ifdef _WIN32
		closesocket(m_socket);
#else
		close(m_socket);
#endif
		m_socket = INVALID_SOCKET;
		m_connected = false;
	}
	return true;
}

bool RestimTCPConnection::IsConnected()
{
	/*
	std::lock_guard<std::mutex> guard(m_socketmutex);
	return m_socket != INVALID_SOCKET;
	*/
	return m_connected;
}

void RestimTCPConnection::UpdateSocket()
{
	if (IsConnected())
	{
		fd_set readfs;
		fd_set writefs;
		struct timeval tv;

		tv.tv_sec = 0;
		tv.tv_usec = 1000;		// 1ms

		FD_ZERO(&readfs);
		FD_ZERO(&writefs);

		FD_SET(m_socket, &readfs);
		{
			std::lock_guard<std::mutex> guard(m_tcodebuffermutex);
			if (!m_tcodebuffer.empty())
			{
				FD_SET(m_socket, &writefs);
			}
		}

		select(m_socket + 1, &readfs, &writefs, 0, &tv);

		if (FD_ISSET(m_socket, &readfs))
		{
			SocketRecv();
		}
		if (IsConnected() && FD_ISSET(m_socket, &writefs))
		{
			SocketSend();
		}
	}
}

void RestimTCPConnection::SocketSend()
{
	if (IsConnected())
	{
		std::lock_guard<std::mutex> guard(m_tcodebuffermutex);
		std::vector<char> data;
		while (!m_tcodebuffer.empty())
		{
			data.insert(data.end(), m_tcodebuffer.front().begin(), m_tcodebuffer.front().end());
			data.push_back('\n');
			m_tcodebuffer.pop();
		}
		while (IsConnected() && data.size() > 0)
		{
			int len = send(m_socket, &data[0], data.size(), 0);
			if (len > 0)
			{
				data.erase(data.begin(), data.begin() + len);
			}
			else
			{
				Disconnect();
			}
		}
	}
}

void RestimTCPConnection::SocketRecv()
{
	if (IsConnected())
	{
		std::array<char, 4096> receivebuffer;
		int len = recv(m_socket, &receivebuffer[0], receivebuffer.size(), 0);
		if (len > 0)
		{
			// restim doesn't send anything currently, so just discard
		}
		else
		{
			Disconnect();
		}
	}
}
