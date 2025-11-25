#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <termios.h>

#define BUFFER_SIZE 1024

#define JOIN 1
#define MESSAGE 2
#define EXIT 3

typedef struct{
    int cmd;
    char user_name[100];
    char buffer[1024];    
} Packet;


int main(int argc , char *argv[]) {

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }
    int port = std::stoi(argv[2]);

    int sock = 0, fd_num, fd_max;
    struct sockaddr_in serv_addr;
  
    std::string name, input;
    Packet packet;

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

    std::cout << "Enter your name: ";
    std::getline(std::cin, name);
    strncpy(packet.user_name, name.c_str(), sizeof(packet.user_name) - 1);
    packet.user_name[sizeof(packet.user_name) - 1] = '\0';

    std::cout << "들어갈 방의 이름을 입력하세요: ";
    std::cin >> packet.buffer;
    std::cin.ignore();
    packet.cmd = JOIN;
    write(sock, &packet, sizeof(packet));
    std::cout << packet.buffer << " 방에 들어갔습니다." << std::endl;

    // 5. 메시지 보내고 받기
    while (1) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        fd_max = std::max(sock, STDIN_FILENO);

        if((fd_num = select(fd_max + 1, &read_fds, NULL, NULL, NULL)) < 0){
            perror("select failed");
            exit(EXIT_FAILURE);
        }

        if(FD_ISSET(STDIN_FILENO, &read_fds)){   
            
            std::cout << "[보내기] ";
            
            std::getline(std::cin, input);
            if (input == "exit") break;
            
            strncpy(packet.user_name, name.c_str(), sizeof(packet.user_name) - 1);
            packet.user_name[sizeof(packet.user_name) - 1] = '\0';

            strncpy(packet.buffer, input.c_str(), sizeof(packet.buffer) - 1);
            packet.buffer[sizeof(packet.buffer) - 1] = '\0';

            packet.cmd = MESSAGE;
            write(sock, &packet, sizeof(packet));
            std::cout << input << std::endl;
        }

        if(FD_ISSET(sock, &read_fds)){
            int valread = read(sock, &packet, sizeof(packet));
            if (valread > 0) {
                std::cout << "[받기] " << packet.user_name << " : " << packet.buffer << std::endl;
                memset(&packet, 0, sizeof(packet));
            }
        }  
    }

    // 6. 소켓 닫기
    close(sock);

    return 0;
}
