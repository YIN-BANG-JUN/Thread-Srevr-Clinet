//�����
//���߳�+socket��̵�һ������ʹ��
//�û���������߳�ͬ��  socket���   �ٽ���   ȫ�ֱ���

#include <WinSock2.h>
#include <iostream>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLNT 256
#define MAX_BUF_SIZE 256


SOCKET clntSocks[MAX_CLNT];  //���е����ӵĿͻ���socket
HANDLE hMutex;
int clntCnt = 0;  //��ǰ���ӵ���Ŀ

// ����˵���ƣ�
// 1 ÿ��һ�����ӣ��������һ���̣߳�����һ�����ˣ�ά��
// 2 ���յ�����Ϣת�������еĿͻ���
// 3 ĳ�����ӶϿ�����Ҫ����Ͽ�������

//���͸����еĿͻ���
void SendMsg(char* szMsg, int iLen)
{
	int i = 0;
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntCnt; i++)
	{
		send(clntSocks[i], szMsg, iLen, 0);
	}
	ReleaseMutex(hMutex);
}


//����ͻ������ӵĺ���
unsigned WINAPI HandleCln(void* arg)
{
	//1 ���մ��ݹ����Ĳ���
	SOCKET hClntSock = *((SOCKET*)arg);
	int iLen = 0, i;
	char szMsg[MAX_BUF_SIZE] = { 0 };
	//2 �������ݵ��շ�  ѭ������
	//���յ��ͻ��˵�����

//  	while ((iLen = recv(hClntSock, szMsg, sizeof(szMsg),0)) != 0)
//  	{ 		//�յ����������������еĿͻ���
//  		SendMsg(szMsg, iLen);
//  	}

	while (1)
	{
		iLen = recv(hClntSock, szMsg, sizeof(szMsg), 0);
		if (iLen != -1)
		{
			//�յ����������������еĿͻ���
			SendMsg(szMsg, iLen);
		}
		else
		{
			break;
		}
	}


	printf("��ʱ������ĿΪ %d\n", clntCnt);

	//3 ĳ�����ӶϿ�����Ҫ����Ͽ�������  ����
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntCnt; i++)
	{
		if (hClntSock == clntSocks[i])
		{
			//��λ
			while (i++ < clntCnt)
			{
				clntSocks[i] = clntSocks[i + 1];
			}
			break;
		}
	}
	clntCnt--;  //��ǰ��������һ���Լ�
	printf("�Ͽ���ʱ������Ŀ %d", clntCnt);
	ReleaseMutex(hMutex);
	closesocket(hClntSock);
	return 0;

}


int main()
{
	// �����׽��ֿ�
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	HANDLE hThread;
	wVersionRequested = MAKEWORD(1, 1);
	// ��ʼ���׽��ֿ�
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		return err;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return -1;
	}
	//����һ���������
	hMutex = CreateMutex(NULL, FALSE, NULL);
	// �½��׽���
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(9190);

	// ���׽��ֵ�����IP��ַ���˿ں�9190
	if (bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		printf("bind ERRORnum = %d\n", GetLastError());
		return -1;
	}

	// ��ʼ����
	if (listen(sockSrv, 5) == SOCKET_ERROR)
	{
		printf("listen ERRORnum = %d\n", GetLastError());
		return -1;
	}

	printf("start listen\n");

	SOCKADDR_IN addrCli;
	int len = sizeof(SOCKADDR);

	while (1)
	{
		// ���տͻ�����  sockConn��ʱ���Ŀͻ�������
		SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrCli, &len);

		//ÿ��һ�����ӣ��������һ���̣߳�����һ�����ˣ�ά���ͻ��˵�����
		//ÿ��һ�����ӣ�ȫ������Ӧ�ü�һ����Ա�������������1
		WaitForSingleObject(hMutex, INFINITE);
		clntSocks[clntCnt++] = sockConn;
		ReleaseMutex(hMutex);

		hThread = (HANDLE)_beginthreadex(NULL, 0, HandleCln,
			(void*)&sockConn, 0, NULL);
		printf("Connect client IP: %s \n", inet_ntoa(addrCli.sin_addr));
		printf("Connect client num: %d \n", clntCnt);
	}

	closesocket(sockSrv);
	WSACleanup();

	return 0;
}





//�ͻ���
// 1  ���շ���˵���Ϣ   ����һ������ ��һ���߳̽�����Ϣ
// 2 ������Ϣ�������    ����һ������ ��һ���̷߳�����Ϣ
// 3 �˳�����

#include <WinSock2.h>
#include <iostream>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define NAME_SIZE 32
#define BUF_SIZE 256

char szName[NAME_SIZE] = "[DEFAULT]";
char szMsg[BUF_SIZE];

//������Ϣ�������
unsigned WINAPI SendMsg(void* arg)
{
	//1 ���մ��ݹ����Ĳ���
	SOCKET hClntSock = *((SOCKET*)arg);
	char szNameMsg[NAME_SIZE + BUF_SIZE];  //�������֣�������Ϣ
	//ѭ�����������ڿ���̨����Ϣ
	while (1)
	{
		fgets(szMsg, BUF_SIZE, stdin); //��������һ��
		//�˳�����  ���յ�q��Q  �˳�
		if (!strcmp(szMsg, "Q\n") || !strcmp(szMsg, "q\n"))
		{
			closesocket(hClntSock);
			exit(0);
		}

		sprintf(szNameMsg, "%s %s", szName, szMsg);//�ַ���ƴ��
		send(hClntSock, szNameMsg, strlen(szNameMsg), 0);//����
	}
	return 0;
}

//���շ���˵���Ϣ
unsigned WINAPI RecvMsg(void* arg)
{
	//1 ���մ��ݹ����Ĳ���
	SOCKET hClntSock = *((SOCKET*)arg);
	char szNameMsg[NAME_SIZE + BUF_SIZE];  //�������֣�������Ϣ
	int iLen = 0;
	while (1)
	{
		//recv����
		iLen = recv(hClntSock, szNameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
		//����˶Ͽ�
		if (iLen == -1)
		{
			return -1;
		}
		// szNameMsg��0��iLen -1 �����յ������� iLen��
		szNameMsg[iLen] = 0;
		//���յ����������������̨
		fputs(szNameMsg, stdout);
	}
	return 0;
}

// ��������main������������������  �ڵ�ǰĿ¼����shift + ����Ҽ� cmd
int main(int argc, char* argv[])
{
	// �����׽��ֿ�
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	SOCKET hSock;
	SOCKADDR_IN servAdr;
	HANDLE hSendThread, hRecvThread;
	wVersionRequested = MAKEWORD(1, 1);
	// ��ʼ���׽��ֿ�
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		return err;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		return -1;
	}
	sprintf(szName, "[%s]", argv[1]);
	//1 ����socket
	hSock = socket(PF_INET, SOCK_STREAM, 0);

	// 2 ���ö˿ں͵�ַ
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	servAdr.sin_family = AF_INET;
	servAdr.sin_port = htons(9190);

	// 3 ���ӷ�����
	if (connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
	{
		printf("connect error error code = %d\n", GetLastError());
		return -1;
	}

	// 4  ���ͷ���˵���Ϣ   ����һ������ ��һ���̷߳�����Ϣ

	hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendMsg,
		(void*)&hSock, 0, NULL);

	//  5 ������Ϣ�������    ����һ������ ��һ���߳̽�����Ϣ

	hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg,
		(void*)&hSock, 0, NULL);

	//�ȴ��ں˶�����źŷ����仯
	WaitForSingleObject(hSendThread, INFINITE);
	WaitForSingleObject(hRecvThread, INFINITE);

	// 6 �ر��׽���
	closesocket(hSock);
	WSACleanup();
	return 0;
}