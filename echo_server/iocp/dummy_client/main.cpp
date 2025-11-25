#include <iostream>
#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

const int DUMMY_COUNT = 50;  // 동시 접속자 수 (너무 높이면 PC가 힘들어함)
const int SERVER_PORT = 9000;
const char* SERVER_IP = "127.0.0.1";

mutex consoleLock; // 콘솔 출력이 섞이지 않게 막아주는 락

// 각 더미 클라이언트가 실행할 함수 (봇)
void DummyBot(int id) {
    // 1. 소켓 생성
    SOCKET clientSock = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSock == INVALID_SOCKET) return;

    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    // 2. 접속 시도
    if (connect(clientSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        // 접속 실패 시 조용히 종료 (로그 너무 많이 뜨면 안보이니까)
        closesocket(clientSock);
        return;
    }

    {
        lock_guard<mutex> lock(consoleLock);
        cout << "[Bot " << id << "] Connected!" << endl;
    }

    char sendBuf[128];
    char recvBuf[1024];

    // 3. 무한 루프: 보내고 -> 받고 -> 쉰다
    while (true) {
        // 보낼 메시지 만들기
        sprintf_s(sendBuf, "Hello I am Bot %d", id);

        // Send
        int sendResult = send(clientSock, sendBuf, (int)strlen(sendBuf), 0);
        if (sendResult == SOCKET_ERROR) break;

        // Recv (Echo 대기)
        int recvLen = recv(clientSock, recvBuf, 1024, 0);
        if (recvLen == 0 || recvLen == SOCKET_ERROR) break;

        // 로그 출력 (너무 빠르면 정신없으니 주석 처리 가능)
        /*
        recvBuf[recvLen] = '\0';
        {
            lock_guard<mutex> lock(consoleLock);
            cout << "[Bot " << id << "] Recv: " << recvBuf << endl;
        }
        */

        // 0.5초 ~ 1초 휴식 (너무 빠르면 디도스 공격이 됨)
        Sleep(rand() % 500 + 500);
    }

    closesocket(clientSock);

    {
        lock_guard<mutex> lock(consoleLock);
        cout << "[Bot " << id << "] Disconnected." << endl;
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    vector<thread> threads;

    cout << "Starting " << DUMMY_COUNT << " Dummy Clients..." << endl;

    // 더미 봇 생성 및 실행
    for (int i = 0; i < DUMMY_COUNT; ++i) {
        threads.emplace_back(DummyBot, i + 1);
        Sleep(10); // 접속 폭주 방지용 딜레이
    }

    // 모든 스레드가 끝날 때까지 대기 (사실상 무한 대기)
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    WSACleanup();
    return 0;
}