#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int main(int argc , char *argv[]) {

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }
    int port = std::stoi(argv[2]);

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    std::string input;

    // 1. 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cout << "\n Socket creation error \n";
        return -1;
    }

    // 2. 서버 주소 설정
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

    // 4. 서버에 연결
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "\nConnection Failed \n";
        return -1;
    }

    std::cout << "Connected to server!" << std::endl;

    // 5. 메시지 보내고 받기
    while (true) {
        std::cout << "You: ";
        std::getline(std::cin, input);

        if (input == "exit") break;

        send(sock, input.c_str(), input.length(), 0);

        int valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            std::cout << "Echo: " << buffer << std::endl;
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

    // 6. 소켓 닫기
    close(sock);

    return 0;
}
