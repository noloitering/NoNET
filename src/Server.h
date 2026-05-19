#ifndef SERVER_H
#define SERVER_H

// name conflicts with raylib
#define CloseWindow CloseWinWindow
#define Rectangle WinRectangle
#define ShowCursor ShowWinCursor
#define LoadImage LoadWinImage
#define DrawText DrawWinText
#define DrawTextEx DrawWinTextEx
#define PlaySound PlayWinSound
#include <enet/enet.h>
#undef CloseWindow
#undef Rectangle
#undef ShowCursor
#undef LoadImage
#undef DrawText
#undef DrawTextEx
#undef PlaySound

#include <vector>
#include <iostream>
#include <memory>
#include "raylib.h"

namespace NoNET
{
	// TODO: read from config file
	const size_t DEFAULTCHANNELS = 2;
	const size_t MAXCONNECTIONS = 32;
	const size_t MAXCLIENTNAME =  32;
	const enet_uint16 DEFAULTPORT = 1234;
	//TODO: I just threw static here to shut the compiler up. Find the actual correct design decision to handle this
	static const ENetAddress DEFAULTADDRESS = (ENetAddress){ENET_HOST_ANY, NoNET::DEFAULTPORT};
	enum State {OFFLINE, ONLINE};
	const enet_uint8 MSG_JOIN = 0b00000001;
	const enet_uint8 MSG_BROADCAST = 0b00000010;
	
	struct Bandwidth
	{
		enet_uint32 in = 0; // unlimited incoming bandwidth
		enet_uint32 out = 0; // unlimited outgoing bandwidth
	};
	
	// TODO: tempted to make this a pair with the clients id and the contents since opcode will always be just the 0th element in the data
	struct MsgInfo
	{
		enet_uint8 opcode;
		size_t id;
		const char* contents;
	};
	
	class ClientInfo
	{
	private:
		size_t id = 0;
		char name[MAXCLIENTNAME];
		bool connected=true;
		ENetPeer* peer = nullptr;
	public:
		ClientInfo(size_t clientid, const char* clientname, ENetPeer* clientpeer)
			: id(clientid), peer(clientpeer) {name[0] = '\0'; strncat(name, clientname, MAXCLIENTNAME - 1);}
		size_t getId()
		{
			
			return id;
		}
		bool getConnected()
		{
			
			return connected;
		}
		const char* getName()
		{
			
			return name;
		}
		ENetPeer* getPeer()
		{
			
			return peer;
		}
		ENetAddress getAddress()
		{
			
			return peer->address;
		}
		const char* getHostName()
		{
			char hostname[39];
			enet_address_get_host(&(peer->address), hostname, 39);
			
			return TextFormat(hostname);
		}
		const char* getIP()
		{
			char ip[39];
			enet_address_get_host(&(peer->address), ip, 39);
			
			return TextFormat(ip);
		}
		void setName(const char* newName)
		{
			name[0] = '\0'; 
			strncat(name, newName, MAXCLIENTNAME - 1);
		}
		void setConnected(bool set)
		{
			connected = set;
		}
	};

	class Host
	{
	protected:
		ENetHost* host = nullptr;
		ENetAddress* addr = NULL;
		ENetEvent event;
		Bandwidth bandwidth;
		State state = NoNET::OFFLINE;
		size_t maxPeers;
		size_t channels;
		virtual void onConnect(ENetEvent& connectEvent) = 0;
		virtual void onReceive(ENetEvent& receiveEvent) = 0;
		virtual void onDisconnect(ENetEvent& discconectEvent) = 0;
	public:
		Host(NoNET::Bandwidth maxBandwidth, size_t maxConnections=MAXCONNECTIONS, size_t maxChannels=DEFAULTCHANNELS)
			: bandwidth(maxBandwidth), maxPeers(maxConnections), channels(maxChannels) {}
		Host(enet_uint32 maxin = 0, enet_uint32 maxout = 0, size_t maxConnections=MAXCONNECTIONS, size_t maxChannels=DEFAULTCHANNELS)
			: maxPeers(maxConnections), channels(maxChannels) {bandwidth.in = maxin; bandwidth.out = maxout;}
		size_t getMaxPeers()
		{
			
			return maxPeers;
		}
		size_t getChannels()
		{
			
			return channels;
		}
		Bandwidth getBandwidth()
		{
			
			return bandwidth;
		}
		State getState()
		{
			
			return state;
		}
		ENetEvent getEvent()
		{
			
			return event;
		}
		ENetAddress* getAddress()
		{
			
			return addr;
		}
		const char* getIP()
		{
			size_t strLength = 17;
			char ip[strLength];
			if ( addr )
			{
				if ( enet_address_get_host_ip(addr, ip, strLength) == 0 )
				{
				
					return TextFormat(ip);
				}
				else
				{
				
					return "";
				}
			}
		}
		const char* getHostname()
		{
			size_t strLength = 17;
			char ip[strLength];
			if ( addr )
			{
				if ( enet_address_get_host(addr, ip, strLength) == 0 )
				{
				
					return TextFormat(ip);
				}
				else
				{
				
					return "";
				}
			}
		}
		ENetEvent poll(enet_uint32 sleep)
		{
			if ( enet_host_service(host, &event, sleep) > 0 )
			{
//				std::cout << "received event" << std::endl;
				switch(event.type)
				{
					case ENET_EVENT_TYPE_CONNECT:
					{
						onConnect(event);
						
						break;
					}
			
					case ENET_EVENT_TYPE_RECEIVE:
					{	
						onReceive(event);
						enet_packet_destroy(event.packet);
						
						break;
					}
			
					case ENET_EVENT_TYPE_DISCONNECT:
					{	
						onDisconnect(event);
						event.peer -> data = NULL;
						
						break;
					}
				}
			}
			
			return event;
		}
		friend int startup(const ENetAddress* addr, NoNET::Host& host, bool force);
		friend void shutdown(NoNET::Host& host, bool force);
		
		virtual ~Host()
		{
			if ( state == NoNET::ONLINE )
			{
				enet_host_destroy(host);
			}
		}
	};
	
	//	wrapper for enet_host_create
	inline int startup(const ENetAddress* addr, NoNET::Host& host, bool force=false)
	{
		if ( host.state == NoNET::OFFLINE || force )
		{
			host.host = enet_host_create(addr, host.maxPeers, host.channels, host.bandwidth.in, host.bandwidth.out);
			if ( host.host == nullptr || host.host == NULL )
			{
				std::cerr << "An error occurred while trying to create an ENet client host." << std::endl;
				
				return 1;
			}
			else
			{
				std::cout << "host initialized properly" << std::endl;
				host.state = NoNET::ONLINE;
				
				return 0;
			}
		}
		
		return 0;
	}
	
	// wrapper for enet_host_destroy
	inline void shutdown(NoNET::Host& host, bool force=false)
	{
		if ( host.state == NoNET::ONLINE || force )
		{
			enet_host_destroy(host.host);
			host.state = NoNET::OFFLINE;
		}
	}
	
	inline MsgInfo parseData(enet_uint8* data, size_t dataLength, const char* delimiter=";")
	{
		MsgInfo info;
		info.opcode = data[0];
		char msg[dataLength];
		memcpy(msg, data, dataLength);
		sscanf(msg + 1, "%zu", &info.id);
		int delimPos = TextFindIndex(msg, ";");
		char contents[dataLength - delimPos - strlen(delimiter)];
		sscanf(msg + delimPos + strlen(delimiter), "%[^\n]", &contents);
		info.contents = TextFormat("%s", contents);
		
		return info;
	}
	
	class Client : public Host
	{
	private:
//		size_t id;
		char name[MAXCLIENTNAME];
		ENetPeer* peer = nullptr;
	public:
		Client(const char* clientname, NoNET::Bandwidth max, size_t maxConnections=MAXCONNECTIONS, size_t maxChannels=DEFAULTCHANNELS)
			: Host(max, maxConnections, maxChannels) {name[0] = '\0'; strncat(name, clientname, MAXCLIENTNAME - 1);}
		Client(const char* clientname, enet_uint32 maxin = 0, enet_uint32 maxout = 0, size_t maxConnections=MAXCONNECTIONS, size_t maxChannels=DEFAULTCHANNELS)
			: Host(maxin, maxout, maxConnections, maxChannels) {name[0] = '\0'; strncat(name, clientname, MAXCLIENTNAME - 1);}
		const char* getName()
		{
			
			return name;
		}
		
		int connect(ENetAddress address, enet_uint32 timeout=5000)
		{
			char IPBuffer[17];
			enet_address_get_host(&address, IPBuffer, 17);
			std::cout << "attempting connection to: " << IPBuffer << " on port: " << address.port << std::endl;
			peer = enet_host_connect(host, &address, channels, 0); // TODO: replace 0 with client name
			if ( peer == NULL || peer == nullptr )
			{
				
				return 1;
			}
			else
			{
				ENetEvent event = poll(timeout);
				if ( event.type == ENET_EVENT_TYPE_CONNECT )
				{
					
					return 0;
				}
				else
				{
					enet_peer_reset(peer);
					
					return 1;
				}
				
				// ENetEvent connectEvent;
				// if ( enet_host_service(host, &connectEvent, timeout) > 0 && event.type == ENET_EVENT_TYPE_CONNECT)
				// {
					
					// return 0;
				// }
				// else
				// {
					// enet_peer_reset(peer);
					
					// return 1;
				// }
			}
		}
		int connect(const char* ip, enet_uint16 port, enet_uint32 timeout=5000)
		{
			ENetAddress address;
			enet_address_set_host(&address, ip);
			address.port = port;
			
			return connect(address, timeout);
		}
		
		int send(enet_uint8 channel, ENetPacket* payload, bool force=false)
		{
			int res = 1;
			if ( peer )
			{
				res = enet_peer_send(peer, channel, payload);
				if ( force )
				{
					enet_host_flush(host);
				}
			}
			
			return res;
		}
		int send(enet_uint8 channel, const char* payload, enet_uint32 flags=0, bool force=false)
		{
			ENetPacket* packet = enet_packet_create(payload, strlen(payload) + 1, flags);
			
			return send(channel, packet, force);
		}
		
		int disconnect(enet_uint32 timeout=3000, bool force=false)
		{
			if ( force )
			{
				enet_peer_reset(peer);
				
				return 0;
			}
			else if ( peer )
			{
				enet_peer_disconnect(peer, 0);
				ENetEvent event = poll(timeout);
				if ( event.type == ENET_EVENT_TYPE_DISCONNECT )
				{
					
					return 0;
				}
			}
			
			return 1;
		}
	};
	
	class ClientManager
	{
	private:
		std::vector< std::shared_ptr< ClientInfo > > clients;
		std::vector< std::shared_ptr< ClientInfo > > toAdd;
		size_t total = 0;
	public:
		ClientManager() {}
		ClientManager(std::vector< std::shared_ptr< ClientInfo > > list)
			: clients(list) {}
		std::shared_ptr< ClientInfo > addClient(const char* name, ENetPeer* peer)
		{
			std::shared_ptr< ClientInfo > client = std::shared_ptr< ClientInfo >(new ClientInfo(total++, name, peer));
			toAdd.push_back(client);
			
			return client;
		}
		size_t getTotal()
		{
			
			return total;
		}
		void clear()
		{
			clients.clear();
			total = 0;
		}
		void update()
		{
			std::vector< std::shared_ptr< ClientInfo > > newVec;
			if ( !clients.empty() )
			{
				for(size_t i=0; i < clients.size(); i++)
				{
					if ( clients.at(i)->getConnected() )
					{
						newVec.push_back(clients[i]);
					}
				}
			}
			if ( !toAdd.empty() )
			{
				for(size_t i=0; i < toAdd.size(); i++)
				{
					if ( toAdd.at(i)->getConnected() )
					{
						newVec.push_back(toAdd.at(i));
					}
				}
				toAdd.clear();
			}
			clients = newVec;
		}
		std::vector< std::shared_ptr< ClientInfo > > getClients()
		{
			
			return clients;
		}
		std::shared_ptr< ClientInfo > getClient(size_t id)
		{
			for (std::shared_ptr< ClientInfo > client : clients)
			{
				if ( client->getId() == id )
				{
					
					return client;
				}
			}
			
			return nullptr;
		}
		std::shared_ptr< ClientInfo > getClient(ENetPeer* peer)
		{
			for (std::shared_ptr< ClientInfo > client : clients)
			{
				if ( client->getPeer() == peer )
				{
					
					return client;
				}
			}
			
			return nullptr;
		}
		std::shared_ptr< ClientInfo > getClient(enet_uint32 host)
		{
			for (std::shared_ptr< ClientInfo > client : clients)
			{
				if ( client->getPeer()->address.host == host )
				{
					
					return client;
				}
			}
			
			return nullptr;
		}
		
		void removeClient(size_t id)
		{
			for (std::shared_ptr< ClientInfo > client : clients)
			{
				if ( client->getId() == id )
				{
					
					client->setConnected(false);
				}
			}
		}
		void removeClient(std::shared_ptr< ClientInfo > client)
		{
			for (size_t i=0; i < clients.size(); i++)
			{
				if ( clients[i] == client )
				{
					
					client->setConnected(false);
				}
			}
		}
		void removeClient(ENetPeer* peer)
		{
			for (std::shared_ptr< ClientInfo > client : clients)
			{
				if ( client->getPeer() == peer )
				{
					
					client->setConnected(false);
				}
			}
		}
		void removeClient(enet_uint32 host)
		{
			for (std::shared_ptr< ClientInfo > client : clients)
			{
				if ( client->getPeer()->address.host == host )
				{
					
					client->setConnected(false);
				}
			}
		}
	};
	
	class Server : public NoNET::Host
	{
	private:
		
	public:
		Server(NoNET::Bandwidth maxBandwidth, size_t maxConnections=MAXCONNECTIONS, size_t maxChannels=DEFAULTCHANNELS)
			: Host(maxBandwidth, maxConnections, maxChannels) 
		{
			addr = (ENetAddress*) malloc(sizeof(ENetAddress));
			addr->host = DEFAULTADDRESS.host;
			addr->port = DEFAULTADDRESS.port;
		}
		Server(enet_uint32 maxin = 0, enet_uint32 maxout = 0, size_t maxConnections=MAXCONNECTIONS, size_t maxChannels=DEFAULTCHANNELS)
			: Host(maxin, maxout, maxConnections, maxChannels) 
		{
			addr = (ENetAddress*) malloc(sizeof(ENetAddress));
			addr->host = DEFAULTADDRESS.host;
			addr->port = DEFAULTADDRESS.port;
		}
		
		virtual ~Server()
		{
			free(addr);
		}
		
		ENetPeer* getPeers()
		{
			
			return host->peers;
		}
		
		ENetPeer getPeer(size_t index)
		{
			
			return host->peers[index];
		}
		
		size_t getConnectedPeers()
		{
			
			return host->connectedPeers;
		}
		
		int send(ENetPeer* peer, enet_uint8 channel, ENetPacket* payload, bool force=false)
		{
			int res = enet_peer_send(peer, channel, payload);
			if ( force )
			{
				enet_host_flush(host);
			}
			
			return res;
		}
		int send(ENetPeer* peer, enet_uint8 channel, const char* payload, enet_uint8 flags=0, bool force=false)
		{
			ENetPacket* packet = enet_packet_create(payload, strlen(payload) + 1, flags);
			
			return send(peer, channel, packet, force);
		}
		
		void broadcast(enet_uint8 channel, ENetPacket* payload)
		{
			enet_host_broadcast(host, channel, payload);
		}
		void broadcast(enet_uint8 channel, const char* payload, enet_uint8 flags=0)
		{
			ENetPacket* packet = enet_packet_create(payload, strlen(payload) + 1, flags);
			broadcast(channel, packet);
		}
		
		int disconnect(ENetPeer* peer, enet_uint32 timeout=3000, bool force=false)
		{
			if ( force )
			{
				enet_peer_reset(peer);
				
				return 0;
			}
			enet_peer_disconnect(peer, 0);
			ENetEvent event = poll(timeout);
			if ( event.type == ENET_EVENT_TYPE_DISCONNECT )
			{
					
				return 0;
			}
			
			return 1;
		}
	};
	
	class ManagedClient : public Client
	{
	private:
		size_t id;
	public:
		ManagedClient(const char* clientname, NoNET::Bandwidth maxBandwidth, size_t maxConnections=MAXCONNECTIONS, size_t maxChannels=DEFAULTCHANNELS)
			: Client(clientname, maxBandwidth, maxConnections, maxChannels) {}
		ManagedClient(const char* clientname, enet_uint32 maxin = 0, enet_uint32 maxout = 0, size_t maxConnections=MAXCONNECTIONS, size_t maxChannels=DEFAULTCHANNELS)
			: Client(clientname, maxin, maxout, maxConnections, maxChannels) {}
		size_t getId()
		{
			
			return id;
		}
		void setId(size_t set)
		{
			id = set;
		}
		virtual void onConnect(ENetEvent& connectEvent)
		{
			
		}
		virtual void onDisconnect(ENetEvent& disconnectEvent)
		{
			
		}
		virtual void onReceive(ENetEvent& receiveEvent)
		{
			std::cout << "client received message: " << receiveEvent.packet->data << std::endl;
			std::cout << "message type: " << receiveEvent.packet->data[0] << std::endl;
			std::cout << "MSG_JOIN = " << MSG_JOIN << std::endl;
			if ( receiveEvent.packet->data[0] == MSG_JOIN )
			{
				std::cout << "message is join request" << std::endl;
				char idStr[receiveEvent.packet->dataLength];
				memcpy(idStr, receiveEvent.packet->data, receiveEvent.packet->dataLength);
				size_t idVal;
				sscanf(idStr + 1, "%zu", &idVal);
				std::cout << "client id: " << idVal << std::endl;
				setId(idVal);
				send(0, TextFormat("%c%zu;%s", MSG_JOIN, getId(), getName()));
			}
		}
	};
	
	class ManagedServer : public Server, public ClientManager
	{
	public:
		ManagedServer(NoNET::Bandwidth maxBandwidth, size_t maxConnections=MAXCONNECTIONS, size_t maxChannels=DEFAULTCHANNELS)
			: Server(maxBandwidth, maxConnections, maxChannels) {}
		ManagedServer(enet_uint32 maxin = 0, enet_uint32 maxout = 0, size_t maxConnections=MAXCONNECTIONS, size_t maxChannels=DEFAULTCHANNELS)
			: Server(maxin, maxout, maxConnections, maxChannels) {}
		virtual void onConnect(ENetEvent& connectEvent)
		{
//			std::cout << "received connect request" << std::endl;
			send(connectEvent.peer, 0, TextFormat("%c%zu", MSG_JOIN, getTotal()));
			addClient("", connectEvent.peer);
//			update();
//			std::cout << "current clients connected: " << getClients().size() << std::endl;
		}
		virtual void onDisconnect(ENetEvent& disconnectEvent)
		{
//			std::cout << "received disconnect request" << std::endl;
			removeClient(disconnectEvent.peer);
//			update();
//			std::cout << "current clients connected: " << getClients().size() << std::endl;
		}
		virtual void onReceive(ENetEvent& receiveEvent)
		{
			MsgInfo msg = parseData(receiveEvent.packet->data, receiveEvent.packet->dataLength); // all data sent to the server must follow this format
			std::cout << "message received with code " << msg.opcode << " from client " << msg.id << " with contents: " << msg.contents << std::endl;
			if ( msg.opcode == MSG_JOIN )
			{
				std::shared_ptr< ClientInfo > client = getClient(msg.id);
				if ( client )
				{
					client->setName(msg.contents);
				}
			}
			else if ( msg.opcode == MSG_BROADCAST )
			{
//				char msg[receiveEvent.packet->dataLength];
//				memcpy(msg, receiveEvent.packet->data, receiveEvent.packet->dataLength);
//				size_t idVal;
//				sscanf(msg + 1, "%zu", &idVal);
//				int delimPos = TextFindIndex(msg, ";");
//				char contents[receiveEvent.packet->dataLength - delimPos - 1];
//				sscanf(msg + delimPos + 1, "%[^\n]", &contents);
				broadcast(0, TextFormat("%c%s;%s", msg.opcode, getClient(msg.id)->getName(), msg.contents));
			}
		}
	};
}

#endif