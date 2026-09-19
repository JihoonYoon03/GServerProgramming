#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib, "ws2_32")

int main(int argc, char** argv)
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

	struct hostent* ptr = gethostbyname(argv[argc > 1 ? 1 : 0]);
	if (ptr == NULL) return 1;

	printf("[Domain Name]\n%s\n", ptr->h_name);

	printf("\n[Aliases]\n");
	for (int i = 0; ptr->h_aliases[i] != NULL; ++i)
	{
		printf("#%d: %s\n", i, ptr->h_aliases[i]);
	}

	printf("\n[IPv4 Addresses]\n");
	char ipv4str[INET_ADDRSTRLEN];
	for (int i = 0; ptr->h_addr_list[i] != NULL; ++i)
	{
		inet_ntop(AF_INET, ptr->h_addr_list[i], ipv4str, sizeof(ipv4str));
		printf("#%d : %s\n", i, ipv4str);
	}

	if (WSACleanup() == SOCKET_ERROR) return 1;

	return 0;
}