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
protected:
	SOCKET m_socket;
	SOCKADDR_IN m_address;
	static ErrorCode Create(SOCKET &sock);
public:
	Socket();
	~Socket();
	void Disconnect();
	ErrorCode Accept(Socket &sock);
	ErrorCode Connect(const char *ip, WORD port);
	ErrorCode Listen(WORD port);
	ErrorCode Receive(void *buffer, int &n) const;
	ErrorCode Send(const void *buffer, int &n) const;
	BOOL IsConnected() const;
	BOOL IsListening() const;
	const char *IP() const;
	unsigned short Port() const;
	BOOL WaitingData(int wait_ms = 0) const;
	static WSADATA StartComm(BYTE revision = 2, BYTE version = 2);
	static void StopComm();
};

#endif // __LIOCOM_H__