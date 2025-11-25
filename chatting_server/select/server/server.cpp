#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <termios.h>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <unordered_set>

#define BUFFER_SIZE 1024

#define JOIN 1
#define MESSAGE 2
#define EXIT 3

typedef struct{
    int cmd;
    char user_name[100];
    char buffer[1024];    
} Packet;

typedef struct{
    std::string name;
    std::unordered_set<int> clients;
} chatRoom;

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
    Packet packet;
    fd_set read_fds;
    FD_ZERO(&read_fds);

    std::unordered_map <std::string, chatRoom> chatRooms;
    std::unordered_map <std::string, std::string> userNames;

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

    int option=1;
    int optlen=sizeof(option);    
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&option,optlen);

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
                int n = read(*it, &packet, sizeof(packet));
                if (n <= 0) {
                    int client_fd = *it;
                    it = client_fds.erase(it);
                    std::string username(packet.user_name);
                    std::string roomname = userNames[username];
                    chatRooms[roomname].clients.erase(client_fd);
                    userNames.erase(username);
                    close(client_fd);

                } else {
                    if(packet.cmd == JOIN){
                        std::string roomname(packet.buffer);
                        std::string username(packet.user_name);
                    
                        userNames[username] = roomname;
                    
                        if(auto room = chatRooms.find(roomname); room != chatRooms.end()){
                            room->second.clients.insert(*it);
                        } else {
                            chatRoom newRoom;
                            newRoom.name = roomname;
                            newRoom.clients.insert(*it);
                            chatRooms[roomname] = newRoom;
                        }
                    
                        ++it;
                    }
                    
                    else if(packet.cmd == MESSAGE){
                        std::string username(packet.user_name);
                        std::string roomname = userNames[username];
                        std::unordered_set<int> client_fds = chatRooms[roomname].clients;
                        for (int client_fd : client_fds) {
                            if (client_fd != *it) {
                                write(client_fd, &packet, sizeof(packet));
                            }
                        }
                        ++it;
                    }
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