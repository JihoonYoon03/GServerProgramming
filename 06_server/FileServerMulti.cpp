#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

#include <chrono>

#pragma comment(lib, "ws2_32")

#define SERVERPORT 9000
#define BUFSIZE    4096

#pragma pack(1)
struct FileInfo
{
	long long fileSize;
	size_t nameLen;
};
#pragma pack()

CRITICAL_SECTION cs;

//커서 정보를 얻기 위함
CONSOLE_SCREEN_BUFFER_INFO curScreen;
HANDLE hConsole;
//최하단 행의 좌표
COORD cursorEnds;

DWORD WINAPI ThreadClient(LPVOID arg)
{
	int retval;
	COORD cursorThread;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	char buf[BUFSIZE + 1];

	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	//전체 파일 바이트, 현재 받은 바이트, 수신 횟수 갱신 카운터
	long long curRecved = 0;
	int counter = 0;
	struct FileInfo fInfo;

	try 
	{
		//파일 크기 받기 (실패 시 소켓 닫기)
		retval = recv(client_sock, (char*)&fInfo, sizeof(struct FileInfo), MSG_WAITALL);
		if (retval == SOCKET_ERROR) throw;

		//파일 이름 길이만큼 파일 이름 받기 (실패 시 소켓 닫기)
		retval = recv(client_sock, buf, sizeof(char) * fInfo.nameLen, MSG_WAITALL);
		if (retval == SOCKET_ERROR) throw;
	}
	catch (...)
	{
		EnterCriticalSection(&cs);
		printf("ERROR::recv()");
		LeaveCriticalSection(&cs);

		closesocket(client_sock);
		return 0;
	}

	buf[fInfo.nameLen] = '\0';

	EnterCriticalSection(&cs);
	SetConsoleCursorPosition(hConsole, cursorEnds);
	cursorEnds.X = 0;
	printf(
		"\n[Server] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n"
		"[Server] 받은 파일 이름: %s\n"
		"[Server] 받은 파일 크기: %lld\n",
		addr, ntohs(clientaddr.sin_port),
		buf,
		fInfo.fileSize
	);
	
	//커서의 콘솔 좌표 기억
	GetConsoleScreenBufferInfo(hConsole, &curScreen);
	cursorThread = curScreen.dwCursorPosition;
	//파일 수신율의 다다음 줄에 다른 스레드가 출력해야 함
	if (cursorEnds.Y < cursorThread.Y + 2)
		cursorEnds.Y = cursorThread.Y + 2;

	printf("[Server] 파일 수신율 : [0.0%%], 0 / %lld Bytes", fInfo.fileSize);
	LeaveCriticalSection(&cs);


	FILE* fp = fopen(buf, "wb");
	if (fp == NULL)
	{
		EnterCriticalSection(&cs);
		printf("파일을 열 수 없습니다.");
		LeaveCriticalSection(&cs);
	}
	else
	{
		while (1)
		{
			//남은 데이터 용량 계산
			long long remaining = fInfo.fileSize - curRecved;
			int recvLen = remaining < BUFSIZE ? remaining : BUFSIZE;

			retval = recv(client_sock, buf, recvLen, MSG_WAITALL);

			if (retval == SOCKET_ERROR)
			{
				EnterCriticalSection(&cs);
				printf("ERROR::recv()");
				LeaveCriticalSection(&cs);
				break;
			}
			else if (++counter >= 10000 * (4096 / BUFSIZE) || retval == 0)
			{
				EnterCriticalSection(&cs);
				SetConsoleCursorPosition(hConsole, cursorThread);
				printf(
					"[Server] 파일 수신율 : [%.2f%%], %lld / %lld Bytes",
					(double)curRecved / (double)fInfo.fileSize * 100, curRecved, fInfo.fileSize
				);
				SetConsoleCursorPosition(hConsole, cursorEnds);
				counter = 0;
				LeaveCriticalSection(&cs);
				if (retval == 0) break;
			}

			//파일에 데이터 작성
			fwrite(buf, 1, retval, fp);
			curRecved += retval;
		}
		fclose(fp);
	}

	closesocket(client_sock);

	EnterCriticalSection(&cs);
	cursorThread.Y += 1;
	SetConsoleCursorPosition(hConsole, cursorThread);
	printf(
		"[Server] 클라이언트 종료: IP 주소=%s, 포트 번호=%d",
		addr, ntohs(clientaddr.sin_port)
	);
	SetConsoleCursorPosition(hConsole, cursorEnds);
	LeaveCriticalSection(&cs);
	return 0;
}

int main(int argc, char* argv[])
{
	//스레드 별 print 위치를 고정하기 위해 임계 영역 사용
	InitializeCriticalSection(&cs);
	int retval = 0;

	hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) exit(1);

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	//bind 후 정보 저장 및 오류 체크
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) exit(1);

	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) exit(1);

	//클라이언트 소켓 정보
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	while (1) {
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET)
		{
			EnterCriticalSection(&cs);
			printf("ERROR::accept()");
			LeaveCriticalSection(&cs);
			break;
		}
		else 
		{
			CreateThread(NULL, 0, ThreadClient, (LPVOID)client_sock, NULL, NULL);
		}
	}

	// 소켓 닫기
	closesocket(listen_sock);

	//임계영역 제거
	DeleteCriticalSection(&cs);

	// 윈속 종료
	WSACleanup();
	return 0;
}