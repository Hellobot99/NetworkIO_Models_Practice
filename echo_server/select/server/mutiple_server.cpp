#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    fd_set read_fds;
    FD_ZERO(&read_fds);

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

    std::vector<int> client_fds;
    int fd_max = server_fd, fd_num;

    while(1) {
        //fd_set 초기화 및 소켓 추가
        fd_set temp_fds;
        FD_ZERO(&temp_fds);
        FD_SET(server_fd, &temp_fds);
        for (int client_fd : client_fds) {
            FD_SET(client_fd, &temp_fds);
        }

        // select 호출
        if((fd_num = select(fd_max + 1, &temp_fds, NULL, NULL, NULL)) < 0){
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        // 새로운 클라이언트 접속 확인
        if (FD_ISSET(server_fd, &temp_fds)) {
            int temp_client = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (temp_client < 0) {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }

            client_fds.push_back(temp_client);

            if (temp_client > fd_max) {
                fd_max = temp_client;
            }

            std::cout << client_fds.size() << " Client connected!" << std::endl;
        }

        // 기존 클라이언트들 데이터 읽기 및 에코 처리
        for (auto it = client_fds.begin(); it != client_fds.end(); ) {
            if (FD_ISSET(*it, &temp_fds)) {
                int n = read(*it, buffer, sizeof(buffer));
                if (n <= 0) {
                    close(*it);
                    it = client_fds.erase(it);
                    std::cout << "Client left, "<< client_fds.size() << " Client left" << std::endl;
                } else {
                    send(*it, buffer, n, 0);
                    ++it;
                }
            } else {
                ++it;
            }
        }
    }

    std::cout << "Server shutting down..." << std::endl;

    // 7. 소켓 닫기
    for (int client_fd : client_fds) {
        close(client_fd);
    }
    close(server_fd);

    return 0;
}
