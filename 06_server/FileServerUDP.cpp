#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>

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

int main(int argc, char* argv[])
{
	int retval = 0;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) exit(1);

	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	//bind 후 정보 저장 및 오류 체크
	retval = bind(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) exit(1);

	struct sockaddr_in clientaddr;
	int addrlen;
	char addr[INET_ADDRSTRLEN];
	char buf[BUFSIZE + 1];

	while (1) {

		// 클라이언트 정보 얻기
		addrlen = sizeof(clientaddr);

		//전체 파일 바이트, 현재 받은 바이트, 수신 횟수 갱신 카운터
		long long curRecved = 0;
		struct FileInfo fInfo;

		try
		{
			//파일 크기 받기 (실패 시 소켓 닫기)
			retval = recvfrom(sock, (char*)&fInfo, sizeof(struct FileInfo), 0,
				(struct sockaddr*)&clientaddr, &addrlen);
			if (retval == SOCKET_ERROR) throw;

			//파일 이름 길이만큼 파일 이름 받기 (실패 시 소켓 닫기)
			retval = recvfrom(sock, buf, sizeof(char) * fInfo.nameLen, 0,
				(struct sockaddr*)&clientaddr, &addrlen);
			if (retval == SOCKET_ERROR) throw;
		}
		catch (...)
		{
			printf("ERROR::recv()");

			closesocket(sock);
			return 0;
		}

		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		buf[fInfo.nameLen] = '\0';

		printf(
			"\n[Server] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n"
			"[Server] 받은 파일 이름: %s\n"
			"[Server] 받은 파일 크기: %lld\n",
			addr, ntohs(clientaddr.sin_port),
			buf,
			fInfo.fileSize
		);

		FILE* fp = fopen(buf, "wb");
		if (fp == NULL)
		{
			printf("파일을 열 수 없습니다.");
		}
		else
		{
			while (1)
			{
				//남은 데이터 용량 계산
				long long remaining = fInfo.fileSize - curRecved;
				int recvLen = remaining < BUFSIZE ? remaining : BUFSIZE;

				retval = recvfrom(sock, buf, recvLen, 0,
					(struct sockaddr*)&clientaddr, &addrlen);

				if (retval == SOCKET_ERROR)
				{
					printf("ERROR::recv()");
					break;
				}
				else if (retval == 0)
				{
					printf(
						"[Server] 파일 수신율 : [100%%], %lld / %lld Bytes",
						curRecved, fInfo.fileSize
					);
					break;
				}

				//파일에 데이터 작성
				fwrite(buf, 1, retval, fp);
				curRecved += retval;
			}
			fclose(fp);
		}

		return 0;
	}

	// 소켓 닫기
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}