#include "Common.h"

int main(int argc, char* argv[])
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	/*----------------*/
	/* IPv4 변환 연습 */
	/*----------------*/
	// 원래의 IPv4 주소 출력
	const char* ipv4test = "147.46.114.70";
	printf("IPv4 주소(변환 전) = %s\n", ipv4test);

	// inet_pton() 함수 연습
	struct in_addr ipv4num;
	//inet_pton(AF_INET, ipv4test, &ipv4num);
	ipv4num.s_addr = inet_addr(ipv4test);
	printf("IPv4 주소(변환 후) = %#x\n", ipv4num.s_addr);

	// inet_ntop() 함수 연습
	//char ipv4str[INET_ADDRSTRLEN];
	//inet_ntop(AF_INET, &ipv4num, ipv4str, sizeof(ipv4str));
	char* ipv4str = nullptr;
	ipv4str = inet_ntoa(ipv4num);
	printf("IPv4 주소(다시 변환 후) = %s\n", ipv4str);
	printf("\n");

	/*----------------*/
	/* IPv6 변환 연습 */
	/*----------------*/
	// 원래의 IPv6 주소 출력
	//const char* ipv6test = "2001:0230:abcd:ffab:0023:eb00:ffff:1111";
	wchar_t ipv6test[] = L"2001:0230:abcd:ffab:0023:eb00:ffff:1111";
	printf("IPv6 주소(변환 전) = %ws\n", ipv6test);

	// inet_pton() 함수 연습
	//struct in6_addr ipv6num;
	//inet_pton(AF_INET6, ipv6test, &ipv6num);
	struct sockaddr_in6 ipv6num;
	WSAStringToAddress(ipv6test, AF_INET6, NULL, (LPSOCKADDR)&ipv6num, (LPINT)sizeof(SOCKADDR_IN6));
	printf("IPv6 주소(변환 후) = 0x");
	for (int i = 0; i < 16; i++)
		printf("%02x", ipv6num.sin6_addr.s6_addr[i]);
	printf("\n");

	// inet_ntop() 함수 연습
	char ipv6str[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &ipv6num, ipv6str, sizeof(ipv6str));
	printf("IPv6 주소(다시 변환 후) = %s\n", ipv6str);

	// 윈속 종료
	WSACleanup();
	return 0;
}