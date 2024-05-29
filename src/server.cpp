#include <cstring> 
#include <iostream> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h> 

std::string data_buffer = "";

int main() {

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5959);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.0.112"); //INADDR_ANY;

    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    listen(serverSocket, 5);

    int clientSocket = accept(serverSocket, nullptr, nullptr);

    char buffer[1024] = { 0 };
    recv(clientSocket, buffer, sizeof(buffer), 0);

    std::string data = buffer;

    std::cout << buffer;

    if(data.rfind("set:", 0) == 0) {
        data_buffer = data;

        close(clientSocket);
        main();
    }

    if(data.rfind("get?", 0) == 0) {
        send(clientSocket, data_buffer.c_str(), sizeof(data_buffer.c_str()), 0);
        
        close(clientSocket);
        main();
    }
    


}