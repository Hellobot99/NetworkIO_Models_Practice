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
#include <sys/epoll.h>
#include <thread>
#include <mutex>
#include "thread_pool.h"

#define BUFFER_SIZE 1024
#define MAX_EVENTS 100

#define JOIN 1
#define MESSAGE 2
#define EXIT 3

typedef struct
{
    int cmd;
    char user_name[100];
    char buffer[1024];
} Packet;

typedef struct
{
    std::string name;
    std::unordered_set<int> clients;
} chatRoom;

void new_client(int server_fd, int epfd, struct sockaddr_in address, int addrlen, std::vector<int> &client_fds, std::mutex &data_mutex);
void handle_client(int client_fd, int epfd, std::vector<int> &client_fds, std::unordered_map<std::string, chatRoom> &chatRooms, std::unordered_map<std::string, std::string> &userNames, std::mutex &data_mutex);

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    Packet packet;

    std::mutex data_mutex;

    std::unordered_map<std::string, chatRoom> chatRooms;
    std::unordered_map<std::string, std::string> userNames;

    // 1. 서버 소켓 생성
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int option = 1;
    int optlen = sizeof(option);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, optlen);

    // 2. 주소 설정
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // 3. 바인딩
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 4. 리슨
    if (listen(server_fd, 1) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port << "..." << std::endl;

    std::vector<int> client_fds;

    struct epoll_event ev, events[MAX_EVENTS];
    int epfd = epoll_create(1), nfds;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

    ThreadPool threadPool(20); // ThreadPool 생성

    while (1)
    {

        nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; i++)
        {
            int fd = events[i].data.fd;   
            // 새로운 클라이언트 접속 확인
            if (fd == server_fd)
            {
                struct epoll_event ev_cpy = ev;
                threadPool.enqueue(std::bind(new_client, server_fd, epfd, address, std::ref(addrlen), std::ref(client_fds), std::ref(data_mutex)));;
            }
            // 기존 클라이언트들 데이터 읽기 및 에코 처리
            else
            {             
                threadPool.enqueue(std::bind(handle_client, fd, epfd,std::ref(client_fds), std::ref(chatRooms), std::ref(userNames), std::ref(data_mutex)));                
            }
        }
    }

    std::cout << "Server shutting down..." << std::endl;

    // 7. 소켓 닫기
    for (int client_fd : client_fds)
    {
        close(client_fd);
    }
    close(server_fd);

    return 0;
}

void new_client(int server_fd, int epfd, struct sockaddr_in address, int addrlen, std::vector<int> &client_fds, std::mutex &data_mutex)
{
    // Accept a new client connection
    int temp_client = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    
    if (temp_client < 0)
    {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = temp_client;
    epoll_ctl(epfd, EPOLL_CTL_ADD, temp_client, &ev);

    std::lock_guard<std::mutex> lock(data_mutex);

    std::cout << client_fds.size() << " Client connected!" << std::endl;
    
    client_fds.push_back(temp_client);        
}

void handle_client(int client_fd, int epfd, std::vector<int> &client_fds, std::unordered_map<std::string, chatRoom> &chatRooms, std::unordered_map<std::string, std::string> &userNames, std::mutex &data_mutex)
{
    Packet packet;
    memset(&packet, 0, sizeof(packet));

    int n = read(client_fd, &packet, sizeof(packet));
    
    if (n <= 0)
    {
        /*
        std::string username(packet.user_name);
        std::string roomname = userNames[username];

        packet.cmd = EXIT;
        strncpy(packet.buffer, roomname.c_str(), sizeof(packet.buffer) - 1);
        epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, nullptr);

        for (int temp_fd : chatRooms[roomname].clients)
        {
            if (temp_fd != client_fd)
            {
                write(temp_fd, &packet, sizeof(packet));
            }
        }

        std::lock_guard<std::mutex> lock(data_mutex);    

        chatRooms[roomname].clients.erase(client_fd);
        userNames.erase(username);

        close(client_fd);
        client_fds.erase(std::remove(client_fds.begin(), client_fds.end(), client_fd), client_fds.end());
        std::cout << client_fds.size() << " Client left" << std::endl;

        return;
        */
    }
    else
    {
        if (packet.cmd == JOIN)
        {
            std::string roomname(packet.buffer);
            std::string username(packet.user_name);

            userNames[username] = roomname;

            for (int fd : chatRooms[roomname].clients)
            {
                if (fd != client_fd)
                {
                    write(fd, &packet, sizeof(packet));
                }
            }            

            std::lock_guard<std::mutex> lock(data_mutex); 

            if (auto room = chatRooms.find(roomname); room != chatRooms.end())
            {
                room->second.clients.insert(client_fd);
            }
            else
            {
                chatRoom newRoom;
                newRoom.name = roomname;
                newRoom.clients.insert(client_fd);
                chatRooms[roomname] = newRoom;
            }
        }
        else if (packet.cmd == MESSAGE)
        {
            std::string username(packet.user_name);
            std::string roomname = userNames[username];
            for (int fd : chatRooms[roomname].clients)
            {
                if (fd != client_fd)
                {
                    write(fd, &packet, sizeof(packet));
                }
            }
        }
        else if (packet.cmd == EXIT)
        {
            std::string username(packet.user_name);
            std::string roomname = userNames[username];

            packet.cmd = EXIT;
            strncpy(packet.buffer, roomname.c_str(), sizeof(packet.buffer) - 1);
            epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, nullptr);

            for (int temp_fd : chatRooms[roomname].clients)
            {
                if (temp_fd != client_fd)
                {
                    write(temp_fd, &packet, sizeof(packet));
                }
            }
            std::lock_guard<std::mutex> lock(data_mutex); 
            chatRooms[roomname].clients.erase(client_fd);
            userNames.erase(username);

            close(client_fd);
            client_fds.erase(std::remove(client_fds.begin(), client_fds.end(), client_fd), client_fds.end());
            std::cout << client_fds.size() << " Client left" << std::endl;

        }
    }
}