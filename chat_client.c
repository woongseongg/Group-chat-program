#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>

/* 상수 정의 */
#define BUF_SIZ 100	// 메세지 버퍼 크기(한 번에 전송 가능한 메세지 크기)
#define NAME_SIZ 20	// 사용자 이름의 최대 크기

/* 함수 원형 선언 */
void *send_msg(void *arg);		// 서버로 메세지를 전송하는 스레드 함수
void *recv_msg(void *arg);		// 서버로부터 메세지를 수신하는 스레드 함수
void error_handling(char *msg);		// 프로그램 실행 중 오류가 발생했을 때 출력하는 함수

/* 전역 변수 */
char name[NAME_SIZ] = "[DEFAULT]";	// 사용자 이름을 저장하는 변수(기본값 : [DEFAULT])
char msg[BUF_SIZ];			// 사용자가 입력한 메세지를 저장하는 버퍼

int main(int argc, char *argv[]) {
	int sock;				// 서버와의 통신에 사용될 소켓
	struct sockaddr_in serv_addr;		// 서버 주소를 저장하는 구조체
	pthread_t snd_thread, rcv_thread;	// 송신 스레드와 수신 스레드의 ID를 저장하는 변수
	void *thread_return;			// 스레드가 반환하는 값을 저장하는 변수

	/* 명령줄 인수가 부족한 경우 오류 출력 및 종료 */
	if(argc != 4) {
		fprintf(stderr, "Usage : %s <IP> <port> <name>\n", argv[0]);	// 사용법 안내 메세지 출력
		exit(1);	// 프로그램 종료
	}

	// 사용자 이름 설정
	sprintf(name, "[%s]", argv[3]);
	// TCP 소켓 생성
	sock = socket(PF_INET, SOCK_STREAM, 0);	// IPv4, TCP 소켓 생성
	if(sock == -1) {
		error_handling("socket() error!");	// 소켓 생성 실패 시 오류 처리
	}

	/* 서버 주소 설정 */
	memset(&serv_addr, 0x00, sizeof(serv_addr));	// 구조체 초기화
	serv_addr.sin_family = AF_INET;			// 주소 체계: IPv4
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);	// 서버 IP 주소 설정
	serv_addr.sin_port = htons(atoi(argv[2]));	// 서버 포트 설정

	/* 서버에 연결 요청 */
	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		error_handling("connect() error!");	// 연결 실패 시 오류 처리
	}

	/* 송신 및 수신 스레드 생성 */
	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);	// 메세지 송신 스레드 생성
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);	// 메세지 수신 스레드 생성
	/* 두 스레드의 종료를 대기 */
	pthread_join(snd_thread, &thread_return);	// 송신 스레드 종료 대기
	pthread_join(rcv_thread, &thread_return);	// 수신 스레드 종료 대기

	/* 모든 작업 완료 후 스레드 남기고 소켓 닫기 */
	close(sock);

	return 0;
}

/* 메세지를 서버로 송신하는 스레드 함수 */
void *send_msg(void *arg) {
	int sock = *((int*)arg);		// 메인 함수에서 전달된 소켓을 가져옴
	char send_msg[NAME_SIZ + BUF_SIZ];	// 사용자 이름과 메세지를 합친 최종 송신 데이터

	while(1) {
		fgets(msg, BUF_SIZ, stdin);	// 최대 BUF_SIZ 바이트만큼 사용자 입력 받기 
		// 'q' 또는 'Q' 입력 시 종료
		if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")) {
			close(sock);
			exit(0);
		}
		// [사용자 이름] 메세지 형식으로 작성
		sprintf(send_msg, "%s %s", name, msg);
		// 서버로 메세지 전송
		write(sock, send_msg, sizeof(send_msg));
	}
	// 함수 종료
	return NULL;
}

/* 서버로부터 메세지를 수신하는 스레드 함수 */
void *recv_msg(void *arg) {
	int sock = *((int*)arg);		// 메인 함수에서 전달된 소켓 가져옴
	char recv_msg[NAME_SIZ + BUF_SIZ];	// 서버로부터 수신한 메세지를 저장할 버퍼
	int str_len = 0;			// 수신한 데이터의 길이 저장

	while(1) {
		// 서버로부터 메세지 읽기
		str_len = read(sock, recv_msg, NAME_SIZ + BUF_SIZ - 1);
		if(str_len < 0) {	// read()가 실패했거나 연결이 끊어진 경우
			return (void*)-1;	// 스레드 종료
		}
		recv_msg[str_len] = 0;		// 수신된 문자열 끝에 NULL 추가
		fputs(recv_msg, stdout);	// 수신한 메세지를 터미널에 출력
	}
	// 함수 종료
	return NULL;
}

/* 오류 처리 함수 */
void error_handling(char *msg) {
	fputs(msg, stderr);	// 표준 오류에 메세지 출력
	fputc('\n', stderr);
	exit(1);		// 프로그램 종료
}
