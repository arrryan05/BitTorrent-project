#include<iostream> 
// Next two headers for socket function
#include<vector>
#include <sys/types.h>
#include <sys/socket.h>
// For memset
#include<string.h>
// For getnameinfo  --> line 3 and below line
#include<netdb.h>
#include<unistd.h>
#include <sstream>
#include <arpa/inet.h>
#include<string>

using namespace std;

vector<string> get_words(char str[], char a) {
    vector<string> res;
    string temp;
    stringstream ss(str);
    while(getline(ss, temp, a)) {
        res.push_back(temp);
    }
    return res;
}

string Hello(string name) {
    string t = "Hello, nice to connect you ";
    return t+name;
}

int main(){
    // Create a socket
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if(listening == -1){
        cerr << "Can't create a socket!";
        return -1;
    }
    /*#include <netinet/in.h>

    struct sockaddr_in {
        short            sin_family;   // e.g. AF_INET
        unsigned short   sin_port;     // e.g. htons(3490)
        struct in_addr   sin_addr;     // see struct in_addr, below
        char             sin_zero[8];  // zero this if you want to
    };

    struct in_addr { 
        unsigned long s_addr;  // load with inet_aton()
    };
    Note ---> All of the data in the SOCKADDR_IN structure, except for the address family, 
            must be specified in network-byte-order (big-endian).
            But most of the modern CPUs are little endian.
            Endians is the just the way how no is stored if it is greater than 255 that is
            takes more than one byte, so to convert it from what this machine understands
            to what the network understands we use htons which stand for 
            host to network short.
            There is also ntohs which does the revers of that*/

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    // host to network short. Taking a random port for now, if it does not work we will try
    // 54001 and so on.
    hint.sin_port = htons(54000);

    // String to numeric array
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr); // 0.0.0.0 is any address

    if(bind(listening, (sockaddr*)&hint, sizeof(hint))== -1){
        cerr << "Can't bind to IP/port!";
        return -2;
    }
    cout << "Server socket created\r\n";
    if(listen(listening, SOMAXCONN)==-1){
        cerr << "Can't listen!";
        return -3;  
    }
    cout<<"Now waiting for a client to connect\r\n";
    sockaddr_in client;
    socklen_t clientsize = sizeof(client);
    // Buffers
    char host[NI_MAXHOST];
    char serv[NI_MAXHOST];

    int clientsocket = accept(listening, (sockaddr*)&client, &clientsize);

    if(clientsocket == -1){
        cerr << "Problem with client connecting!";
        return -4;
    }

    close(listening); 
    
    memset(host, 0, NI_MAXHOST);
    memset(serv, 0, NI_MAXSERV);

    int result = getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, serv, NI_MAXSERV, 0);

    if(result){
        cout << host << " connected on " << serv; 
    }
    else
    {
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        // Numeric array to string
        cout << host << " connected on " <<  ntohs(client.sin_port) << endl;
    }
    
    // While recieving the message echo the message.
    char buf[4096];
    while(true){
        // Clear the buffer
        memset(buf, 0, 4096);
        // wait for a message
        
        int bytesRecv = recv(clientsocket, buf, 4096, 0);
        if(bytesRecv == -1){
            cerr << "There was a connection issue" << endl;
            break;
        }
        if(bytesRecv == 0){
            cout << "The client disconnected";
            break;
        }
        // Display message
        cout << "Recieved : " << string(buf, 0, bytesRecv) << endl;
       
        
        vector<string> data = get_words(buf, ' ');
        string message_to_send = Hello(data[data.size()-1]);
        
        // Now we want to echo that back out again.
        // Resend message for that
        send(clientsocket, message_to_send.c_str(), message_to_send.size(), 0); 

    }
    close(clientsocket);
    // Close socket

}
