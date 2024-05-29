#include <cstring> 
#include <iostream> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <unistd.h>

std::string data_buffer = "";

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error creating socket\n";
        return -1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5959);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.0.112");

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Error binding socket\n";
        close(serverSocket);
        return -1;
    }

    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error listening on socket\n";
        close(serverSocket);
        return -1;
    }

    std::cout << "Server is listening on 192.168.0.112:5959\n";

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket < 0) {
            std::cerr << "Error accepting connection\n";
            continue;
        }

        char buffer[1024] = { 0 };
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived < 0) {
            std::cerr << "Error receiving data\n";
            close(clientSocket);
            continue;
        }

        std::string data(buffer, bytesReceived);
        std::cout << "Received: " << data << std::endl;

        if (data.rfind("set:", 0) == 0) {
            data_buffer = data.substr(4); // store the actual data after "set:"
            std::cout << "Data stored: " << data_buffer << std::endl;
        } else if (data.rfind("get?", 0) == 0) {
            if (send(clientSocket, data_buffer.c_str(), data_buffer.size(), 0) < 0) {
                std::cerr << "Error sending data\n";
            } else {
                std::cout << "Data sent: " << data_buffer << std::endl;
            }
        }

        close(clientSocket);
    }

    close(serverSocket);
    return 0;
}
