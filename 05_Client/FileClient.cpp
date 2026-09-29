#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32")

#define SERVERPORT 9000
#define BUFSIZE    512

//파일 정보 헤더(파일 크기, 파일 이름 길이), 패딩 없도록 pragma pack
#pragma pack(1)
struct FileInfo
{
	int fileSize;
	size_t nameLen;
};
#pragma pack()

int main(int argc, char* argv[])
{
	int retval;
	if (argc < 2) return 0;

	//윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	//소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) exit(1);

	//서버 주소정보를 담는 serveraddr
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	//본인 컴퓨터 IP로 접속
	inet_pton(AF_INET, "127.0.0.1", &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	//서버에 connect()
	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) exit(1);

	//데이터 송신용 버퍼
	char buf[BUFSIZE];

	//바이너리 읽기 모드로 파일 열기
	FILE* fp = fopen(argv[1], "rb");
	if (fp == NULL)
	{
		printf("파일을 열 수 없습니다.\n");
	}
	else
	{
		struct FileInfo fInfo;

		//파일 사이즈 읽기
		//파일 위치 지시자를 맨 뒤로 옮기고, ftell()을 통해 파일 시작에서 얼마나 떨어졌는지 계산
		fseek(fp, 0, SEEK_END);
		fInfo.fileSize = ftell(fp);
		fInfo.nameLen = strlen(argv[1]);
		
		//위치 지시자 위치를 파일 시작으로 초기화
		fseek(fp, 0, SEEK_SET);

		//파일 사이즈, 파일 이름 길이 정보 보내기
		retval = send(sock, (char*)&fInfo, sizeof(struct FileInfo), 0);
		if (retval == SOCKET_ERROR)	{ printf("ERROR::send()"); }

		//파일 이름 보내기
		retval = send(sock, argv[1], sizeof(char) * fInfo.nameLen, 0);
		if (retval == SOCKET_ERROR)	{ printf("ERROR::send()"); }
		printf(
			"[TCP 클라이언트] 파일 이름: %d바이트를 보냈습니다.\n"
			"[TCP 클라이언트] 파일 이름 길이: %d바이트를 보냈습니다.\n"
			"[TCP 클라이언트] 파일 사이즈: %d바이트를 보냈습니다.\n",
			(int)fInfo.nameLen, (int)sizeof(size_t), fInfo.fileSize
		);

		//파일 바이너리 데이터 보내기. 보낼 데이터가 더 없다면 루프 종료
		int sendLen = 0;
		while ((sendLen = fread(buf, 1, BUFSIZE, fp)) != NULL)
		{
			retval = send(sock, buf, sendLen, 0);
			if (retval == SOCKET_ERROR)	{ printf("ERROR::send()");	break; }
		}
		printf("[TCP 클라이언트] 파일 송신 종료.\n");
		fclose(fp);
	}

	// 소켓 닫기
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}