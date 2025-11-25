#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// 서버로부터 오는 메시지를 계속 받는 스레드
void RecvThread(SOCKET sock) {
    char buf[1024];
    while (true) {
        // 서버가 보낼 때까지 여기서 대기 (Blocking)
        int len = recv(sock, buf, 1024, 0);

        if (len == 0 || len == SOCKET_ERROR) {
            cout << "\n[System] Server disconnected." << endl;
            break;
        }

        // 받은 문자열 끝처리
        if (len < 1024) buf[len] = '\0';

        cout << "\n[Server]: " << buf << endl;
        cout << ">> "; // 입력창 프롬프트 다시 출력
    }
}

int main() {
    // 1. 윈속 초기화
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed" << endl;
        return 1;
    }

    // 2. 소켓 생성
    SOCKET clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock == INVALID_SOCKET) {
        cout << "Socket creation failed" << endl;
        return 1;
    }

    // 3. 연결할 서버 정보 설정
    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000); // 서버와 같은 포트

    // 로컬호스트(내 컴퓨터) 접속
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    // 4. 서버 연결 시도
    cout << "Connecting to server..." << endl;
    if (connect(clientSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cout << "Connect failed. Is the server running?" << endl;
        closesocket(clientSock);
        WSACleanup();
        return 1;
    }

    cout << "Connected! Type message to send." << endl;

    // 5. 수신 전용 스레드 시작
    thread receiver(RecvThread, clientSock);
    receiver.detach(); // 스레드가 알아서 돌도록 분리

    // 6. 메인 스레드는 송신(Send)만 담당
    string input;
    while (true) {
        cout << ">> ";
        getline(cin, input); // 한 줄 입력 받기

        if (input == "exit" || input == "quit") break;

        // 서버로 전송
        int ret = send(clientSock, input.c_str(), (int)input.length(), 0);
        if (ret == SOCKET_ERROR) {
            cout << "Send failed" << endl;
            break;
        }
    }

    // 종료 처리
    closesocket(clientSock);
    WSACleanup();
    return 0;
}