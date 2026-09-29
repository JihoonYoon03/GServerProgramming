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
#include <profileapi.h>

#pragma comment(lib, "ws2_32")

#define SERVERPORT 9000
//최대 4KB(=메모리 1페이지 크기) 수신. 4KB 시 평균 3000ms, 512Bytes 시 평균 5000ms 소요
#define BUFSIZE    4096

//파일 정보 헤더(파일 크기, 파일 이름 길이), 패딩 없도록 pragma pack
#pragma pack(1)
struct FileInfo
{
	int fileSize;
	size_t nameLen;
};
#pragma pack()

//에러 처리용 함수
void stopConnect(SOCKET& sock, char* addr, struct sockaddr_in& sockaddr)
{
	closesocket(sock);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(sockaddr.sin_port));
}

int main(int argc, char* argv[])
{
	int retval = 0;

	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	//listen용 소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) exit(1);

	//bind()로 로컬 IP주소, 포트 번호 결정
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	//모든 연결 요청 수신할 수 있도록 INADDR_ANY
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	//bind 후 정보 저장 및 오류 체크
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) exit(1);

	//listen() 후 클라이언트 접속 대기
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) exit(1);

	//클라이언트 소켓 정보
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;
	//데이터 수신용 버퍼
	char buf[BUFSIZE];

	//성능측정
	LARGE_INTEGER Frequency, StartTime, EndTime;
	QueryPerformanceFrequency(&Frequency);
	//Ctrl + C 입력 전까지 반복
	while (1) {
		// accept() 후 클라이언트 소켓 정보 저장 및 오류 체크
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) { printf("ERROR::accept()");	break; }

		//IP주소를 문자열로 저장하기 위한 버퍼와 inet_ntop 함수
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",	addr, ntohs(clientaddr.sin_port));

		//전체 파일 바이트, 현재 받은 바이트, 수신 횟수 갱신 카운터
		int curRecved = 0, counter = 0;
		struct FileInfo fInfo;

		//파일 크기 받기 (실패 시 소켓 닫기)
		retval = recv(client_sock, (char*)&fInfo, sizeof(struct FileInfo), MSG_WAITALL);
		if (retval == SOCKET_ERROR)
		{ 
			printf("ERROR::recv()"); 
			stopConnect(client_sock, addr, clientaddr); 
			continue;
		}

		//파일 이름 길이만큼 파일 이름 받기 (실패 시 소켓 닫기)
		retval = recv(client_sock, buf, sizeof(char) * fInfo.nameLen, MSG_WAITALL);
		if (retval == SOCKET_ERROR)
		{
			printf("ERROR::recv()");
			stopConnect(client_sock, addr, clientaddr);
			continue;
		}
		buf[fInfo.nameLen] = '\0';
		printf(
			"[TCP 서버] 받은 파일 이름: %s\n"
			"[TCP 서버] 받은 파일 이름 길이 : % d\n"
			"[TCP 서버] 받은 파일 크기: %d\n", 
			buf, (int)fInfo.nameLen, fInfo.fileSize
		);

		//바이너리 쓰기 모드 파일 생성
		FILE* fp = fopen(buf, "wb");
		//파일 오픈 실패 시 소켓 닫기
		if (fp == NULL)
		{
			printf("파일을 열 수 없습니다.\n");
		}
		else
		{
			QueryPerformanceCounter(&StartTime);
			printf("\n");
			while (1)
			{
				//남은 데이터 용량 계산
				int remaining = fInfo.fileSize - curRecved;
				//BUFSIZE보다 데이터가 적게 남았을 경우 남은 만큼만 받기
				int recvLen = remaining < BUFSIZE ? remaining : BUFSIZE;

				//데이터를 버퍼에 저장
				retval = recv(client_sock, buf, recvLen, MSG_WAITALL);

				//에러 시 수신 중단
				if (retval == SOCKET_ERROR) { printf("ERROR::recv()");	break; }
				//수신 횟수 10000 이상 또는 데이터 수신이 없는 경우 전송률 갱신(printf 호출이 많아질수록 성능이 저하되므로 제한)
				else if (++counter >= 10000 * (4096 / BUFSIZE) || retval == 0)
				{
					printf("\033[1A\033[2K");
					printf("파일 전송률 : [%.2f%%], %d / %d Bytes\n", (float)curRecved / (float)fInfo.fileSize * 100, curRecved, fInfo.fileSize);
					counter = 0;
					//데이터 수신 없는 경우 break
					if (retval == 0) break;
				}

				//파일에 데이터 작성
				fwrite(buf, 1, retval, fp);
				//수신 횟수 갱신
				curRecved += retval;
			}
			QueryPerformanceCounter(&EndTime);
			printf("파일 수신 완료. 소요 시간: %.2lf초\n", (EndTime.QuadPart - StartTime.QuadPart) / (double)Frequency.QuadPart);
			fclose(fp);
		}

		// 소켓 닫기
		closesocket(client_sock);
		printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));
	}

	// 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}