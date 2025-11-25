#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <termios.h>
#include <sys/epoll.h>

#define BUFFER_SIZE 1024

#define JOIN 1
#define MESSAGE 2
#define EXIT 3

typedef struct {
    int cmd;
    char user_name[100];
    char buffer[BUFFER_SIZE];
} Packet;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <IP> <PORT>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[2]);
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        return -1;
    }

    std::cout << "Connected to server!" << std::endl;

    struct epoll_event ev{}, events[2];
    int epfd = epoll_create1(0);
    ev.events = EPOLLIN;

    ev.data.fd = sock;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev);

    ev.data.fd = STDIN_FILENO;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);

    Packet packet{};
    std::string name, input;

    std::cout << "Enter your name: ";
    std::getline(std::cin, name);

    std::cout << "Enter room name to join: ";
    std::getline(std::cin, input);

    strncpy(packet.user_name, name.c_str(), sizeof(packet.user_name) - 1);
    strncpy(packet.buffer, input.c_str(), sizeof(packet.buffer) - 1);
    packet.cmd = JOIN;

    write(sock, &packet, sizeof(packet));
    std::cout << "Joined room: " << packet.buffer << std::endl;

    while (true) {
        int nfds = epoll_wait(epfd, events, 2, -1);
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == sock) {
                int bytes = read(sock, &packet, sizeof(packet));
                if (bytes <= 0) {
                    std::cout << "Disconnected from server." << std::endl;
                    close(sock);
                    close(epfd);
                    return 0;
                }

                switch (packet.cmd) {
                    case JOIN:
                        std::cout << packet.user_name << " joined " << packet.buffer << "." << std::endl;
                        break;
                    case MESSAGE:
                        std::cout << packet.user_name << " > " << packet.buffer << std::endl;
                        break;
                    case EXIT:
                        std::cout << packet.user_name << " left " << packet.buffer << "." << std::endl;
                        break;
                }
            } else if (events[i].data.fd == STDIN_FILENO) {
                std::getline(std::cin, input);
                if (std::cin.eof()) {
                    std::cout << "EOF detected. Exiting.\n";
                    close(sock);
                    close(epfd);
                    return 0;
                }

                if (input == "exit") {
                    packet.cmd = EXIT;
                    strncpy(packet.buffer, "leaving", sizeof(packet.buffer) - 1);\
                    write(sock,&packet,sizeof(packet));
                    close(sock);
                    close(epfd);
                    return 0;
                }

                packet.cmd = MESSAGE;
                strncpy(packet.user_name, name.c_str(), sizeof(packet.user_name) - 1);
                strncpy(packet.buffer, input.c_str(), sizeof(packet.buffer) - 1);
                write(sock, &packet, sizeof(packet));
            }
        }
    }
    close(sock);
    close(epfd);
    return 0;    
}
