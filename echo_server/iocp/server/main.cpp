#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <vector>
#include <thread>
#include <winsock2.h>
#include <mswsock.h>



#pragma comment(lib, "ws2_32.lib")

using namespace std;

// IO 작업의 종류를 구분하기 위한 열거형
enum IO_TYPE {
    IO_RECV,
    IO_SEND
};

// OVERLAPPED 구조체를 확장해서 필요한 정보를 더 담습니다.
// 이 구조체가 "작업 지시서" 역할을 합니다.
struct OverlappedEx {
    OVERLAPPED overlapped; // 무조건 맨 앞에 있어야 함! (포인터 캐스팅을 위해)
    IO_TYPE type;          // 이게 Recv 작업인지 Send 작업인지 표시
    WSABUF wsaBuf;         // 데이터를 담을 버퍼 정보
    char buffer[1024];     // 실제 데이터가 오가는 버퍼
    SOCKET socket;         // 누구의 소켓인지
};

// IOCP 핸들 전역 변수
HANDLE hIocp;

// 워커 스레드 함수 (일개미)
void WorkerThread() {
    while (true) {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0; // 우리는 이걸 안 쓰고 OverlappedEx로 구분할 겁니다.
        LPOVERLAPPED pOverlapped = nullptr;

        // 1. 완료된 작업이 있는지 기다립니다. (일감이 올 때까지 잠듭니다)
        BOOL ret = GetQueuedCompletionStatus(hIocp, &bytesTransferred, &completionKey, &pOverlapped, INFINITE);

        // 2. 에러 처리 혹은 종료 처리
        if (pOverlapped == nullptr) {
            // 타임아웃이나 치명적 에러 등
            continue;
        }

        // OVERLAPPED 포인터를 우리가 확장한 구조체로 캐스팅해서 정보를 꺼냅니다.
        OverlappedEx* pEx = (OverlappedEx*)pOverlapped;

        // 연결이 끊긴 경우 (전송된 바이트가 0이면 보통 연결 종료)
        if (ret == FALSE || (bytesTransferred == 0)) {
            cout << "[Disconnection] Socket: " << pEx->socket << endl;
            closesocket(pEx->socket);
            delete pEx; // 할당했던 메모리 해제
            continue;
        }

        // 3. 작업 종류에 따른 처리
        if (pEx->type == IO_RECV) {
            // [데이터 수신 완료]

            // 받은 문자열 끝에 null 문자 넣어주기 (출력용)
            // 주의: 실제로는 바이너리 데이터일 수 있으므로 조심해야 함
            if (bytesTransferred < 1024) pEx->buffer[bytesTransferred] = '\0';

            cout << "[RECV] " << pEx->buffer << " (" << bytesTransferred << " bytes)" << endl;

            // [ECHO: 받은 걸 그대로 다시 보낸다]
            // Recv용 구조체를 재활용하지 않고 Send용을 새로 만듭니다 (개념 분리를 위해)
            // 실무에서는 메모리 풀을 써야 합니다.
            OverlappedEx* sendEx = new OverlappedEx();
            ZeroMemory(&sendEx->overlapped, sizeof(OVERLAPPED));
            sendEx->type = IO_SEND;
            sendEx->socket = pEx->socket;
            memcpy(sendEx->buffer, pEx->buffer, bytesTransferred); // 데이터 복사
            sendEx->wsaBuf.buf = sendEx->buffer;
            sendEx->wsaBuf.len = bytesTransferred;

            WSASend(pEx->socket, &sendEx->wsaBuf, 1, NULL, 0, &sendEx->overlapped, NULL);

            // [다시 받기 대기]
            // 에코를 보냈으니, 클라이언트가 또 뭘 보낼지 기다려야 합니다.
            // 기존 pEx를 재활용해서 다시 Recv를 겁니다.
            ZeroMemory(&pEx->overlapped, sizeof(OVERLAPPED)); // overlapped 초기화 필수
            pEx->type = IO_RECV;
            pEx->wsaBuf.buf = pEx->buffer;
            pEx->wsaBuf.len = 1024;

            DWORD flags = 0;
            WSARecv(pEx->socket, &pEx->wsaBuf, 1, NULL, &flags, &pEx->overlapped, NULL);
        }
        else if (pEx->type == IO_SEND) {
            // [데이터 송신 완료]
            // Send가 완료되었다는 통지입니다.
            // 여기서는 딱히 할 게 없으므로 메모리만 해제합니다.
            // (주의: Recv용 pEx는 계속 살아있어서 재활용되지만, Send용은 일회용으로 썼습니다)
            delete pEx;
        }
    }
}

int main() {
    // 1. 윈속 초기화
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 2. IOCP 생성
    hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

    // 3. 워커 스레드 생성 (CPU 개수만큼 만드는 게 정석이지만, 여기선 2개만)
    vector<thread> workers;
    for (int i = 0; i < 2; ++i) {
        workers.emplace_back(WorkerThread);
    }

    // 4. 리슨 소켓 생성
    SOCKET listenSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

    SOCKADDR_IN serverAddr;
    ZeroMemory(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9000); // 포트 9000
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    listen(listenSocket, SOMAXCONN);

    cout << "Echo Server Started on Port 9000..." << endl;

    while (true) {
        // 5. Accept 처리 (메인 스레드가 담당)
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

        if (clientSocket == INVALID_SOCKET) continue;

        cout << "[Connected] Client IP: " << inet_ntoa(clientAddr.sin_addr) << endl;

        // 6. 소켓과 IOCP 연결 (이제 이 소켓의 I/O 완료는 IOCP가 관리함)
        // CompletionKey(3번째 인자)는 여기서 안 쓰고 0으로 넘깁니다.
        CreateIoCompletionPort((HANDLE)clientSocket, hIocp, 0, 0);

        // 7. 최초의 Recv 요청 (일감 던지기)
        OverlappedEx* recvEx = new OverlappedEx();
        ZeroMemory(&recvEx->overlapped, sizeof(OVERLAPPED));
        recvEx->type = IO_RECV;
        recvEx->socket = clientSocket;
        recvEx->wsaBuf.buf = recvEx->buffer;
        recvEx->wsaBuf.len = 1024;

        DWORD flags = 0;
        // 비동기 Recv 호출. 즉시 리턴되고, 완료되면 워커 스레드가 깨어남.
        WSARecv(clientSocket, &recvEx->wsaBuf, 1, NULL, &flags, &recvEx->overlapped, NULL);
    }

    // 종료 처리 (실제로는 여기까지 안 옴)
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}