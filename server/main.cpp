#include "Client.h"
#include <iostream>
#include <process.h>
#include <vector>
FD_SET g_ClientSocket;   //client socket ����

vector<Client> g_vClient;  //����ͻ�����Ϣ���� 

/*
@function OpenTCPServer             ��TCP������
@param  _Out_ SOCKET* sServer       �ͻ����׽���
@param _In_ unsigned short Port     �������˿�
@param  _Out_ DWORD* dwError               �������
@return  �ɹ�����TRUE ʧ�ܷ���FALSE
*/
BOOL OpenTCPServer(_Out_ SOCKET* sServer, _In_ unsigned short Port, _Out_ DWORD* dwError)
{
	BOOL bRet = FALSE;
	WSADATA wsaData = { 0 };
	SOCKADDR_IN ServerAddr = { 0 };
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(Port);
	ServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	do
	{
		if (!WSAStartup(MAKEWORD(2, 2), &wsaData))
		{
			if (LOBYTE(wsaData.wVersion) == 2 || HIBYTE(wsaData.wVersion) == 2)
			{
				*sServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (*sServer != INVALID_SOCKET)
				{
					if (SOCKET_ERROR != bind(*sServer, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
					{
						if (SOCKET_ERROR != listen(*sServer, SOMAXCONN))
						{
							bRet = TRUE;
							break;
						}
						*dwError = WSAGetLastError();
						closesocket(*sServer);
					}
					*dwError = WSAGetLastError();
					closesocket(*sServer);
				}
				*dwError = WSAGetLastError();
			}
			*dwError = WSAGetLastError();
		
		}
		*dwError = WSAGetLastError();
	} while (FALSE);
	return bRet;
}

//���ܿͻ�������
BOOL AcceptClient(SOCKET sServer)
{
	FD_ZERO(&g_ClientSocket);   //��ʼ��FD_SET
	SOCKADDR_IN addrClient = { 0 };
	int iaddrLen = sizeof(addrClient);
	int iclientNum = 0;
	while (iclientNum++ < FD_SETSIZE) //����FD_SETSIZE��64�����������SIZE�Ļ������Զ���һ��FD_SET����
	{
		SOCKET clientSocket = accept(sServer, (SOCKADDR*)&addrClient, &iaddrLen);
		if(clientSocket == INVALID_SOCKET)
			continue;
		FD_SET(clientSocket, &g_ClientSocket);  //���µ�client��ӵ�������
		
		pClientInfo pclientInfo = new ClientInfo;                //����ͻ�����Ϣ
		pclientInfo->sClient = clientSocket;
		pclientInfo->usPort = addrClient.sin_port;
		memcpy(pclientInfo->szIP ,inet_ntoa(addrClient.sin_addr),sizeof(pclientInfo->szIP));
		Client client;                          //���ÿͻ�����Ϣ  
		client.SetClientInfo(pclientInfo); 
		g_vClient.push_back(client);            //��ӵ�������
	}
	return TRUE;
}

//����socket�����û�
int FindClient(SOCKET s)
{
	for (int i =0;i<g_vClient.size();i++)
	{
		if (g_vClient[i].GetSocket() == s)
			return i;
	}
	return -1;
}

//����name�����û�
SOCKET FindClient(string name)
{
	for (int i = 0;i < g_vClient.size();i++)
	{
		if (g_vClient[i].GetUserName() == name)
			return g_vClient[i].GetSocket();
	}
	return INVALID_SOCKET;
}

//���ܿͻ��������߳�
unsigned int __stdcall ThreadAccept(void* lparam)
{
	AcceptClient(*(SOCKET*)lparam);   
	return 0;
}

//���������߳�
unsigned int __stdcall ThreadSend(void* lparam)
{
	g_vClient[*(int*)lparam].SendPacket();
	return 0;
}

//���������߳�
unsigned int __stdcall ThreadRecv(void* lparam)
{
	FD_SET fdRead;      //�ɶ�socket����
	FD_ZERO(&fdRead);   //��ʼ���ɶ�socket����
	int iRet = 0;
	char* buf = new char[PACKETSIZE];  //����ռ�
	TIMEVAL tvl = { 0 };
	tvl.tv_sec = 2;  //ֻ�ȴ�2��  
	while (true)
	{
		fdRead = g_ClientSocket;     //�������fdRead�У�fd_count=2,��select��fd_count�͵���1�ˣ���ʾֻ��һ���ɶ�
		if (fdRead.fd_count == 0)
		{
			Sleep(500);
			continue;
		}	        //���ﲻ����Ϊ���޵ȴ�,��Ϊһ��������ԭ״̬��������µ�client�ӽ������Ͳ��ܼ�ʱ����fdRead��
		iRet = select(0, &fdRead, NULL, NULL, &tvl);  //����׼����������������������ʱ�򷵻�0���������򷵻�-1
		if (iRet != SOCKET_ERROR)
		{
			for (int i = 0; i < g_ClientSocket.fd_count; i++) //��������socket
			{
				if (FD_ISSET(g_ClientSocket.fd_array[i], &fdRead))  //���socket�ڿɶ�socket�ڣ�����Խ��ж�ȡ
				{
					memset(buf, 0, PACKETSIZE);
					iRet = recv(g_ClientSocket.fd_array[i], buf, PACKETSIZE, 0);
					if (iRet == SOCKET_ERROR)
					{
						closesocket(g_ClientSocket.fd_array[i]);
						FD_CLR(g_ClientSocket.fd_array[i], &g_ClientSocket);
					}
					else  //�����ݣ�����
					{
						string str = buf;
						if (str.size() > 0)     //��buf���Ե���������
						{
							int iClient = FindClient(g_ClientSocket.fd_array[i]);  //�ҵ���Ӧ���û�
							if (iClient == -1)
								break;
							if (str[0] == '#')  //�û���
							{
								g_vClient[iClient].SetUserName(&buf[1]);
								cout << &buf[1] <<" is online.IP:"<< g_vClient[iClient] .GetIP()<<",Port:"<< g_vClient[iClient].GetPort()<< endl;
							}		
							else if(str[0] == '@') //chat name
								g_vClient[iClient].SetChatName(&buf[1]);
							else  //�������ݣ����������
							{
								Packet packet;
								packet.sClient = FindClient(g_vClient[iClient].GetChatName());
								memcpy(packet.szUserName, g_vClient[iClient].GetUserName().c_str(), sizeof(packet.szUserName));
								memcpy(packet.szChatName, g_vClient[iClient].GetChatName().c_str(), sizeof(packet.szChatName));
								memcpy(packet.szMessage, buf, MAXSIZE);
								g_vClient[iClient].SetPacket(packet);   //�������ݰ�
								_beginthreadex(NULL, 0, ThreadSend, &iClient, 0, NULL); //��������
							}
						}
					}
				}
			}
		}
	}

	if (buf)
		delete buf;
	return 0;
}

//client�����߳�
unsigned int __stdcall ThreadManager(void* lparam)
{
	while (1)
	{
		vector<Client>::iterator iter = g_vClient.begin();
		while (iter != g_vClient.end())
		{
			if (SOCKET_ERROR == send(iter->GetSocket(), "", sizeof(""), 0))
			{
				cout << iter->GetUserName() << " is downline." << endl;
				FD_CLR(iter->GetSocket(), &g_ClientSocket);  //�Ӽ�����ɾ������Ч�׽���
				g_vClient.erase(iter);
				break;
			}
			iter++;
		}
		Sleep(2000);
	}
	return 0;
}

int main()
{
	SOCKET sServer = INVALID_SOCKET;
	DWORD dwError = 0;
	USHORT uPort = 18000;
	if (OpenTCPServer(&sServer, uPort, &dwError)) //�򿪷�����
	{
		_beginthreadex(NULL, 0, ThreadAccept, &sServer, 0, NULL);  //���������߳�
		_beginthreadex(NULL, 0, ThreadRecv, NULL, 0, NULL);
		_beginthreadex(NULL, 0, ThreadManager, NULL, 0, NULL);
	}
	Sleep(100000000); //������˯��
	return 0;
}