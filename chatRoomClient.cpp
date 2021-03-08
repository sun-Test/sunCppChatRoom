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
#include <signal.h>
#include <mutex>
#define PORT 9888 
 
#define MAX_LEN 500
#define RESET_COLOR "\033[0m"
#define YELLOW_COLOR "\033[33m"
#define CYAN_COLOR "\033[36m"
#define QUIT_CMD ":q"

using namespace std;

bool disconnect = false;
thread thSend, thRecv;
int clientSocket;
char recvBuffer[MAX_LEN] = {0};

void catchCtrlC(int signal);
void removeInputPref(int cnt);
void sendWorker(int clientSocket);
void recvWorker(int clientSocket);
void setupUserName();
int createConnection(sockaddr_in& serverAddr);

void setupAddr(sockaddr_in &sockAddr)
{
    sockAddr.sin_family=AF_INET;
	sockAddr.sin_port=htons(PORT);
	sockAddr.sin_addr.s_addr=INADDR_ANY;
	::bzero(&sockAddr.sin_zero, 0);
}

// SOCK_STREAM: tcp protocal, reliable, AF_INET: IPv4
int createConnection(sockaddr_in& serverAddr)
{
    int socketFd;
	if((socketFd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket: ");
		exit(-1);
	}
	if((connect(socketFd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in))) < 0)
	{
		perror("connect: ");
		exit(-1);
	}
    return socketFd;
}

int main()
{
	struct sockaddr_in serverAddr;
    setupAddr(serverAddr);
	signal(SIGINT, catchCtrlC);
    while(true)
    {
        disconnect = false;
        clientSocket = createConnection(serverAddr);
        setupUserName();
        thread t1(sendWorker, clientSocket);
        thread t2(recvWorker, clientSocket);
        thSend = move(t1);
        thRecv = move(t2);
        if(thSend.joinable())
            thSend.join();
        if(thRecv.joinable())
            thRecv.join();

    }
	return 0;
}

void setupUserName()
{
    char name[MAX_LEN];
	cout<<"Enter your name : ";
	cin.getline(name, MAX_LEN);
	send(clientSocket, name, sizeof(name), 0);
	cout<<CYAN_COLOR<<"\n\t  ***** Welcome to Sunny room *****  "<<endl<<RESET_COLOR;
}

void catchCtrlC(int signal) 
{
	char msg[MAX_LEN] = QUIT_CMD;
	send(clientSocket, msg, sizeof(msg), 0);
	disconnect = true;
	thSend.detach();
	thRecv.detach();
	close(clientSocket);
	exit(signal);
}

// remove you:  from terminal
void removeInputPref(int cnt)
{
    string backSpace = "\b";
	for(int i = 0; i<cnt; i++)
	{
		cout<<backSpace;
	}	
}

void sendWorker(int clientSocket)
{
	while(true)
	{
		cout<<YELLOW_COLOR<<"You : "<<RESET_COLOR;
		char msg[MAX_LEN];
		cin.getline(msg, MAX_LEN);
		send(clientSocket, msg, sizeof(msg), 0);
		if(strcmp(msg, QUIT_CMD)==0)
		{
			disconnect = true; //thRecv.detach();	
			close(clientSocket);
			return;
		}	
	}		
}

void recvWorker(int clientSocket)
{
	while(true)
	{
		if(disconnect)
			return;
		char msg[MAX_LEN];
		int bytesReceived = read(clientSocket, msg, sizeof(msg));
		if(bytesReceived<=0)
			continue;
		removeInputPref(6);
        cout<<msg<<endl;
		cout<<YELLOW_COLOR<<"You : "<<RESET_COLOR;
		fflush(stdout);
	}	
}