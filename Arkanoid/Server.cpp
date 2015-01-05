#include <list>

#include "utils.h"
#include "comm.h"

int port = 12345;
Server server;
std::list<Client> clients;

int main(int argc, char *argv[], char *envp[])
{
	if( argc > 1 )
	{
		int n = atoi(argv[0]);
		if( n > 0 )
			port = n;
	}

	Socket::StartComm();

	ErrorCode err = server.Listen(port);
	if( err )
	{
		Print("Error starting listening port %d: %s\n", port, err);
		Socket::StopComm();
		return -1;
	}
	Print("Started listening port %d\n", server.Port());

	bool bUpdate = false;
	while (server.IsListening())
	{
		if( bUpdate )
		{
			bUpdate = false;
		}
		else
		{
			int count = 0;
			err = server.WaitingCount(clients, count, 10);
			if( err )
			{
				Print("Error waiting: %s\n", err);
				Socket::StopComm();
				return -1;
			}
			if( count <= 0 )
				continue;
			if( server.WaitingData() )
			{
				Client client;
				ErrorCode err = server.Accept(client);
				if( err )
				{
					Print("Error accepting: %s\n", err);
				}
				else
				{
					Print("Connected client %s:%d\n", client.IP(), client.Port());
					clients.push_back(client);
				}
			}
		}
		auto it = clients.begin();
		while(it != clients.end())
		{
			auto i = it++;
			Client &client = *i;
			if( !client.IsConnected() )
			{
				Print("Removing a closed connection.\n");
				clients.erase(i);
				continue;
			}
			if( !client.WaitingData() )
				continue;
			char buffer[1024];
			int received = sizeof(buffer);
			ErrorCode err = client.Receive(buffer, received);
			if( err )
			{
				Print("Error receiving from %s:%d: %s\n", client.IP(), client.Port(), err);
				client.Disconnect();
				bUpdate = true;
			}
			else if( received > 0 )
			{
				Print("Receiving %d bytes from %s:%d\n", received, client.IP(), client.Port());
				for(auto j = clients.begin(); j != clients.end(); j++)
				{
					Client &client = *j;
					int sent = received; 
					ErrorCode err = client.Send(buffer, sent);
					if( err )
					{
						Print("Error sending to %s:%d %s\n", client.IP(), client.Port(), err);
						client.Disconnect();
						bUpdate = true;
					}
					else if( sent != received )
					{
						Print("Failed to send all to %s:%d %s\n", client.IP(), client.Port(), err);
					}
				}
			}
			else
			{
				Print("Disconnected client %s:%d.\n", client.IP(), client.Port());
				client.Disconnect();
				bUpdate = true;
			}
		}
	}

	Print("Server stopped.\n");
	Socket::StopComm();
	return 0;
}
