#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <vector>
#include <signal.h>
#include <map>
#include <string>
#define TrackerFile "tracker.txt"
#define chatlogs "chatlog.txt"
#define PORT 4044
using namespace std;
static int uid = 10;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reg_mut = PTHREAD_MUTEX_INITIALIZER;
typedef struct
{
    struct sockaddr_in address;
    int sockfd, id;
    string namee, password, uid;
} client_t;

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

void InsertInFile(string msg)
{
    pthread_mutex_lock(&clients_mutex);
    ofstream chatlogFile(chatlogs, std::ios::app);
    chatlogFile << msg << endl;
    chatlogFile.close();
    pthread_mutex_unlock(&clients_mutex);
}

vector<string> words(const char str[], char a)
{
    vector<string> res;
    stringstream ss(str);
    string temp;
    while (getline(ss, temp, a))
    {
        res.push_back(temp);
    }
    return res;
}

string trackerData(string abc)
{
    ifstream readTracker("tracker.txt");
    string str;
    while (getline(readTracker, str))
    {
        vector<string> res = words(str.c_str(), ':');
        if (abc.compare(res[2]) == 0)
        {
            return res[2] + ":" + res[3];
        }
    }
    return "Not found";
}

int Register(client_t *client)
{
    char userData[1024] = {0};
    int sizeofUserData = recv(client->sockfd, userData, 1024, 0);
    vector<string> res = words(userData, ':');
    if (res.size() != 3)
    {
        send(client->sockfd, "Invalid! Try again...", 22, 0);
        return Register(client);
    }
    if (string("Not found").compare(trackerData(res[2])) != 0)
    {
        send(client->sockfd, "uniqueID is already in use. Try with different ID", 50, 0);
        return Register(client);
    }
    ofstream tracker(TrackerFile, std::ios::app);
    tracker << string(userData) << endl;
    tracker.close();
    client->namee = res[0];
    client->password = res[1];
    client->uid = res[2];
    return 0;
}

int Login(client_t *client)
{
    char userData[1024] = {0};
    int sizeofuserData = recv(client->sockfd, userData, 1024, 0);
    vector<string> res = words(userData, ':');

    if (res.size() != 2)
    {
        send(client->sockfd, string("Invalid! Try again...").c_str(), 22, 0);
        return Login(client);
    }

    //reading tracker file  format (name:password:uid:portno)
    ifstream tracker(TrackerFile);

    stringstream ss;
    ss << tracker.rdbuf();

    string user, fulldata = ss.str();

    while (getline(ss, user, '\n'))
    {
        std::vector<std::string> userdata = words(user.c_str(), ':');
        if (userdata[2] == res[0] && userdata[1] == res[1])
        {
            tracker.close();
            client->uid = res[0];
            client->namee = userdata[0];
            client->password = userdata[1];

            // updating in tracker file
            int pos = fulldata.find(res[0]);
            pos += res[0].length() + 1;
            // fulldata.replace(pos, userdata[3].length(), std::to_string(this->portNo));
            ofstream tracker(TrackerFile);
            tracker << fulldata;
            tracker.close();

            return 0;
        }
    }

    tracker.close();
    send(client->sockfd, "Credentials do not match! Try again", 36, 0);
    return 0;
}

client_t *clients[10];

map<string, pair<string, string>> mp;

int checker(client_t *client)
{
    int clientSocket = client->sockfd;
    int clientID = client->id;
    char msg[1024];
    while (1)
    {
        memset(msg, 0, 1024);
        int bytesrecv = recv(clientSocket, msg, 1024, 0);
        if (bytesrecv <= 0)
        {
            return -1;
        }
        if (strcmp(msg, "register") == 0)
        {
            pthread_mutex_lock(&reg_mut);
            send(clientSocket, "Registration format(Name:Password:uniqueID)", 44, 0);
            int res = Register(client);
            pthread_mutex_unlock(&reg_mut);
            return res;
        }
        else if (strcmp(msg, "login") == 0)
        {
            send(clientSocket, "Please enter you credentials in a format which is similar to (uniqueID:Password) \n", 32, 0);
            int res = Login(client);
            return res;
        }
        else
        {
            send(clientSocket, "Retry...", 9, 0);
        }
    }
    return 0;
}

void send_message(char *s, int id)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < 10; ++i)
    {
        if (clients[i])
        {
            if (clients[i]->id != id)
            {
                int aa = write(clients[i]->sockfd, s, strlen(s));
                InsertInFile(s);

                if (aa < 0)
                {
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg)
{
    char msg_arr[1000];
    char name[32];
    int leave_flag = 0;

    client_t *cli = (client_t *)arg;

    int clientID = cli->id;
    int clientSocket = cli->sockfd;

    cout << "Please insert details in form of username:userID: password\n";

    if (checker(cli) == -1)
    {
        cout << "Client "
             << " disconnected\n";
        close(clientSocket);
        pthread_exit(NULL);
        return NULL;
    }

    mp[cli->uid] = {cli->namee, cli->password};
    cout << "client ready,send messages shawty \n";

    if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32 - 1)
    {
        printf("Didn't enter the name.\n");
        leave_flag = 1;
    }
    else
    {
        sprintf(msg_arr, "%s has joined\n", cli->namee);
        printf("%s", msg_arr);
        send_message(msg_arr, cli->id);
    }
    bzero(msg_arr, 1000);
    while (1)
    {
        if (leave_flag)
        {
            break;
        }

        int receive = recv(cli->sockfd, msg_arr, 1000, 0);
        if (receive > 0)
        {
            if (strlen(msg_arr) > 0)
            {
                send_message(msg_arr, cli->id);

                str_trim_lf(msg_arr, strlen(msg_arr));
                printf("%s -> %s\n", msg_arr, cli->namee);
                InsertInFile(msg_arr);
            }
        }
        else if (receive == 0 || strcmp(msg_arr, "exit") == 0)
        {
            sprintf(msg_arr, "%s has left\n", cli->namee);
            printf("%s", msg_arr);
            send_message(msg_arr, cli->id);
            leave_flag = 1;
        }
        else
        {
            printf("ERROR: -1\n");
            leave_flag = 1;
        }

        bzero(msg_arr, 1000);
    }

    mp.erase(cli->uid);
    close(cli->sockfd);
    free(cli);
    pthread_detach(pthread_self());

    return NULL;
}

int main()
{
    signal(SIGPIPE, SIG_IGN);

    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;

    pthread_t tid;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("ERROR: Socket binding failed");
        return EXIT_FAILURE;
    }

    if (listen(listenfd, 10) < 0)
    {
        perror("ERROR: Socket listening failed");
        return EXIT_FAILURE;
    }

    printf(" Server Established\n");
    InsertInFile("Server Established\n");

    while (1)
    {
        socklen_t clilen = sizeof(cli_addr);
        int connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);
        cout << "\nnew client connected\n";
        InsertInFile("\nnew client connected\n");

        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = cli_addr;
        cli->sockfd = connfd;
        cli->uid = uid++;

        pthread_create(&tid, NULL, &handle_client, (void *)cli);
        sleep(1);
    }

    return EXIT_SUCCESS;
}
