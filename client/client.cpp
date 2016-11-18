#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#pragma comment(lib,"ws2_32.lib")
char userName[16] = { 0 };
char chatName[16] = { 0 };
//��������
unsigned __stdcall ThreadRecv(void* param)
{
	char buf[128] = { 0 };
	while (1)
	{
		memset(buf, 0, sizeof(buf));
		int ret = recv(*(SOCKET*)param, buf, sizeof(buf), 0);
		if (ret == SOCKET_ERROR)
		{
			Sleep(500);
			continue;
		}
		if (strlen(buf) != 0)
			printf("%s\n", buf);
		Sleep(100);
	}
	return 0;
}

//��������
unsigned __stdcall ThreadSend(void* param)
{
	char buf[128] = { 0 };
	int ret = 0;
	while (1)
	{
		int c = getch();
		if (c == 27)   //ESC ASCII��27
		{
			memset(buf, 0, sizeof(buf));
			printf("Please input the chat name:");
			gets_s(buf);
			char b[17] = { 0 };
			sprintf(b, "@%s", buf);
			ret = send(*(SOCKET*)param, b, sizeof(b), 0);
			if (ret == SOCKET_ERROR)
				return 1;
			continue;
		}
		if (c == 72 || c == 0 || c == 68)//Ϊ����ʾ���ۣ���һ���޻��ԵĶ�ȡ�ַ�����
			continue;                   //getch����ֵ���Ǿ���ʵ��ó�����Ƿ����⼸��ֵ����getch�ͻ��Զ�������������Ҳ������
		printf("%s: ", userName);
		gets_s(buf);
		ret = send(*(SOCKET*)param, buf, sizeof(buf), 0);
		if (ret == SOCKET_ERROR)
			return 1;
	}
	return 0;
}

//���ӷ�����
int ConnectServer()
{
	WSADATA wsaData = { 0 };//����׽�����Ϣ
	SOCKET ClientSocket = INVALID_SOCKET;//�ͻ����׽���
	SOCKADDR_IN ServerAddr = { 0 };//����˵�ַ
	USHORT uPort = 18000;//����˶˿�
						 //��ʼ���׽���
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		printf("WSAStartup failed with error code: %d\n", WSAGetLastError());
		return -1;
	}
	//�ж��׽��ְ汾
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
	{
		printf("wVersion was not 2.2\n");
		return -1;
	}
	//�����׽���
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET)
	{
		printf("socket failed with error code: %d\n", WSAGetLastError());
		return -1;
	}
	//���������IP
	printf("Please input server IP:");
	char IP[32] = { 0 };
	gets_s(IP);
	//���÷�������ַ
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(uPort);//�������˿�
	ServerAddr.sin_addr.S_un.S_addr = inet_addr(IP);//��������ַ

	printf("connecting......\n");
	//���ӷ�����
	if (SOCKET_ERROR == connect(ClientSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
	{
		printf("connect failed with error code: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}
	printf("Connecting server successfully IP:%s Port:%d\n",
		IP, htons(ServerAddr.sin_port));
	printf("Please input your UserName: ");
	char b[16] = { 0 };
	gets_s(userName);
	sprintf(b, "#%s", userName);
	send(ClientSocket, b, sizeof(b), 0);
	printf("Please input the ChatName: ");
	memset(b, 0, sizeof(b));
	gets_s(chatName);
	sprintf(b, "@%s", chatName);
	send(ClientSocket, b, sizeof(b), 0);
	printf("\n\n");

	_beginthreadex(NULL, 0, ThreadRecv, &ClientSocket, 0, NULL); //�������պͷ�����Ϣ�߳�
	_beginthreadex(NULL, 0, ThreadSend, &ClientSocket, 0, NULL);
	for (int k = 0;k < 1000;k++)
		Sleep(10000000);
	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}

int main()
{
	ConnectServer(); //���ӷ�����
	return 0;
}