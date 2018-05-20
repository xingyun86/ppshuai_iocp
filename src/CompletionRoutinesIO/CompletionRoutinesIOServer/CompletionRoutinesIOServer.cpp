#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <mswsock.h>    //΢����չ�����
#include <stdio.h>

// Need to link with ws2_32.lib
#pragma comment(lib, "ws2_32.lib")

#include <tchar.h>
#include <stdio.h>

#define DATA_BUFSIZE			16384  
#define SERVER_PORT				6000

#define MAX_SERVER_ACCEPT_NUM	25600

#define IO_MIN	  0x00
#define IO_READ   0x01  
#define IO_WRITE  0x02
#define IO_MAX	  0x88
#define IO_ACCEPT 0xAA

//��չ����������ṹ  
typedef struct _PER_IO_OPERATION_DATA
{
	OVERLAPPED  overlapped;
	WSABUF      databuf;
	CHAR        buffer[DATA_BUFSIZE];
	BYTE        type;
	DWORD       len;
	SOCKET      sock;
}PER_IO_OPERATION_DATA, *LP_PER_IO_OPERATION_DATA;

//��ɼ�  
typedef struct _PER_HANDLE_DATA
{
	SOCKET		sock;
	SOCKADDR_IN	sockaddrin;//�ͻ��˵�������Ϣ
}PER_HANDLE_DATA, *LP_PER_HANDLE_DATA;

//////////////////////////////////////////////////  
LPFN_ACCEPTEX lpfnAcceptEx = NULL;                   //AcceptEx����ָ��  
LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockaddrs = NULL;  //����GetAcceptExSockaddrs����ָ��  
BOOL G_bRunning = FALSE;
SOCKET G_sListen = INVALID_SOCKET;
HANDLE G_hIOCompletionPort = NULL;

DWORD WSAInit(WSAData * pwsadata = (0))
{
	WSADATA wsadata = { 0 };
	WSAData * pwsad = pwsadata ? pwsadata : &wsadata;
	
	if (WSAStartup(MAKEWORD(2, 2), pwsad) != 0)
	{
		printf("WSAStartup failed!\r\n");
		return (-1L);
	}

	if (LOBYTE(pwsad->wVersion) != 2 || HIBYTE(pwsad->wVersion) != 2)
	{
		printf("Socket version failed!\r\n");
		WSACleanup();
		return (-1L);
	}
	return (0L);
}

BOOL AcceptClient(SOCKET sListen)
{
	BOOL bResult = TRUE;
	DWORD dwBytes = (0L);

	LP_PER_IO_OPERATION_DATA lpPerIOOperationData = NULL;
	lpPerIOOperationData = (LP_PER_IO_OPERATION_DATA)GlobalAlloc(GPTR, sizeof(PER_IO_OPERATION_DATA));
	lpPerIOOperationData->databuf.buf = lpPerIOOperationData->buffer;
	lpPerIOOperationData->databuf.len = lpPerIOOperationData->len = DATA_BUFSIZE;
	lpPerIOOperationData->type = IO_ACCEPT;

	//�ȴ���һ���׽���(���accept�е���ڴ�,accept�ǽ��յ����ӲŴ��������׽���,�˷�ʱ��. ������׼��һ��,���ڽ�������)  
	lpPerIOOperationData->sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

	//����AcceptEx��������ַ������Ҫ��ԭ�е��������16���ֽ�  
	//������߳�Ͷ��һ���������ӵĵ�����  
	if (!lpfnAcceptEx(sListen, lpPerIOOperationData->sock,
		lpPerIOOperationData->buffer, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		&dwBytes, &(lpPerIOOperationData->overlapped)))
	{
		if (WSAGetLastError() != ERROR_IO_PENDING)
		{
			printf("%d", WSAGetLastError());
			bResult = FALSE;
		}
	}

	return bResult;
}

BOOL IORecv(LP_PER_HANDLE_DATA lpPerHandleData, LP_PER_IO_OPERATION_DATA lpPerIOOperationData)
{
	BOOL bResult = TRUE;
	DWORD dwFlags = (0L);
	DWORD dwRecvBytes = (0L);
	ZeroMemory(&lpPerIOOperationData->overlapped, sizeof(OVERLAPPED));

	lpPerIOOperationData->type = IO_READ;
	lpPerIOOperationData->databuf.buf = lpPerIOOperationData->buffer;
	lpPerIOOperationData->databuf.len = lpPerIOOperationData->len = DATA_BUFSIZE;

	if (SOCKET_ERROR == WSARecv(lpPerHandleData->sock, &lpPerIOOperationData->databuf, 1, &dwRecvBytes, &dwFlags, &lpPerIOOperationData->overlapped, NULL))
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			printf("�����ص�����ʧ��!   %d\n", GetLastError());
			bResult = FALSE;
		}
	}

	return bResult;
}

BOOL IOSend(LP_PER_HANDLE_DATA lpPerHandleData, LP_PER_IO_OPERATION_DATA lpPerIOOperationData)
{
	BOOL bResult = TRUE;
	DWORD dwFlags = (0L);
	DWORD dwSendBytes = (0L);
	ZeroMemory(&lpPerIOOperationData->overlapped, sizeof(OVERLAPPED));

	lpPerIOOperationData->type = IO_WRITE;
	lpPerIOOperationData->databuf.len = DATA_BUFSIZE;

	if (SOCKET_ERROR == WSASend(lpPerHandleData->sock, &lpPerIOOperationData->databuf, 1, &dwSendBytes, dwFlags, &lpPerIOOperationData->overlapped, NULL))
	{
		if (ERROR_IO_PENDING != WSAGetLastError())
		{
			printf("�������ص�����ʧ��!\n");
			bResult = FALSE;
		}
	}

	return bResult;
}

BOOL IOOperationScheduler(LP_PER_HANDLE_DATA lpPerHandleData, LP_PER_IO_OPERATION_DATA lpPerIOOperationData)
{
	BOOL bResult = FALSE;
	switch (lpPerIOOperationData->type)
	{
	case IO_READ:
	{
		//���յ�����
		//�����յ�������ԭ�ⲻ���ķ��ͻ�ȥ
		bResult = IOSend(lpPerHandleData, lpPerIOOperationData);   
	}
	break;
	case IO_WRITE:
	{
		//������ղ���
		bResult = IORecv(lpPerHandleData, lpPerIOOperationData);
	}
	break;
	case IO_ACCEPT:
	{
		SOCKADDR_IN *psockaddrinLocal = NULL;
		SOCKADDR_IN *psockaddrinRemote = NULL;
		INT nLocalSize = sizeof(SOCKADDR_IN);
		INT nRemoteSize = sizeof(SOCKADDR_IN);
		LP_PER_HANDLE_DATA pClientPerHandleData = NULL;
						
		setsockopt(lpPerIOOperationData->sock, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const char *)&(lpPerHandleData->sock), sizeof(lpPerHandleData->sock));

		pClientPerHandleData = (LP_PER_HANDLE_DATA)GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA));
		pClientPerHandleData->sock = lpPerIOOperationData->sock;
		
		//ʹ��GetAcceptExSockaddrs���� ��þ���ĸ�����ַ����.  
		lpfnGetAcceptExSockaddrs(lpPerIOOperationData->buffer, 0,
			sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
			(LPSOCKADDR*)&psockaddrinLocal, &nLocalSize,
			(LPSOCKADDR*)&psockaddrinRemote, &nRemoteSize);

		memcpy(&pClientPerHandleData->sockaddrin, psockaddrinRemote, sizeof(SOCKADDR_IN));
		
		printf("Accept success!\n");
		printf("ClientIp: [%s:%d]\r\n", inet_ntoa(pClientPerHandleData->sockaddrin.sin_addr), pClientPerHandleData->sockaddrin.sin_port);

		//�����������׽��ֹ�������ɶ˿�  
		CreateIoCompletionPort((HANDLE)pClientPerHandleData->sock, G_hIOCompletionPort, (DWORD)pClientPerHandleData, 0);

		bResult = IORecv(pClientPerHandleData, lpPerIOOperationData);
		if (bResult)
		{
			//���յ�һ�����ӣ�����һ���첽������  
			bResult = AcceptClient(G_sListen);
		}
	}
	break;
	default:
	{

	}
	break;
	}

	return bResult;
}

DWORD WINAPI WorkerThread(LPVOID lpParameter)
{
	DWORD dwResult = (0L);
	DWORD dwIOOperationDataSize = (0L);
	LP_PER_HANDLE_DATA lpClientPerHandleData = NULL;//��ɼ�  
	LP_PER_IO_OPERATION_DATA  lpPerIOOperationData = NULL;//I/O����  
	HANDLE hIOCompletionPort = (HANDLE)lpParameter;
	
	//���ڷ�������ص�����  
	BOOL bQueuedCompletionStatus = FALSE;

	while (G_bRunning)
	{
		lpPerIOOperationData = NULL;
		lpClientPerHandleData = NULL;
		dwIOOperationDataSize = (-1L);
		bQueuedCompletionStatus = FALSE;

		bQueuedCompletionStatus = GetQueuedCompletionStatus(G_hIOCompletionPort, &dwIOOperationDataSize, (PULONG_PTR)&lpClientPerHandleData, (LPOVERLAPPED*)&lpPerIOOperationData, INFINITE);

		if (!bQueuedCompletionStatus)
		{
			DWORD dwIOError = GetLastError();
			if (WAIT_TIMEOUT == dwIOError)
			{
				continue;
			}
			else if (NULL != lpPerIOOperationData)
			{
				//ȡ���ȴ�ִ�е��첽����
				CancelIo((HANDLE)lpClientPerHandleData->sock);
				closesocket(lpClientPerHandleData->sock);
				GlobalFree(lpClientPerHandleData);
				lpClientPerHandleData = NULL;
			}
			else
			{
				G_bRunning = FALSE;
				break;
			}
		}
		else
		{
			if (!dwIOOperationDataSize && ((IO_MIN < lpPerIOOperationData->type) && (lpPerIOOperationData->type < IO_MAX)))
			{
				printf("�ͻ�[%s:%d]�Ͽ�������!\r\n", inet_ntoa(lpClientPerHandleData->sockaddrin.sin_addr), lpClientPerHandleData->sockaddrin.sin_port);
				
				//ȡ���ȴ�ִ�е��첽����
				CancelIo((HANDLE)lpClientPerHandleData->sock);
				closesocket(lpClientPerHandleData->sock);
				GlobalFree(lpClientPerHandleData);
				lpClientPerHandleData = NULL;

				GlobalFree(lpPerIOOperationData);
				lpPerIOOperationData = NULL;

				continue;
			}
			else
			{
				IOOperationScheduler(lpClientPerHandleData, lpPerIOOperationData);
			}
		}
	}

	return dwResult;
}

int _tmain(int argc, _TCHAR ** argv)
{
	int nResult = (0);
	INT nOptVal = (1);
	DWORD dwServerAcceptNumber = (0L);
	DWORD dwWorkerThreadNumber = (0L);
	HANDLE hIOCompletionPort = NULL;
	SOCKET sListen = INVALID_SOCKET;
	SYSTEM_INFO systeminfo = { 0 };
	HANDLE hThread = NULL;
	DWORD dwThreadId = (0L);
	SOCKADDR_IN sockadrdin = { 0 };
	//��ɼ�
	LP_PER_HANDLE_DATA lpPerHandleData = NULL;

	//������ɶ˿�
	G_hIOCompletionPort = hIOCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	//�׽��ֳ�ʼ��
	WSAInit();

	//�ȴ���һ���׽������ڼ���  
	G_sListen = sListen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	
	//��ȡCPU����������
	GetNativeSystemInfo(&systeminfo);
	
	//�趨�߳�����״̬
	G_bRunning = TRUE;
	
	// ����CPU����������������(dwNumberOfProcessors * 2 + 2)�Ĺ����߳� 
	dwWorkerThreadNumber = (systeminfo.dwNumberOfProcessors * 2 + 2);
	while ((dwWorkerThreadNumber--) > 0)
	{
		hThread = CreateThread(0, 0, WorkerThread, hIOCompletionPort, 0, &dwThreadId);
		if (hThread)
		{
			CloseHandle(hThread);
			hThread = NULL;
		}
	}

	//����socket�˿�Ϊ������
	setsockopt(sListen, SOL_SOCKET, SO_REUSEADDR, (const char *)&nOptVal, sizeof(nOptVal));

	//�������׽�������ɶ˿ڰ�	
	lpPerHandleData = (LP_PER_HANDLE_DATA)GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA));
	lpPerHandleData->sock = sListen;
	//�������׽�������ɶ˿ڰ�
	CreateIoCompletionPort((HANDLE)sListen, G_hIOCompletionPort, (DWORD)lpPerHandleData, 0);

	//�����׽��ְ󶨼���  
	sockadrdin.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	sockadrdin.sin_family = AF_INET;
	sockadrdin.sin_port = htons(SERVER_PORT);

	//�󶨼����׽���
	bind(sListen, (SOCKADDR*)&sockadrdin, sizeof(SOCKADDR));

	//�����׽��ּ���
	listen(sListen, SOMAXCONN);

	if (sListen == SOCKET_ERROR)
	{
		goto STOP_SERVER;
	}

	/////////////////////////////////////////////////////////////////////  
	//ʹ��WSAIoctl��ȡAcceptEx����ָ��  
	if (true)
	{
		DWORD dwBytes = (0L);
		//Accept function GUID  
		GUID guidAcceptEx = WSAID_ACCEPTEX;

		if (0 != WSAIoctl(sListen, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidAcceptEx, sizeof(guidAcceptEx),
			&lpfnAcceptEx, sizeof(lpfnAcceptEx),
			&dwBytes, NULL, NULL))
		{
		}
		
		// ��ȡGetAcceptExSockAddrs����ָ�룬Ҳ��ͬ��  
		GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
		if (0 != WSAIoctl(sListen, SIO_GET_EXTENSION_FUNCTION_POINTER,
			&guidGetAcceptExSockaddrs,
			sizeof(guidGetAcceptExSockaddrs),
			&lpfnGetAcceptExSockaddrs,
			sizeof(lpfnGetAcceptExSockaddrs),
			&dwBytes, NULL, NULL))
		{
		}
	}

	//������յ��첽����  
	dwServerAcceptNumber = MAX_SERVER_ACCEPT_NUM;
	while ((dwServerAcceptNumber--) > 0)
	{
		AcceptClient(sListen);
	}

	//�������߳��˳�  
	while (true)
	{
		Sleep(1000);
	}

STOP_SERVER:
	G_bRunning = FALSE;
	if (lpPerHandleData)
	{		
		closesocket(lpPerHandleData->sock);
		GlobalFree(lpPerHandleData);
		lpPerHandleData = NULL;
	}
	WSACleanup();

	return nResult;
}