#pragma once
#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include <winsock2.h> // 윈속2 메인 헤더
#include <ws2tcpip.h> // 윈속2 확장 헤더

#include <stdio.h> // printf(), ...
#include <stdlib.h> // exit(), ...
#include <string.h> // strncpy(), ...
#include <Windows.h>

#pragma comment(lib, "ws2_32") // ws2_32.lib 링크

#define SERVERPORT 9000
#define BUFSIZE    4096

// 소켓 함수 오류 출력 후 종료
void err_quit(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	MessageBoxA(NULL, (const char*)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(const char* msg)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[%s] %s\n", msg, (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 소켓 함수 오류 출력
void err_display(int errcode)
{
	LPVOID lpMsgBuf;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errcode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf, 0, NULL);
	printf("[오류] %s\n", (char*)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

int main(int argc, char* argv[])
{
	int retval = 0;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	char buf[BUFSIZE];

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));

		int fileSize = 0, curRecved = 0, counter = 0;

		// 파일 크기 받기
		retval = recv(client_sock, (char*)&fileSize, sizeof(int), MSG_WAITALL);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			continue;
		}
		printf("[TCP 서버] 받은 파일 크기: %d\n", fileSize);

		// 파일 이름 받기
		size_t nameLen = 0;
		retval = recv(client_sock, (char*)&nameLen, sizeof(size_t), MSG_WAITALL);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			continue;
		}
		printf("[TCP 서버] 받은 파일 이름 길이: %d\n", (int)nameLen);

		retval = recv(client_sock, buf, nameLen, MSG_WAITALL);
		if (retval == SOCKET_ERROR)
		{
			err_display("recv()");
			continue;
		}
		buf[nameLen] = '\0';
		printf("[TCP 서버] 받은 파일 이름: %s\n", buf);

		FILE* fp = fopen(buf, "wb");
		if (fp == NULL)
		{
			printf("파일을 열 수 없습니다.\n");
		}
		else
		{
			// 파일 데이터 받기
			while (1)
			{
				int remaining = fileSize - curRecved;
				int recvLen = remaining < BUFSIZE ? remaining : BUFSIZE;

				retval = recv(client_sock, buf, recvLen, MSG_WAITALL);
				if (retval == SOCKET_ERROR)
				{
					err_display("recv()");
					break;
				}
				else if (remaining <= 0 || retval == 0)
				{
					system("cls");
					printf("파일 전송률 : [%.2f%%], %d / %d Bytes\n", (float)curRecved / (float)fileSize * 100, curRecved, fileSize);
					counter = 0;
					break;
				}

				fwrite(buf, 1, retval, fp);
				curRecved += retval;
				if (++counter > 10000)
				{
					system("cls");
					printf("파일 전송률 : [%.2f%%], %d / %d Bytes\n", (float)curRecved / (float)fileSize * 100, curRecved, fileSize);
					counter = 0;
				}
			}

			fclose(fp);
		}

		// 소켓 닫기
		closesocket(client_sock);
		printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));
	}

	// 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}