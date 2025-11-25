# Network I/O Models Practice

리눅스 환경에서 다양한 **I/O 멀티플렉싱(I/O Multiplexing)** 기법과 **멀티스레딩** 모델을 활용하여 채팅 및 에코 서버를 구현한 실습 저장소입니다.
동일한 기능을 수행하는 서버를 서로 다른 네트워크 모델(**Select, Poll, Epoll, Thread**)로 구현하여 구조와 성능 차이를 학습하기 위해 작성되었습니다.

## 📂 Repository Structure

이 프로젝트는 구현 방식(I/O Model)에 따라 디렉토리가 분류되어 있습니다.

### 1. Chatting Server
다자간 채팅 기능을 다양한 모델로 구현하였습니다.

| Directory | Model | Description |
|:---|:---:|:---|
| **`chatting_server/select`** | **Select** | 전통적인 동기 I/O 멀티플렉싱 (`fd_set` 활용) |
| **`chatting_server/poll`** | **Poll** | `select`의 파일 디스크립터 개수 제한을 개선한 모델 |
| **`chatting_server/epoll`** | **Epoll** | 리눅스 특화 고성능 이벤트 기반 입출력 처리 |
| **`chatting_server/thread`** | **Multi-thread** | 클라이언트마다 별도의 스레드를 할당하는 1:1 스레드 모델 |

### 2. Echo Server
기본적인 패킷 송수신 테스트를 위한 에코 서버입니다.

- **`echo_server/server`**: 단일/다중 접속 처리를 위한 에코 서버 로직
- **`echo_server/client`**: 서버 부하 테스트를 위한 다중 클라이언트(`multiple_client`) 구현 포함

## 🛠 Environment

- **OS**: Linux (Ubuntu/CentOS)
- **Language**: C++
- **APIs**:
  - BSD Sockets (`<sys/socket.h>`)
  - POSIX Threads (`<pthread.h>`)
  - Linux System Calls (`epoll_create`, `poll`, `select`)

## 🚀 Key Concepts Learned

- **Blocking vs Non-Blocking I/O**: 소켓의 입출력 모드 제어 및 동작 차이 이해
- **I/O Multiplexing**: 단일 스레드로 여러 클라이언트의 요청을 동시에 처리하는 원리 학습
- **Context Switching**: 멀티스레드 방식과 이벤트 기반(Epoll) 방식의 오버헤드 차이 비교
- **Level Trigger vs Edge Trigger**: Epoll 사용 시 이벤트 감지 방식의 차이점 연구

## 📝 Usage

각 디렉토리로 이동하여 소스 코드를 컴파일 후 실행합니다.

```bash
# 예시: Epoll 기반 채팅 서버 실행
cd chatting_server/epoll/server
g++ -o server server.cpp
./server [PORT]
