#include<iostream> 
// Next two headers for socket function
#include <sys/types.h>
#include <sys/socket.h>
// For memset
#include<string.h>
// For getnameinfo  --> line 3 and below line
#include<netdb.h>
#include<unistd.h>
#include <arpa/inet.h>
#include<string>
using namespace std;
int main(){
    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        cerr << "Can't create a socket!";
        return 1;
    }
    // Create a hint structure for the server we're connecting with.
    int port = 54000;
    string ipaddress = "127.0.0.1";
    // string ipaddress = "192.168.0.102";

    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(port);
    inet_pton(AF_INET, ipaddress.c_str(), &hint.sin_addr);
    // Connect to the server on the socket
    int connectres = connect(sock, (sockaddr*)&hint, sizeof(hint));
    if(connectres == -1){
        return 1; 
    }

    char buf[4096];
    string Userinput;

    do{
        cout<< ">";
        getline(cin, Userinput);

        int sendres = send(sock, Userinput.c_str(), Userinput.size() + 1, 0);
        if(sendres == -1){
            cout << "could not send to server! whoops\r\n";
            continue;
        }
        
        memset(buf, 0, 4096);
        int bytesrecv = recv(sock, buf, 4096, 0);

        if(bytesrecv == -1){
            cout << "There was a connection issue\r\n" << endl;
           
        }
        else{
            cout<< "From the server " << string(buf, bytesrecv) << "\r\n";
        }
    }while(true);

    close(sock);
    return 0;
}
