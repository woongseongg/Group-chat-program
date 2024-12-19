# 그룹 채팅 프로그램 (Group Chat Program)

## 소개
이 프로젝트는 C 언어로 구현된 그룹 채팅 프로그램입니다. 클라이언트-서버 구조를 기반으로 하며, 여러 사용자가 실시간으로 메시지를 주고받을 수 있는 환경을 제공합니다.

---

## 주요 기능
1. **클라이언트-서버 통신**: TCP/IP 소켓 프로그래밍을 통해 안정적인 메시지 송수신 지원.
2. **다중 사용자 지원**: 여러 사용자가 동시에 서버에 접속 가능.
3. **실시간 메시지 전달**: 서버를 통해 클라이언트 간 메시지가 실시간으로 전달됩니다.

---

## 프로젝트 구조
Group-chat-program-main/ 

│ 

├── chat_server.c # 서버 측 프로그램 

├── chat_client.c # 클라이언트 측 프로그램 

├── LICENSE # 라이선스 파일 

└── README.md # 프로젝트 설명 파일


---

## 요구사항

### 소프트웨어 요구사항
- C 컴파일러 (예: GCC)
- Linux 또는 Windows 운영 체제
- 네트워크 연결 지원

---

## 설치 및 실행 방법

1. **프로젝트 클론**
   ```bash
   git clone https://github.com/your-repository-url.git
   cd Group-chat-program-main
