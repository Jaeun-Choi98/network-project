#include "Common.h"

#define BUFSIZE 512

// 송/수신을 위한 쓰레드
DWORD WINAPI ProcessRecv(LPVOID arg);
DWORD WINAPI ProcessSend(LPVOID arg);

char SERVERIP[20];
int SERVERPORT;
char name[20];

int main(int argc, char* argv[]) {

	// 사용자로부터 정보 얻기
	char buf[50];
	printf("접속할 서버의 IPv4 주소를 적어주세요: ");
	while (fgets(SERVERIP, sizeof(SERVERIP), stdin) == NULL || SERVERIP[0] == '\n') {
		printf("\n접속할 서버의 IPv4 주소를 적어주세요: ");
	}

	printf("접속할 서버의 포트 번호를 적어주세요: ");
	while (fgets(buf, sizeof(buf), stdin) == NULL || buf[0] == '\n') {
		printf("\n접속할 서버의 포트 번호를 적어주세요: ");
	}
	SERVERPORT = atoi(buf);

	printf("접속할 닉네임을 적어주세요: ");
	while (fgets(name, sizeof(name), stdin) == NULL || buf[0] == '\n') {
		printf("\n접속할 닉네임을 적어주세요: ");
	}
	name[(int)strlen(name) - 1] = '\0';
	// 소켓 통신
	int retval;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");
	// 이벤트 생성

	// recv, send 쓰레드 생성
	HANDLE hThread[2];
	hThread[0] = CreateThread(NULL, 0, ProcessRecv, (LPVOID)sock, 0, NULL);
	hThread[1] = CreateThread(NULL, 0, ProcessSend, (LPVOID)sock, 0, NULL);

	WaitForMultipleObjects(2, hThread, TRUE, INFINITE);

	CloseHandle(hThread[0]);
	CloseHandle(hThread[1]);
	closesocket(sock);
	WSACleanup();
	return 0;
}

DWORD WINAPI ProcessRecv(LPVOID arg) {
	int retval;

	// 데이터 통신에 사용할 변수
	SOCKET recv_sock = (SOCKET)arg;
	char buf[BUFSIZE + 1];
	int len;
	while (1) {
		retval = recv(recv_sock, buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0)
			break;
		buf[retval] = '\0';
		printf("%s\n", buf);
	}
}

DWORD WINAPI ProcessSend(LPVOID arg) {
	int retval;

	// 데이터 통신에 사용할 변수
	SOCKET send_sock = (SOCKET)arg;
	char buf[BUFSIZE + 1];
	int len;

	retval = send(send_sock, name, (int)strlen(name), 0);
	if (retval == SOCKET_ERROR)
		err_display("send()");

	while (1) {

		if (fgets(buf, BUFSIZE + 1, stdin) == NULL)
			break;

		len = (int)strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0)
			break;

		retval = send(send_sock, buf, (int)strlen(buf), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
	}
}