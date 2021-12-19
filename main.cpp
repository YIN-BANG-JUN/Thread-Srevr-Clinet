//服务端
//多线程+socket编程的一个联合使用
//用互斥体进行线程同步  socket编程   临界区   全局变量

#include <WinSock2.h>
#include <iostream>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define MAX_CLNT 256
#define MAX_BUF_SIZE 256


SOCKET clntSocks[MAX_CLNT];  //所有的连接的客户端socket
HANDLE hMutex;
int clntCnt = 0;  //当前连接的数目

// 服务端的设计：
// 1 每来一个连接，服务端起一个线程（安排一个工人）维护
// 2 将收到的消息转发给所有的客户端
// 3 某个连接断开，需要处理断开的连接

//发送给所有的客户端
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


//处理客户端连接的函数
unsigned WINAPI HandleCln(void* arg)
{
	//1 接收传递过来的参数
	SOCKET hClntSock = *((SOCKET*)arg);
	int iLen = 0, i;
	char szMsg[MAX_BUF_SIZE] = { 0 };
	//2 进行数据的收发  循环接收
	//接收到客户端的数据

//  	while ((iLen = recv(hClntSock, szMsg, sizeof(szMsg),0)) != 0)
//  	{ 		//收到的数据立马发给所有的客户端
//  		SendMsg(szMsg, iLen);
//  	}

	while (1)
	{
		iLen = recv(hClntSock, szMsg, sizeof(szMsg), 0);
		if (iLen != -1)
		{
			//收到的数据立马发给所有的客户端
			SendMsg(szMsg, iLen);
		}
		else
		{
			break;
		}
	}


	printf("此时连接数目为 %d\n", clntCnt);

	//3 某个连接断开，需要处理断开的连接  遍历
	WaitForSingleObject(hMutex, INFINITE);
	for (i = 0; i < clntCnt; i++)
	{
		if (hClntSock == clntSocks[i])
		{
			//移位
			while (i++ < clntCnt)
			{
				clntSocks[i] = clntSocks[i + 1];
			}
			break;
		}
	}
	clntCnt--;  //当前连接数的一个自减
	printf("断开此时连接数目 %d", clntCnt);
	ReleaseMutex(hMutex);
	closesocket(hClntSock);
	return 0;

}


int main()
{
	// 加载套接字库
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	HANDLE hThread;
	wVersionRequested = MAKEWORD(1, 1);
	// 初始化套接字库
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
	//创建一个互斥对象
	hMutex = CreateMutex(NULL, FALSE, NULL);
	// 新建套接字
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN addrSrv;
	addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(9190);

	// 绑定套接字到本地IP地址，端口号9190
	if (bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR)) == SOCKET_ERROR)
	{
		printf("bind ERRORnum = %d\n", GetLastError());
		return -1;
	}

	// 开始监听
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
		// 接收客户连接  sockConn此时来的客户端连接
		SOCKET sockConn = accept(sockSrv, (SOCKADDR*)&addrCli, &len);

		//每来一个连接，服务端起一个线程（安排一个工人）维护客户端的连接
		//每来一个连接，全局数组应该加一个成员，最大连接数加1
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





//客户端
// 1  接收服务端的消息   安排一个工人 起一个线程接收消息
// 2 发送消息给服务端    安排一个工人 起一个线程发送消息
// 3 退出机制

#include <WinSock2.h>
#include <iostream>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define NAME_SIZE 32
#define BUF_SIZE 256

char szName[NAME_SIZE] = "[DEFAULT]";
char szMsg[BUF_SIZE];

//发送消息给服务端
unsigned WINAPI SendMsg(void* arg)
{
	//1 接收传递过来的参数
	SOCKET hClntSock = *((SOCKET*)arg);
	char szNameMsg[NAME_SIZE + BUF_SIZE];  //又有名字，又有消息
	//循环接收来自于控制台的消息
	while (1)
	{
		fgets(szMsg, BUF_SIZE, stdin); //阻塞在这一句
		//退出机制  当收到q或Q  退出
		if (!strcmp(szMsg, "Q\n") || !strcmp(szMsg, "q\n"))
		{
			closesocket(hClntSock);
			exit(0);
		}

		sprintf(szNameMsg, "%s %s", szName, szMsg);//字符串拼接
		send(hClntSock, szNameMsg, strlen(szNameMsg), 0);//发送
	}
	return 0;
}

//接收服务端的消息
unsigned WINAPI RecvMsg(void* arg)
{
	//1 接收传递过来的参数
	SOCKET hClntSock = *((SOCKET*)arg);
	char szNameMsg[NAME_SIZE + BUF_SIZE];  //又有名字，又有消息
	int iLen = 0;
	while (1)
	{
		//recv阻塞
		iLen = recv(hClntSock, szNameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
		//服务端断开
		if (iLen == -1)
		{
			return -1;
		}
		// szNameMsg的0到iLen -1 都是收到的数据 iLen个
		szNameMsg[iLen] = 0;
		//接收到的数据输出到控制台
		fputs(szNameMsg, stdout);
	}
	return 0;
}

// 带参数的main函数，用命令行启动  在当前目录按下shift + 鼠标右键 cmd
int main(int argc, char* argv[])
{
	// 加载套接字库
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
	SOCKET hSock;
	SOCKADDR_IN servAdr;
	HANDLE hSendThread, hRecvThread;
	wVersionRequested = MAKEWORD(1, 1);
	// 初始化套接字库
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
	//1 建立socket
	hSock = socket(PF_INET, SOCK_STREAM, 0);

	// 2 配置端口和地址
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	servAdr.sin_family = AF_INET;
	servAdr.sin_port = htons(9190);

	// 3 连接服务器
	if (connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
	{
		printf("connect error error code = %d\n", GetLastError());
		return -1;
	}

	// 4  发送服务端的消息   安排一个工人 起一个线程发送消息

	hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendMsg,
		(void*)&hSock, 0, NULL);

	//  5 接收消息给服务端    安排一个工人 起一个线程接收消息

	hRecvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg,
		(void*)&hSock, 0, NULL);

	//等待内核对象的信号发生变化
	WaitForSingleObject(hSendThread, INFINITE);
	WaitForSingleObject(hRecvThread, INFINITE);

	// 6 关闭套接字
	closesocket(hSock);
	WSACleanup();
	return 0;
}