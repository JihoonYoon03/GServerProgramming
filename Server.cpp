#include "Common.h"

int f(int x)
{
	if (x >= 0) {
		WSASetLastError(0);
		return 0;
	}
	else {
		WSASetLastError(WSAEINVAL);
		return SOCKET_ERROR;
	}
}

int main(int argc, char* argv[])
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;
	printf("[알림] 윈속 초기화 성공\n");
	
	//연습1번
	printf("주 버전: %d, 부 버전: %d, 소켓 정보: %s, 시스템 상태: %s\n", LOBYTE(wsa.wVersion), HIBYTE(wsa.wHighVersion), wsa.szDescription, wsa.szSystemStatus);

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");
	printf("[알림] 소켓 생성 성공\n");
	
	//연습2번
	SOCKET sockUDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockUDP == INVALID_SOCKET) err_quit("socket()");
	printf("[알림] UDP 소켓 생성 성공\n");

	//연습3번
	SOCKET sockIPv6TCP = socket(AF_INET6, SOCK_STREAM, 0);
	if (sockIPv6TCP == INVALID_SOCKET) err_quit("socket()");
	printf("[알림] IPv6 TCP 소켓 생성 성공\n");

	//연습4번
	SOCKET sockIPv6UDP = socket(AF_INET6, SOCK_DGRAM, 0);
	if (sockIPv6UDP == INVALID_SOCKET) err_quit("socket()");
	printf("[알림] IPv6 UDP 소켓 생성 성공\n");

	//연습6번
	SOCKET sockTCP2 = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
	if (sockTCP2 == INVALID_SOCKET) err_quit("socket()");
	printf("[알림] TCP 소켓 생성(WSASocket) 성공\n");

	SOCKET sockUDP2 = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, 0);
	if (sockUDP2 == INVALID_SOCKET) err_quit("socket()");
	printf("[알림] UDP 소켓 생성(WSASocket) 성공\n");

	// 소켓 닫기
	if (closesocket(sock) == SOCKET_ERROR)
		err_quit("closesocket()");

	//연습2번
	if (closesocket(sockUDP) == SOCKET_ERROR)
		err_quit("closesocket()");

	//연습3번
	if (closesocket(sockIPv6TCP) == SOCKET_ERROR)
		err_quit("closesocket()");

	//연습4번
	if (closesocket(sockIPv6UDP) == SOCKET_ERROR)
		err_quit("closesocket()");

	//연습5번
	int retval = f(0);
	if (retval == SOCKET_ERROR) err_quit("f()");

	//연습6번
	if (closesocket(sockTCP2) == SOCKET_ERROR)
		err_quit("closesocket()");

	if (closesocket(sockUDP2) == SOCKET_ERROR)
		err_quit("closesocket()");

	// 윈속 종료
	if (WSACleanup() == SOCKET_ERROR)
		err_quit("WSACleanup()");
	return 0;
}