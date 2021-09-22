#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#define PORT 4044
#define LENGTH 2048
using namespace std;
volatile sig_atomic_t flag = 0;
int listenfd = 0;
char name[32];
ofstream clientFile("clientchatlog.txt", std::ios::app);

void str_trim_lf(char *arr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{ // trim \n
		if (arr[i] == '\n')
		{
			arr[i] = '\0';
			break;
		}
	}
}

void catch_ctrl_c_and_exit(int sig)
{
	flag = 1;
}

void *send_msg_handler(void *arg)
{
	char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};

	while (1)
	{
		fgets(message, LENGTH, stdin);
		str_trim_lf(message, LENGTH);

		clientFile << message << endl;
		if (strcmp(message, "exit") == 0)
		{
			break;
		}
		else
		{
			sprintf(buffer, "%s: %s\n", name, message);
			send(listenfd, buffer, strlen(buffer), 0);
		}

		bzero(message, LENGTH);
		bzero(buffer, LENGTH + 32);
	}
	catch_ctrl_c_and_exit(2);
	pthread_exit(NULL);
}

void *recv_msg_handler(void *arg)
{
	char message[LENGTH] = {};
	while (1)
	{
		int receive = recv(listenfd, message, LENGTH, 0);
		clientFile << message << endl;
		if (receive > 0)
		{
			printf("%s \n >", message);
		}
		else if (receive == 0)
		{
			break;
		}
		else
		{
			// -1
		}
		memset(message, 0, sizeof(message));
	}
	pthread_exit(NULL);
}

int main()
{
	signal(SIGINT, catch_ctrl_c_and_exit);

	printf("Please enter details in form of username:userID:password ");
	fgets(name, 32, stdin);
	str_trim_lf(name, strlen(name));

	sockaddr_in serv_addr;
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_addr.sin_port = htons(PORT);

	int err = connect(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (err == -1)
	{
		printf("ERROR: connect\n");
		return EXIT_FAILURE;
	}

	send(listenfd, name, 32, 0);

	printf(" Connected! \n Please select what do you want to do?\n register or login");
	clientFile << " Connected! \n Please select what do you want to do?\n register or login";

	pthread_t send_msg_thread;
	if (pthread_create(&send_msg_thread, NULL, send_msg_handler, NULL) != 0)
	{
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
	if (pthread_create(&recv_msg_thread, NULL, recv_msg_handler, NULL) != 0)
	{
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	while (1)
	{
		if (flag)
		{
			printf("\nServer Disconnected, shawty\n");
			break;
		}
	}
	clientFile.close();
	close(listenfd);

	return EXIT_SUCCESS;
}
