#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>
#include <termios.h>
#include <poll.h>
#include <unordered_map>
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

std::unordered_map <std::string, chatRoom> chatRooms;
std::unordered_map <std::string, std::string> userNames;

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

    // 1. 서버 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int option=1;
    int optlen=sizeof(option);    
    setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&option,optlen);

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
    
    struct pollfd fds[1024];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    for(int i = 1; i < 1024; i++) {
        fds[i].fd = -1; // 초기화
    }

    while(1) {

        int activity = poll(fds, client_fds.size() + 1, -1);
        if (activity < 0) {
            perror("poll error");
            exit(EXIT_FAILURE);
        }

        // 새로운 클라이언트 접속 확인
        if (fds[0].revents & POLLIN) {
            int temp_client = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (temp_client < 0) {
                perror("accept failed");
                exit(EXIT_FAILURE);
            }

            client_fds.push_back(temp_client);

            fds[client_fds.size()].fd = temp_client;
            fds[client_fds.size()].events = POLLIN;

            std::cout << client_fds.size() << " Client connected!" << std::endl;
        }

        // 기존 클라이언트들 데이터 읽기 및 에코 처리
        for(int i = 1; i < client_fds.size() + 1; i++) {
            if (fds[i].revents & POLLIN) {
                int n = read(fds[i].fd, &packet, sizeof(packet));
                if (n <= 0) {
                    int client_fd = fds[i].fd;
                    std::string username(packet.user_name);
                    std::string roomname = userNames[username];
                    chatRooms[roomname].clients.erase(client_fd);
                    userNames.erase(username);

                    packet.cmd = EXIT;
                    strncpy(packet.buffer, roomname.c_str(), sizeof(packet.buffer) - 1);

                    for (int temp_fd : chatRooms[roomname].clients) {
                        if (temp_fd != fds[i].fd) {
                            write(temp_fd, &packet, sizeof(packet));
                        }
                    }

                    close(fds[i].fd);
                    fds[i].fd = -1;
                    client_fds.erase(client_fds.begin() + (i - 1));
                    std::cout << "Client left, "<< client_fds.size() << " Client left" << std::endl;
                } else {
                    if(packet.cmd == JOIN){
                        std::string roomname(packet.buffer);
                        std::string username(packet.user_name);
                    
                        userNames[username] = roomname;
                    
                        if(auto room = chatRooms.find(roomname); room != chatRooms.end()){
                            room->second.clients.insert(fds[i].fd);
                        } else {
                            chatRoom newRoom;
                            newRoom.name = roomname;
                            newRoom.clients.insert(fds[i].fd);
                            chatRooms[roomname] = newRoom;
                        }

                        for (int client_fd : chatRooms[roomname].clients) {
                            if (client_fd != fds[i].fd) {
                                write(client_fd, &packet, sizeof(packet));
                            }
                        }
                    }
                    else if(packet.cmd == MESSAGE){
                        std::string username(packet.user_name);
                        std::string roomname = userNames[username];
                        for (int client_fd : chatRooms[roomname].clients) {
                            if (client_fd != fds[i].fd) {
                                write(client_fd, &packet, sizeof(packet));
                            }
                        }
                    }
                }
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
