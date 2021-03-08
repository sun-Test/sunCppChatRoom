#include <iostream>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>

#define MAX_LEN 500
#define CYAN_COLOR "\033[36m"
#define RESET_COLOR "\033[0m"
#define YELLOW_COLOR "\033[35m"
#define PORT 9888
#define QUIT_CMD ":q"

using namespace std;

struct ClientConnection
{
	int id;
	string name;
	int socket;
	thread th;
};

mutex coutMtx, clientsMtx;
vector<ClientConnection> clients;
struct sockaddr_in addr;

class ChatServer{
    public:
        int clientId = 0;
        struct sockaddr_in serverAddr;
        int socketFd;
        unique_ptr<vector<ClientConnection>> clientsPtr;
        void setupAddr();
        void setupSocketListener();
        void broadcastMessage(string message, int senderId);
        void updateClientName(int id, char name[]);
        void printSync(string str);
        bool ifDisconnect(int receivedBytes, int id, const string& clientName, const string& msg);
        void endClientConnection(int id);
        ChatServer(struct sockaddr_in& serverAddr, vector<ClientConnection>* clients)
        {
            serverAddr = serverAddr;
            clientsPtr.reset(clients);
        }
        ~ChatServer(){}
};

void ChatServer::setupAddr()
{
    this->serverAddr.sin_family=AF_INET;
	this->serverAddr.sin_port=htons(PORT);
	this->serverAddr.sin_addr.s_addr=INADDR_ANY;
	::bzero(&this->serverAddr.sin_zero, 0);
}

void ChatServer::setupSocketListener()
{
    if((socketFd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        perror("socket: ");
        exit(-1);
    }
    if(::bind(socketFd,(struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in)) < 0)
    {
        perror("bind error: ");
        exit(-1);
    }
    if((listen(socketFd, 8)) < 0)
    {
        perror("listen error: ");
        exit(-1);
    }
}

void ChatServer::updateClientName(int id, char name[])
{
	for(int i=0; i<clientsPtr->size(); i++)
	{
        if(clientsPtr->at(i).id==id)	
        {
            clientsPtr->at(i).name=string(name);
        }
	}	
}

// Broadcast message to all clients except the sender
void ChatServer::broadcastMessage(string message, int senderId)
{
	char temp[MAX_LEN];
	strcpy(temp,message.c_str());
	for(int i=0; i< clientsPtr->size(); i++)
	{
		if(clientsPtr->at(i).id!=senderId)
		{
			send(clientsPtr->at(i).socket,temp,sizeof(temp),0);
		}
	}		
}

void ChatServer::endClientConnection(int id)
{
	for(int i=0; i<clientsPtr->size(); i++)
	{
		if(clientsPtr->at(i).id==id)	
		{
			lock_guard<mutex> guard(clientsMtx);
            if(clientsPtr->at(i).th.joinable())
                clientsPtr->at(i).th.detach();
			close(clientsPtr->at(i).socket);
			clientsPtr->erase(clientsPtr->begin()+i);
			break;
		}
	}				
}

bool ChatServer::ifDisconnect(int receivedBytes, int id, const string& clientName, const string& msg)
{
        if(receivedBytes <= 0)
			return true;
		if(msg.compare(QUIT_CMD) == 0)
		{
			string message= clientName + " has left";	
			broadcastMessage(message,id);
			printSync("\033[33m" +message + CYAN_COLOR + "\n");
			endClientConnection(id);							
			return true;
		}
        return false;
}

void ChatServer::printSync(string str)
{	
	lock_guard<mutex> guard(coutMtx);
	cout<<str;
}

ChatServer chatServer(addr, &clients);
void handleConnectionRequest(int clientSocket, int id);
void handleConnectionRequest(int clientSocket, int id)
{
	char name[MAX_LEN],msg[MAX_LEN];
	read(clientSocket,name,sizeof(name));
	chatServer.updateClientName(id, name);	
	string welcomeMessage=string(name)+string(" has joined");
	chatServer.broadcastMessage(welcomeMessage, id);	
	chatServer.printSync(CYAN_COLOR + welcomeMessage + RESET_COLOR + "\n");
	while(1)
	{
		int bytesReceived=recv(clientSocket,msg,sizeof(msg),0);
        if(chatServer.ifDisconnect(bytesReceived, id, string(name), string(msg)))
            return;
        chatServer.broadcastMessage( string(name) + " : " + string(msg), id);
		chatServer.printSync(YELLOW_COLOR + string(name)+" : "+RESET_COLOR+msg + "\n");		
	}	
}

void startListening(ChatServer& server, vector<ClientConnection>& clientList)
{
    struct sockaddr_in client;
	int newConnectionSocket;
	unsigned int len=sizeof(sockaddr_in);
    int clientId = 0;
    while(1)
	{
		if((newConnectionSocket=accept(server.socketFd,(struct sockaddr *)&client,&len)) < 0)
		{
			perror("accept error: ");
			exit(-1);
		}
		clientId++;
		thread t(handleConnectionRequest, newConnectionSocket, clientId);
		lock_guard<mutex> guard(clientsMtx);
		clientList.push_back({clientId, string("Anonymous"),newConnectionSocket,(move(t))});
	}
}

void stopListening(ChatServer& server, vector<ClientConnection>& clientList)
{
    for(int i=0; i<clientList.size(); i++)
	{
		if(clientList[i].th.joinable())
			clientList[i].th.join();
	}
	close(server.socketFd);
}

int main()
{
    cout<<"app starting..."<<endl;
    chatServer.setupAddr();
    chatServer.setupSocketListener();
	cout<<"\033[36m"<<"\n\t ***** welcome to the Sunny World *****  "<<endl<<RESET_COLOR;
    startListening(chatServer, clients);
    stopListening(chatServer, clients);
	return 0;
}
