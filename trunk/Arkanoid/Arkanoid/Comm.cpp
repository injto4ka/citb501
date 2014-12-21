#include "comm.h"

#pragma comment(lib, "ws2_32.lib" )

Socket::Socket(): m_socket(INVALID_SOCKET)
{
	memset(&m_address, 0, sizeof(m_address));
}
Socket::~Socket()
{
	Disconnect();
}
WSADATA Socket::StartComm(BYTE revision, BYTE version)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(revision,version), &wsaData);
	return wsaData;
}
void Socket::StopComm()
{
	WSACleanup();
}
void Socket::Disconnect()
{
	if(m_socket != INVALID_SOCKET)
    {
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		memset(&m_address, 0, sizeof(m_address));
    }
}
ErrorCode Socket::Listen(WORD port)
{
	Disconnect();
	SOCKET sock;
	ErrorCode err = Create(sock);
	if (err)
		return err;
	SOCKADDR_IN addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if(::bind(sock, (SOCKADDR*) &addr, sizeof(addr)) == SOCKET_ERROR)
	{
		closesocket(sock);
		switch(WSAGetLastError())
		{
			case WSANOTINITIALISED: return "Note  A successful WSAStartup call must occur before using this function.";
			case WSAENETDOWN: return "The network subsystem has failed.";
			case WSAEACCES: return "An attempt to bind a datagram socket to the broadcast address failed because the setsockopt option is not enabled.";
			case WSAEADDRINUSE: return "A process on the computer is already bound to the same fully-qualified address and the socket has not been marked to allow address reuse.";
			case WSAEADDRNOTAVAIL: return "The specified address pointed to by the name parameter is not a valid local IP address on this computer.";
			case WSAEFAULT: return "The name or namelen parameter is not a valid part of the user address space, the namelen parameter is too small, the name parameter contains an incorrect address format for the associated address family, or the first two bytes of the memory block specified by name does not match the address family associated with the socket descriptor.";
			case WSAEINPROGRESS: return "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
			case WSAEINVAL: return "The socket is already bound to an address.";
			case WSAENOBUFS: return "Not enough buffers are available, too many connections.";
			case WSAENOTSOCK: return "The descriptor in the s parameter is not a socket.";
			default: return "Unknown bind error";
		}
	}
	if(::listen(sock, SOMAXCONN) == SOCKET_ERROR)
	{
		closesocket(sock);
		switch(WSAGetLastError())
		{
			case WSANOTINITIALISED: return "A successful WSAStartup call must occur before using this function.";
			case WSAENETDOWN: return "The network subsystem has failed.";
			case WSAEADDRINUSE: return "The socket's local address is already in use and the socket was not marked to allow address reuse.";
			case WSAEINPROGRESS: return "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
			case WSAEINVAL: return "The socket has not been bound with bind.";
			case WSAEISCONN: return "The socket is already connected.";
			case WSAEMFILE: return "No more socket descriptors are available.";
			case WSAENOBUFS: return "No buffer space is available.";
			case WSAENOTSOCK: return "The descriptor is not a socket.";
			case WSAEOPNOTSUPP: return "The referenced socket is not of a type that supports the listen operation.";
			default: return "Unknown listen error";
		}
	}
	m_socket = sock;
	m_address = addr;
	return NO_ERROR;
}
ErrorCode Socket::Accept(Socket &sock)
{
	if( m_socket == INVALID_SOCKET)
		return "Not listening";
	sock.Disconnect();
	int addrlen = sizeof(sock.m_address);
	sock.m_socket = ::accept(m_socket, (SOCKADDR*)&sock.m_address, &addrlen);
	if( m_socket == INVALID_SOCKET )
	{
		switch(WSAGetLastError())
		{
			case WSANOTINITIALISED: return "A successful WSAStartup call must occur before using this function.";
			case WSAECONNRESET: return "An incoming connection was indicated, but was subsequently terminated by the remote peer prior to accepting the call.";
			case WSAEFAULT: return "The addrlen parameter is too small or addr is not a valid part of the user address space.";
			case WSAEINTR: return "A blocking Windows Sockets 1.1 call was canceled.";
			case WSAEINVAL: return "The listen function was not invoked prior to accept.";
			case WSAEINPROGRESS: return "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
			case WSAEMFILE: return "The queue is nonempty upon entry to accept and there are no descriptors available.";
			case WSAENETDOWN: return "The network subsystem has failed.";
			case WSAENOBUFS: return "No buffer space is available.";
			case WSAENOTSOCK: return "The descriptor is not a socket.";
			case WSAEOPNOTSUPP: return "The referenced socket is not a type that supports connection-oriented service.";
			case WSAEWOULDBLOCK: return "The socket is marked as nonblocking and no connections are present to be accepted.";
			default: return "Unknown accept error";
		}
	}
	return NO_ERROR;
}
ErrorCode Socket::Create(SOCKET &sock)
{
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock != INVALID_SOCKET)
		return NO_ERROR;
	switch( WSAGetLastError() )
	{
		case WSANOTINITIALISED: return "A successful WSAStartup call must occur before using this function.";
		case WSAENETDOWN: return "The network subsystem or the associated service provider has failed.";
		case WSAEAFNOSUPPORT: return "The specified address family is not supported.";
		case WSAEINPROGRESS: return "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
		case WSAEMFILE: return "No more socket descriptors are available.";
		case WSAENOBUFS: return "No buffer space is available. The socket cannot be created.";
		case WSAEPROTONOSUPPORT: return "The specified protocol is not supported.";
		case WSAEPROTOTYPE: return "The specified protocol is the wrong type for this socket.";
		case WSAESOCKTNOSUPPORT: return "The specified socket type is not supported in this address family.";
		default: return "Unknown creation error";
	}
}
ErrorCode Socket::Connect(const char *ip, WORD port)
{
	Disconnect();
	SOCKADDR_IN addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if(!ip)
	{
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else if(strcmp(ip,"localhost") == 0)
	{
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	}
	else if(strcmp(ip,"broadcast") == 0)
	{
		addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	}
	else
	{
		ULONG tmp = inet_addr(ip);
		if(tmp == INADDR_NONE)
			return "Malformed internet address";
		addr.sin_addr.s_addr = tmp;
	}
	SOCKET sock;
	ErrorCode err = Create(sock);
	if (err)
		return err;
	if(::connect(sock, (SOCKADDR*) &addr, sizeof(addr)) == SOCKET_ERROR)
	{
		closesocket(sock);
		switch(WSAGetLastError())
		{
			case WSANOTINITIALISED: return "A successful WSAStartup call must occur before using this function.";
			case WSAENETDOWN: return "The network subsystem has failed.";
			case WSAEADDRINUSE: return "The socket's local address is already in use.";
			case WSAEINTR: return "The blocking Windows Socket 1.1 call was canceled.";
			case WSAEINPROGRESS: return "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
			case WSAEALREADY: return "A nonblocking connect call is in progress on the specified socket.";
			case WSAEADDRNOTAVAIL: return "The remote address is not a valid address.";
			case WSAEAFNOSUPPORT: return "Addresses in the specified family cannot be used with this socket.";
			case WSAECONNREFUSED: return "The attempt to connect was forcefully rejected.";
			case WSAEFAULT: return "The sockaddr structure pointed to by the name contains incorrect address format for the associated address family or the namelen parameter is too small.";
			case WSAEINVAL: return "The parameter s is a listening socket.";
			case WSAEISCONN: return "The socket is already connected (connection-oriented sockets only).";
			case WSAENETUNREACH: return "The network cannot be reached from this host at this time.";
			case WSAEHOSTUNREACH: return "A socket operation was attempted to an unreachable host.";
			case WSAENOBUFS: return "Note  No buffer space is available. The socket cannot be connected.";
			case WSAENOTSOCK: return "The descriptor specified in the s parameter is not a socket.";
			case WSAETIMEDOUT: return "An attempt to connect timed out without establishing a connection.";
			case WSAEWOULDBLOCK: return "The socket is marked as nonblocking and the connection cannot be completed immediately.";
			case WSAEACCES: return "An attempt to connect a datagram socket to broadcast address failed because setsockopt option is not enabled.";
			default: return "Unknown connection error";
		}
	}
	m_socket = sock;
	m_address = addr;
	return NO_ERROR;
}
ErrorCode Socket::Receive(void *buffer, int &bytes) const
{
	bytes = ::recv(m_socket, (char *)buffer, bytes, 0);
	if(bytes != SOCKET_ERROR)
		return NO_ERROR;
	switch(WSAGetLastError())
	{
		case WSANOTINITIALISED: return "A successful WSAStartup call must occur before using this function.";
		case WSAENETDOWN: return "The network subsystem has failed.";
		case WSAEFAULT: return "The buf parameter is not completely contained in a valid part of the user address space.";
		case WSAENOTCONN: return "The socket is not connected.";
		case WSAEINTR: return "The (blocking) call was canceled.";
		case WSAEINPROGRESS: return "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
		case WSAENETRESET: return "For a connection-oriented socket, this error indicates that the connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.";
		case WSAENOTSOCK: return "The descriptor is not a socket.";
		case WSAEOPNOTSUPP: return "MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only send operations.";
		case WSAESHUTDOWN: return "The socket has been shut down.";
		case WSAEWOULDBLOCK: return "The socket is marked as nonblocking and the receive operation would block.";
		case WSAEMSGSIZE: return "The message was too large to fit into the specified buffer and was truncated.";
		case WSAEINVAL: return "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled or (for byte stream sockets only) len was zero or negative.";
		case WSAECONNABORTED: return "The virtual circuit was terminated due to a time-out or other failure. The application should close the socket as it is no longer usable.";
		case WSAETIMEDOUT: return "The connection has been dropped because of a network failure or because the peer system failed to respond.";
		case WSAECONNRESET: return "The virtual circuit was reset by the remote side executing a hard or abortive close.";
		default: return "Unknown receive error";
	}
}
ErrorCode Socket::Send(const void *buffer, int &bytes) const
{
	bytes = ::send(m_socket, (const char *)buffer, bytes, 0);
	if(bytes != SOCKET_ERROR)
		return NO_ERROR;
	switch(WSAGetLastError())
	{
		case WSANOTINITIALISED: return "A successful WSAStartup call must occur before using this function.";
		case WSAENETDOWN: return "The network subsystem has failed.";
		case WSAEACCES: return "The requested address is a broadcast address, but the appropriate flag was not set.";
		case WSAEINTR: return "A blocking Windows Sockets 1.1 call was canceled.";
		case WSAEINPROGRESS: return "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function.";
		case WSAEFAULT: return "The buf parameter is not completely contained in a valid part of the user address space.";
		case WSAENETRESET: return "The connection has been broken due to the keep-alive activity detecting a failure while the operation was in progress.";
		case WSAENOBUFS: return "No buffer space is available.";
		case WSAENOTCONN: return "The socket is not connected.";
		case WSAENOTSOCK: return "The descriptor is not a socket.";
		case WSAEOPNOTSUPP: return "MSG_OOB was specified, but the socket is not stream-style such as type SOCK_STREAM, OOB data is not supported in the communication domain associated with this socket, or the socket is unidirectional and supports only receive operations.";
		case WSAESHUTDOWN: return "The socket has been shut down.";
		case WSAEWOULDBLOCK: return "The socket is marked as nonblocking and the requested operation would block.";
		case WSAEMSGSIZE: return "The socket is message oriented, and the message is larger than the maximum supported by the underlying transport.";
		case WSAEHOSTUNREACH: return "The remote host cannot be reached from this host at this time.";
		case WSAEINVAL: return "The socket has not been bound with bind, or an unknown flag was specified, or MSG_OOB was specified for a socket with SO_OOBINLINE enabled.";
		case WSAECONNABORTED: return "The virtual circuit was terminated due to a time-out or other failure.";
		case WSAECONNRESET: return "The virtual circuit was reset by the remote side executing a hard or abortive close.";
		case WSAETIMEDOUT: return "The connection has been dropped, because of a network failure or because the system on the other end went down without notice.";
		default: return "Unknown sending error";
	}
}
const char *Socket::IP() const
{
	return m_socket != INVALID_SOCKET ? inet_ntoa(m_address.sin_addr) : NULL;
}
unsigned short Socket::Port() const
{
	return m_socket != INVALID_SOCKET ? ntohs(m_address.sin_port) : 0;
}
BOOL Socket::WaitingData(int wait_ms) const
{
	if( m_socket == INVALID_SOCKET)
		return FALSE;
	timeval time = {0};
	time.tv_usec = wait_ms * 1000;
	fd_set set = {0};
	FD_SET(m_socket, &set);
	return select(1, &set, NULL, NULL, &time) > 0;
}
BOOL Socket::IsConnected() const
{
	if( m_socket == INVALID_SOCKET)
		return FALSE;
	int opt;
    int len = sizeof(opt);
    if(::getsockopt(m_socket,  SOL_SOCKET, SO_CONNECT_TIME, (char*)&opt, &len) == SOCKET_ERROR)
		return FALSE;
	return opt >= 0;
}
BOOL Socket::IsListening() const
{
	if( m_socket == INVALID_SOCKET)
		return FALSE;
	int opt;
    int len = sizeof(opt);
    if(::getsockopt(m_socket,  SOL_SOCKET, SO_ACCEPTCONN, (char*)&opt, &len) == SOCKET_ERROR)
		return FALSE;
	return !!opt;
}