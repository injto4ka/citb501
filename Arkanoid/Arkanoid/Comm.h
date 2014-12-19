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
	WSADATA wsaData;
	SOCKADDR_IN m_address;
public:
	int error;
	Socket(BYTE revision = 2, BYTE version = 2);
	~Socket();
	BOOL Accept(Socket &sock);
	BOOL Connect(const char *ip, WORD port);
	BOOL Listen(WORD port);
	void Disconnect();
	BOOL Receive(void *buffer, int n);
	BOOL Send(const void *buffer, int n);
	const WSADATA *Info() const {return &wsaData;}
};

#endif // __LIOCOM_H__