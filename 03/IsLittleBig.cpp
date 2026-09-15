#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
#define _WINSOCK_DEPRECATED_NO_WARNINGS // 구형 소켓 API 사용 시 경고 끄기

#include <winsock2.h> // 윈속2 메인 헤더

#include <stdio.h> // printf(), ...

#pragma comment(lib, "ws2_32") // ws2_32.lib 링크

bool IsLittleEndian()
{
	u_short temp = 0x1234;
	if (temp != htons(temp)) {
		printf("The Host uses Little-Endian order\n");
		return true;
	}
	return false;
}

bool IsBigEndian()
{
	u_short temp = 0x1234;
	if (temp == htons(temp)) {
		printf("The Host uses Big-Endian order\n");
		return true;
	}
	return false;
}

int main(int argc, char* argv[])	
{
	IsLittleEndian();
	IsBigEndian();

	return 0;
}
