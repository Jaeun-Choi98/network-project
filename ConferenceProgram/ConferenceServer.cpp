#include "Common.h"

#define SERVERPORT 9000
#define BUFSIZE 512
#define NAMESIZE 20

// �۾� ������
DWORD WINAPI WorkerThread(LPVOID arg);

typedef struct _SOCKETINFO {
	OVERLAPPED overlapped;
	SOCKET sock;
	char name[NAMESIZE];
	bool checkName;
	int namebytes;
	char buf[BUFSIZE + 1];
	int recvbytes;
	int sendbytes;
	int checkAll;
	WSABUF wsabuf;
	_SOCKETINFO* next;
} SOCKETINFO;

// ����� Ŭ���̾�Ʈ���� �����ϱ� ���� ����Ʈ
SOCKETINFO* SocketInfoList;
void AddSocketInfo(SOCKETINFO* SocketInfo);
void RemoveSocketInfo(SOCKETINFO* SocketInfo);

FILE* pFile;

int main(int argc, char* argv[]) {
	int retval;
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	/*--- Completeion Port ���� ����ϱ� ���� ����� ��Ʈ ���� �� �۾��� ������ ���� ���� ---*/
	HANDLE hcp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (hcp == NULL) return 1;

	SYSTEM_INFO si;
	GetSystemInfo(&si);

	HANDLE hThread;
	for (int i = 0; i < (int)si.dwNumberOfProcessors * 2; i++) {
		hThread = CreateThread(NULL, 0, WorkerThread, hcp, 0, NULL);
		if (hThread == NULL) return 1;
		CloseHandle(hThread);
	}
	/*--- ����� ��Ʈ �� �۾��� ������ ���� ���� ---*/

	/*--- ȸ�� ������ ������ ���� ���� ���� ---*/
	pFile = fopen("conference.txt", "a");

	SOCKET listen_sock4 = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock4 == INVALID_SOCKET) err_quit("socket()");

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock4, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	retval = listen(listen_sock4, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	DWORD recvbytes, flags;
	SocketInfoList = NULL;

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock4, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// ������ Ŭ���̾�Ʈ ���� ���
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			addr, ntohs(clientaddr.sin_port));

		// ���ϰ� ����� �Ϸ� ��Ʈ ����
		CreateIoCompletionPort((HANDLE)client_sock, hcp, client_sock, 0);

		// ���� ���� ����ü �Ҵ�
		SOCKETINFO* ptr = new SOCKETINFO;
		if (ptr == NULL) break;
		memset(&ptr->overlapped, 0, sizeof(ptr->overlapped));
		ptr->sock = client_sock;
		ptr->checkName = false;
		ptr->recvbytes = ptr->sendbytes = ptr->namebytes = 0;
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;
		ptr->checkAll = 0;
		ptr->next = NULL;

		// �������� ����Ʈ�� �߰�
		AddSocketInfo(ptr);

		// �񵿱� ����� ����
		flags = 0;
		retval = WSARecv(client_sock, &ptr->wsabuf, 1, &recvbytes,
			&flags, &ptr->overlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				err_display("WSARecv()");
			}
			continue;
		}
	}
	closesocket(listen_sock4);
	fclose(pFile);
	WSACleanup();
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID arg)
{
	int retval;
	HANDLE hcp = (HANDLE)arg;
	DWORD recvbytes, flags;


	// �񵿱� ����� �Ϸ� ��ٸ���
	DWORD cbTransferred;
	SOCKETINFO* ptr;
	SOCKET client_sock;
	retval = GetQueuedCompletionStatus(hcp, &cbTransferred,
		&client_sock, (LPOVERLAPPED*)&ptr, INFINITE);

	// Ŭ���̾�Ʈ ����(IP �ּ�, ��Ʈ ��ȣ) ���
	struct sockaddr_in clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(ptr->sock, (struct sockaddr*)&clientaddr, &addrlen);
	char addr[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	// �񵿱� ����� ��� Ȯ��
	if (retval == 0 || cbTransferred == 0) {
		if (!ptr->checkName) printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n", addr, ntohs(clientaddr.sin_port));
		else {
			fputs(ptr->name, pFile);
			fputs("���� �����ϼ̽��ϴ�.\n", pFile);
			printf("%s���� �����ϼ̽��ϴ�.\n", ptr->name);
		}
		RemoveSocketInfo(ptr);
		return 0;
	}

	if (!ptr->checkName) {
		ptr->recvbytes = ptr->namebytes = cbTransferred;
		ptr->sendbytes = 0;
		strncpy(ptr->name, ptr->buf, ptr->recvbytes);
		ptr->name[ptr->recvbytes] = '\0';
		char greet[30] = "���� �����Ͽ����ϴ�.";
		strncpy(ptr->buf + ptr->recvbytes, greet, (int)strlen(greet));
		ptr->recvbytes += (int)strlen(greet);
		ptr->buf[ptr->recvbytes] = '\0';
		fputs(ptr->buf, pFile);
		fputs("\n", pFile);
		printf("%s���� �����Ͽ����ϴ�.\n", ptr->name);
	}
	else if (ptr->recvbytes == 0) {
		ptr->recvbytes = cbTransferred;
		ptr->sendbytes = 0;
		ptr->buf[ptr->recvbytes] = '\0';
		printf("[%s] %s\n", ptr->name, ptr->buf);
		fputs(ptr->name, pFile);
		fputs(": ", pFile);
		fputs(ptr->buf, pFile);
		fputs("\n", pFile);
	}
	else {
		//ptr->sendbytes += cbTransferred;
		ptr->recvbytes = 0;
	}


	if (ptr->recvbytes != 0) {
		// ������ ������
		DWORD sendbytes;
		SOCKETINFO* cur = SocketInfoList;
		int curSendbytes = ptr->sendbytes;
		int curRecvbytes = ptr->recvbytes - ptr->sendbytes;
		char curBuf[BUFSIZE + 1];
		strncpy(curBuf, ptr->buf + curSendbytes, curRecvbytes);
		char c = ':';
		if (!ptr->checkName) {
			ptr->checkName = true;
			while (cur != NULL) {
				memset(&cur->overlapped, 0, sizeof(cur->overlapped));
				cur->sendbytes = curSendbytes;
				cur->recvbytes = curRecvbytes;
				strncpy(cur->buf, curBuf, curRecvbytes);
				cur->wsabuf.buf = cur->buf;
				cur->wsabuf.len = cur->recvbytes;

				retval = WSASend(cur->sock, &cur->wsabuf, 1,
					&sendbytes, 0, &cur->overlapped, NULL);
				if (retval == SOCKET_ERROR) {
					if (WSAGetLastError() != WSA_IO_PENDING) {
						err_display("WSASend()");
					}
					return 0;
				}
				if(ptr!=cur) cur->checkAll += 1;
				cur = cur->next;
			}
		}
		else {
			while (cur != NULL) {
				memset(&cur->overlapped, 0, sizeof(cur->overlapped));
				cur->sendbytes = curSendbytes;
				cur->recvbytes = curRecvbytes + ptr->namebytes + sizeof(c);
				strncpy(cur->buf, ptr->name, ptr->namebytes);
				strncpy(cur->buf + ptr->namebytes, &c, sizeof(c));
				strncpy(cur->buf + ptr->namebytes + sizeof(c), curBuf, curRecvbytes);
				cur->wsabuf.buf = cur->buf;
				cur->wsabuf.len = cur->recvbytes;

				retval = WSASend(cur->sock, &cur->wsabuf, 1,
					&sendbytes, 0, &cur->overlapped, NULL);
				if (retval == SOCKET_ERROR) {
					if (WSAGetLastError() != WSA_IO_PENDING) {
						err_display("WSASend()");
					}
					return 0;
				}
				if (ptr != cur) cur->checkAll += 1;
				cur = cur->next;
			}
		}
	}
	else {
		if (ptr->checkAll > 0) {
			ptr->checkAll -= 1;
			return 0;
		}
		// ������ �ޱ�
		//ptr->recvbytes = 0;

		memset(&ptr->overlapped, 0, sizeof(ptr->overlapped));
		ptr->wsabuf.buf = ptr->buf;
		ptr->wsabuf.len = BUFSIZE;

		DWORD recvbytes;
		DWORD flags = 0;
		retval = WSARecv(ptr->sock, &ptr->wsabuf, 1,
			&recvbytes, &flags, &ptr->overlapped, NULL);
		if (retval == SOCKET_ERROR) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				err_display("WSARecv()");
			}
			return 0;
		}
	}

	return 0;
}

void AddSocketInfo(SOCKETINFO* ptr) {
	ptr->next = SocketInfoList;
	SocketInfoList = ptr;
}

void RemoveSocketInfo(SOCKETINFO* ptr) {
	SOCKETINFO* cur = SocketInfoList;
	SOCKETINFO* prev = NULL;
	while (cur) {
		if (cur == ptr) {
			if (prev)
				prev->next = cur->next;
			else
				SocketInfoList = cur->next;
			closesocket(ptr->sock);
			delete(cur);
			return;
		}
		prev = cur;
		cur = cur->next;
	}
}