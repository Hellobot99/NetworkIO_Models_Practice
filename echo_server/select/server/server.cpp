#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // 1. 서버 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. 주소 설정
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // 3. 바인딩
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. 리슨
    if (listen(server_fd, 1) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port << "..." << std::endl;

    // 5. 클라이언트 접속 수락
    if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Client connected!" << std::endl;

    // 6. 에코 처리
    int valread;
    while ((valread = read(client_socket, buffer, BUFFER_SIZE)) > 0) {
        std::cout << "Received: " << buffer << std::endl;
        send(client_socket, buffer, valread, 0);  // 받은 걸 그대로 보냄
        memset(buffer, 0, BUFFER_SIZE);  // 버퍼 초기화
    }

    std::cout << "Client disconnected" << std::endl;

    // 7. 소켓 닫기
    close(client_socket);
    close(server_fd);

    return 0;
}
