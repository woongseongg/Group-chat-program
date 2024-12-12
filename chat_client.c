#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>

#define BUF_SIZ 100
#define NAME_SIZ 20

void *send_msg(void *arg);
void *recv_msg(void *arg);
void error_handling(char *msg);

char name[NAME_SIZ] = "[DEFAULT]";
char msg[BUF_SIZ];

int main(int argc, char *argv[]) {
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void *thread_return;

	if(argc != 4) {
		fprintf(stderr, "Usage : %s <IP> <port> <name>\n", argv[0]);
		exit(1);
	}

	sprintf(name, "[%s]", argv[3]);
	sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0x00, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
		error_handling("connect() error!");
	}

	pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
	pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
	pthread_join(snd_thread, &thread_return);
	pthread_join(rcv_thread, &thread_return);
	close(sock);

	return 0;
}

void *send_msg(void *arg) {
	int sock = *((int*)arg);
	char send_msg[NAME_SIZ + BUF_SIZ];

	while(1) {
		fgets(msg, BUF_SIZ, stdin);
		if(!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")) {
			close(sock);
			exit(0);
		}
		sprintf(send_msg, "%s %s", name, msg);
		write(sock, send_msg, sizeof(send_msg));
	}
	return NULL;
}

void *recv_msg(void *arg) {
	int sock = *((int*)arg);
	char recv_msg[NAME_SIZ + BUF_SIZ];
	int str_len = 0;

	while(1) {
		str_len = read(sock, recv_msg, NAME_SIZ + BUF_SIZ - 1);
		if(str_len < 0) {
			return (void*)-1;
		}
		recv_msg[str_len] = 0;
		fputs(recv_msg, stdout);
	}
	return NULL;
}

void error_handling(char *msg) {
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
