#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netinet/in.h>

/* 상수 정의 */
#define BUF_SIZ 100	// 메세지 버퍼 크기(한 번에 전송 가능한 메세지 크기)
#define MAX_CLNT 20	// 최대 클라이언트 수

/* 함수 원형 선언 */
void *handle_clnt(void *arg);				// 클라이언트 연결을 처리하는 스레드 함수
void *send_data(void *arg, char *data, int len);	// 메세지 송신부를 제외한 연결된 모든 클라이언트에 메세지를 브로드캐스트
void error_handling(char *msg);				// 오류 메시지를 출력하고 프로그램을 종료하는 함수

/* 전역 변수 */
int clnt_cnt = 0;		// 현재 연결된 클라이언트의 수
int clnt_socks[MAX_CLNT];	// 연결된 클라이언트 소켓을 저장하는 배열
pthread_mutex_t mutx;		// 뮤텍스: 여러 스레드가 클라이언트 배열을 안전하게 공유할 수 있도록 동기화

int main(int argc, char *argv[]) {
	int serv_sock;				// 서버 소켓 (클라이언트 연결 요청을 수락하기 위한 소켓)
	int clnt_sock;				// 클라이언트 소켓 (클라이언트와 통신하기 위한 소켓)
	struct sockaddr_in serv_adr, clnt_adr;	// 서버와 클라이언트의 주소를 저장하는 구조체
	int clnt_adr_sz;			// 클라이언트 주소 구조체 크기
	pthread_t t_id;				// 새로 생성된 스레드 ID 저장

	/* 명령줄 인수가 부족한 경우 오류 출력 및 종료 */
	if(argc != 2) {
		fprintf(stderr, "Usage : %s <port>\n", argv[0]);	// 사용법 안내 메세지 출력
		exit(1);	// 프로그램 종료
	}

	/* 뮤텍스 초기화 */
	pthread_mutex_init(&mutx, NULL);		// 클라이언트 배열 접근을 동기화하기 위한 뮤텍스 초기화

	/* 1. 서버 소켓 생성 (TCP, IPv4) */
	serv_sock = socket(PF_INET, SOCK_STREAM, 0);	// IPv4, TCP 소켓 생성
	if(sock == -1) {
		error_handling("socket() error!");	// 소켓 생성 실패 시 오류 처리
	}
	
	/* 서버 주소 설정 */
	memset(&serv_adr, 0x00, sizeof(serv_adr));	// 구조체 초기화
	serv_adr.sin_family = AF_INET;			// 주소 체계: IPv4
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);	// 서버 IP 주소 설정(모든 IP로부터의 연결 허용)
	serv_adr.sin_port = htons(atoi(argv[1]));	// 서버 포트 설정

	/* 2. 소켓과 주소를 연결 */
	if(bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
		error_handling("bind() error!");	// 실패 시 오류 처리
	}
	/* 3. 연결 대기열 설정 (최대 5개 연결 대기 가능) */
	if(listen(serv_sock, 5) == -1) {
		error_handling("listen() error!");	// 실패 시 오류 처리
	}

	/* 클라이언트 연결 처리 루프 */
	while(1) {
		clnt_adr_sz = sizeof(clnt_adr);		// 클라이언트 주소 구조체 크기 설정

		/* 4. 클라이언트 연결 수락 */
		clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
		if(clnt_sock == -1) {
			error_handling("accept() error!");	// 실패 시 오류 처리
		}

		// 뮤텍스를 잠그고 클라이언트 배열에 새로운 소켓 추가
		pthread_mutex_lock(&mutx);
		clnt_socks[clnt_cnt++] = clnt_sock;	// 연결된 클라이언트 소켓을 배열에 추가
		pthread_mutex_unlock(&mutx);

		// 클라이언트 처리를 위한 스레드 생성
		pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);		// 새 스레드에서 클라이언트 처리
		pthread_detach(t_id);			// 생성된 스레드의 리소스를 자동으로 해제(좀비 스레드 방지)
		printf("Connected client IP : %s\n", inet_ntoa(clnt_adr.sin_addr));	// 연결된 클라이언트 IP 출력
	}
	
	/* 모든 작업 완료 후 스레드 남기고 소켓 닫기 */
	close(serv_sock);

	return 0;
}

/* 클라이언트를 처리하는 스레드 함수 */
void *handle_clnt(void *arg) {
	int clnt_sock = *((int*)arg);		// 메인 함수에서 전달받은 클라이언트 소켓
	int str_len = 0;			// 읽은 메시지 길이
	char data[BUF_SIZ];			// 클라이언트로부터 수신한 메시지를 저장하는 버퍼

	// 배열 초기화
	memset(&data, 0x00, BUF_SIZ);

	/* 클라이언트로부터 메시지를 계속 수신 */
	while((str_len = read(clnt_sock, data, sizeof(data))) > 0) {
		/* 메시지를 다른 클라이언트들에게 브로드캐스트 */
		send_data((void*)&clnt_sock, data, str_len);
		//fputs(data, stdout);
	}

	/* 클라이언트 연결 종료 처리 */
	pthread_mutex_lock(&mutx);			// 배열 수정 전 뮤텍스 잠금
	for(int i=0;i<clnt_cnt;i++) {			// 현재 연결된 클라이언트 소켓 배열을 탐색
		if(clnt_sock == clnt_socks[i]) {	// 연결이 끊긴 클라이언트 소켓을 배열에서 제거
			while(i++ < clnt_cnt - 1) {	// 소켓 배열을 앞으로 이동
				clnt_socks[i] = clnt_socks[i + 1];
			}
			break;
		}
	}
	clnt_cnt--;					// 연결된 클라이언트 수 감소
	pthread_mutex_unlock(&mutx);			// 뮤텍스 잠금 해제
	close(clnt_sock);				// 클라이언트 소켓 닫기

	// 스레드 종료
	return NULL;
}

/* 모든 연결된 클라이언트에게 메시지 브로드캐스트 */
void *send_data(void *arg, char *data, int len) {
	int clnt_sock = *((int*)arg);

	pthread_mutex_lock(&mutx);			// 배열 접근 전 뮤텍스 잠금
	for(int i=0;i<clnt_cnt;i++) {
		if(clnt_sock != clnt_socks[i]) {	// 송신부를 제외한 모든 연결된 클라이언트 소켓에 메시지 전송
			write(clnt_socks[i], data, len);
		}
	}
	pthread_mutex_unlock(&mutx);			// 뮤텍스 잠금 해제
}

/* 오류 처리 함수 */
void error_handling(char *msg) {
	fputs(msg, stderr);	// 표준 오류에 메세지 출력
	fputc('\n', stderr);
	exit(1);		// 프로그램 종료
}
