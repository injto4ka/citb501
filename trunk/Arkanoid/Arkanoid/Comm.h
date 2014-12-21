#ifndef __COMM_H__
#define __COMM_H__

#include "utils.h"

#define IP_BROADCAST "255.255.255.255"
#define IP_LOCALHOST "127.0.0.1"
#define IP_NOTSPECIF "0.0.0.0"

#define SOCKET_REV 2
#define SOCKET_VER 2

class Socket
{
public:
	SOCKET m_socket;
	SOCKADDR_IN m_address;
	Socket();
	void Disconnect();
	const char *IP() const;
	unsigned short Port() const;
	static WSADATA StartComm(BYTE revision = 2, BYTE version = 2);
	static void StopComm();
	BOOL WaitingData(int wait_ms = 0) const;
};

class Client: public Socket
{
public:
	ErrorCode Connect(const char *ip, WORD port);
	ErrorCode Receive(void *buffer, int &n) const;
	ErrorCode Send(const void *buffer, int &n) const;
	BOOL IsConnected() const;
};

class Server: public Socket 
{
public:
	BOOL IsListening() const;
	ErrorCode Accept(Client &client);
	ErrorCode Listen(WORD port);
	ErrorCode WaitingCount(const std::list<Client> &clients, int &count, int wait_ms = 0) const;
};

#endif // __LIOCOM_H__