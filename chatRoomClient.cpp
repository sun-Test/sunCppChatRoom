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

thread *thSend, *thRecv;

void catchCtrlC(int signal);
void removeInputPref(int cnt);

class RoomClient
{

public:
	bool disconnect = false;
	int clientSocket;
	struct sockaddr_in serverAddr;

	void setupAddr()
	{
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(PORT);
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		::bzero(&serverAddr.sin_zero, 0);
	}

	void setupUserName()
	{
		char name[MAX_LEN];
		cout << "Enter your name : ";
		cin.getline(name, MAX_LEN);
		send(clientSocket, name, sizeof(name), 0);
		cout << CYAN_COLOR << "\n\t  ***** Welcome to Sunny room *****  " << endl
		     << RESET_COLOR;
	}
	// SOCK_STREAM: tcp protocal, reliable, AF_INET: IPv4
	void createConnection()
	{
		if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		{
			perror("socket: ");
			exit(-1);
		}
		if ((connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr_in))) < 0)
		{
			perror("connect: ");
			exit(-1);
		}
	}

	void sendWorker()
	{
		while (true)
		{
			cout << YELLOW_COLOR << "You : " << RESET_COLOR;
			char msg[MAX_LEN];
			cin.getline(msg, MAX_LEN);
			send(clientSocket, msg, sizeof(msg), 0);
			if (strcmp(msg, QUIT_CMD) == 0)
			{
				disconnect = true; //thRecv.detach();
				close(clientSocket);
				return;
			}
		}
	}

	void recvWorker()
	{
		while (true)
		{
			if (disconnect)
				return;
			char msg[MAX_LEN];
			int bytesReceived = read(clientSocket, msg, sizeof(msg));
			if (bytesReceived <= 0)
				continue;
			removeInputPref(6);
			cout << msg << endl;
			cout << YELLOW_COLOR << "You : " << RESET_COLOR;
			fflush(stdout);
		}
	}
};

RoomClient client;

void catchCtrlC(int signal)
{
	char msg[MAX_LEN] = QUIT_CMD;
	send(client.clientSocket, msg, sizeof(msg), 0);
	client.disconnect = true;
	thSend->detach();
	thRecv->detach();
	close(client.clientSocket);
	exit(signal);
}

int main()
{
	client.setupAddr();
	signal(SIGINT, catchCtrlC);
	while (true)
	{
		client.createConnection();
		client.setupUserName();
		thread t1(&RoomClient::sendWorker, ref(client));
		thread t2(&RoomClient::recvWorker, ref(client));
		thSend = &t1;
		thRecv = &t2;
		if (t1.joinable())
			t1.join();
		if (t2.joinable())
			t2.join();
	}
	return 0;
}
// remove you:  from terminal
void removeInputPref(int cnt)
{
	string backSpace = "\b";
	for (int i = 0; i < cnt; i++)
	{
		cout << backSpace;
	}
}