#pragma once


#include <winsock2.h>
#include <mswsock.h>    //΢����չ�����
#include <malloc.h>
#include <stdio.h>

#include <psapi.h>
#pragma comment(lib,"psapi.lib")

// Need to link with ws2_32.lib
#pragma comment(lib, "mswsock.lib")
#pragma comment(lib, "ws2_32.lib")

#include <vector>

#include "ThreadHelper.h"

namespace IOCP {

#define MALLOC(S)			GlobalAlloc(GPTR,S)
#define FREE(H)				GlobalFree(H)
	typedef LONG IRES_TYPE;
	class CIocpCore
	{
		//��չ����������ṹ
		typedef struct _PER_IO_OPERATION_DATA {
			OVERLAPPED  o;
			WSABUF      d;
			CHAR        b[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
			BYTE        t;
			DWORD       l;
			SOCKET      s;
		}PER_IO_OPERATION_DATA, *LP_PER_IO_OPERATION_DATA;

		//��ɼ�
		typedef struct _PER_HANDLE_DATA {
			SOCKET		s;
			SOCKADDR_IN	i;//�ͻ��˵�������Ϣ
		}PER_HANDLE_DATA, *LP_PER_HANDLE_DATA;

		typedef enum _IO_TYPE {
			IOT_MINI = 0x00,
			IOT_SEND = 0x01,
			IOT_RECV = 0x02,
			IOT_MAXI = 0xAA,
			IOT_ACCEPT = 0xFF,
		}IO_TYPE;

	public:
		CIocpCore(USHORT nServerListenPort = 6000) : m_wsaData({ 0 }),
			m_nServerListenPort(nServerListenPort),
			m_lpfnAcceptEx(NULL),
			m_lpfnGetAcceptExSockaddrs(NULL),
			m_bRunning(FALSE),
			m_sListen(INVALID_SOCKET),
			m_hIOCP(NULL) {
		}
		~CIocpCore() {

		}

	public:
		BOOL NetworkIoStartup(WORD wVersion = MAKEWORD(2, 2))
		{
			return (WSAStartup(wVersion, &m_wsaData) == 0);
		}
		BOOL NetworkIoCleanup()
		{
			return (WSACleanup() == 0);
		}

		BOOL AcceptClient(SOCKET sListen)
		{
			int nError = 0;
			BOOL bResult = TRUE;
			DWORD dwBytesReceived = (0L);
			LP_PER_IO_OPERATION_DATA ppiod = NULL;
			DWORD dwAddrSize = sizeof(SOCKADDR_IN) + 0x10;

			ppiod = (LP_PER_IO_OPERATION_DATA)MALLOC(sizeof(PER_IO_OPERATION_DATA));
			if (ppiod)
			{
				ppiod->d.buf = ppiod->b;
				ppiod->d.len = ppiod->l = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;
				ppiod->t = IOT_ACCEPT;

				//�ȴ���һ���׽���(���accept�е���ڴ�,accept�ǽ��յ����ӲŴ��������׽���,�˷�ʱ��. ������׼��һ��,���ڽ�������)  
				ppiod->s = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

				//����AcceptEx��������ַ������Ҫ��ԭ�е��������16���ֽ�  
				//������߳�Ͷ��һ���������ӵĵ�����  
				if (!m_lpfnAcceptEx(sListen, ppiod->s, ppiod->b, 0, dwAddrSize, dwAddrSize, &dwBytesReceived, &(ppiod->o)))
				{
					if ((nError = WSAGetLastError()) != ERROR_IO_PENDING)
					{
						printf("AcceptEx failure!(%d).\n", nError);
						bResult = FALSE;
					}
				}
			}
			else
			{
				printf("PER_IO_OPERATION_DATA MALLOC failure!(%d).\n", GetLastError());
				bResult = FALSE;
			}

			return bResult;
		}

		BOOL IOSendHandler(LP_PER_HANDLE_DATA pphd, LP_PER_IO_OPERATION_DATA ppiod)
		{
			int nError = 0;
			BOOL bResult = TRUE;
			DWORD dwFlags = (0L);
			DWORD dwRecvBytes = (0L);
			ZeroMemory(&ppiod->o, sizeof(OVERLAPPED));

			ppiod->t = IOT_RECV;
			ppiod->d.buf = ppiod->b;
			ppiod->d.len = ppiod->l = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;

			if (SOCKET_ERROR == WSARecv(pphd->s, &ppiod->d, 1, &dwRecvBytes, &dwFlags, &ppiod->o, NULL))
			{
				if ((nError = WSAGetLastError()) != ERROR_IO_PENDING)
				{
					printf("WSARecv failure!(%d)\n", nError);
					bResult = FALSE;
				}
			}

			return bResult;
		}

		BOOL IORecvHandler(LP_PER_HANDLE_DATA pphd, LP_PER_IO_OPERATION_DATA ppiod)
		{
			int nError = 0;
			BOOL bResult = TRUE;
			DWORD dwFlags = (0L);
			DWORD dwNumberOfBytesSent = (0L);
			ZeroMemory(&ppiod->o, sizeof(OVERLAPPED));

			ppiod->t = IOT_SEND;
			ppiod->d.len = MAXIMUM_REPARSE_DATA_BUFFER_SIZE;

			if (SOCKET_ERROR == WSASend(pphd->s, &ppiod->d, 1, &dwNumberOfBytesSent, dwFlags, &ppiod->o, NULL))
			{
				if ((nError = WSAGetLastError()) != ERROR_IO_PENDING)
				{
					printf("WSASend failure!(%d)\n", nError);
					bResult = FALSE;
				}
			}

			return bResult;
		}
		BOOL IOAcceptHandler(LP_PER_HANDLE_DATA pphd, LP_PER_IO_OPERATION_DATA ppiod)
		{
			BOOL bResult = FALSE;
			SOCKADDR_IN *pil = NULL;
			SOCKADDR_IN *pir = NULL;
			LP_PER_HANDLE_DATA pcphd = NULL;
			INT nill = sizeof(SOCKADDR_IN);
			INT nirl = sizeof(SOCKADDR_IN);
			DWORD dwAddrSize = sizeof(SOCKADDR_IN) + 0x10;

			setsockopt(ppiod->s, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (const char *)&(pphd->s), sizeof(pphd->s));

			pcphd = (LP_PER_HANDLE_DATA)MALLOC(sizeof(PER_HANDLE_DATA));
			if (pcphd)
			{
				pcphd->s = ppiod->s;

				//ʹ��GetAcceptExSockaddrs���� ��þ���ĸ�����ַ����.  
				m_lpfnGetAcceptExSockaddrs(ppiod->b, 0, dwAddrSize, dwAddrSize, (LPSOCKADDR*)&pil, &nill, (LPSOCKADDR*)&pir, &nirl);

				memcpy(&pcphd->i, pir, sizeof(SOCKADDR_IN));

				printf("Accept success!\n");
				printf("ClientIp: [%s:%d]\n", inet_ntoa(pcphd->i.sin_addr), pcphd->i.sin_port);

				//�����������׽��ֹ�������ɶ˿�(����Ҫ�ͷŷ��ؾ��)
				CreateIoCompletionPort((HANDLE)pcphd->s, m_hIOCP, (ULONG_PTR)pcphd, 0L);

				bResult = IOSendHandler(pcphd, ppiod);
				if (bResult)
				{
					//���յ�һ�����ӣ�����һ���첽������
					bResult = AcceptClient(m_sListen);
				}
			}
			else
			{
				printf("PER_HANDLE_DATA MALLOC failure!(%d).\n", GetLastError());
				bResult = FALSE;
			}

			return bResult;
		}
		BOOL IODispatchSchedule(LP_PER_HANDLE_DATA pphd, LP_PER_IO_OPERATION_DATA ppiod)
		{
			BOOL bResult = FALSE;
			switch (ppiod->t)
			{
			case IOT_RECV:
			{
				//�����������
				bResult = IORecvHandler(pphd, ppiod);
			}
			break;
			case IOT_SEND:
			{
				//����������
				bResult = IOSendHandler(pphd, ppiod);
			}
			break;
			case IOT_ACCEPT:
			{
				//�����½��Ự
				bResult = IOAcceptHandler(pphd, ppiod);
			}
			break;
			default:
			{

			}
			break;
			}

			return bResult;
		}

		static DWORD WINAPI WorkerThread(LPVOID lpParameter)
		{
			DWORD dwResult = (0L);
			DWORD dwNumberOfBytesTransferred = (0L);
			LP_PER_HANDLE_DATA pcphd = NULL;//��ɼ�  
			LP_PER_IO_OPERATION_DATA ppiod = NULL;//I/O����
			CIocpCore * thiz = reinterpret_cast<CIocpCore*>(lpParameter);
			HANDLE hIOCompletionPort = (HANDLE)lpParameter;

			//���ڷ�������ص�����  
			BOOL bQueuedCompletionStatus = FALSE;

			while (thiz->Running())
			{
				ppiod = NULL;
				pcphd = NULL;
				dwNumberOfBytesTransferred = (-1L);
				bQueuedCompletionStatus = FALSE;

				bQueuedCompletionStatus = GetQueuedCompletionStatus(thiz->HICOP(), &dwNumberOfBytesTransferred, (PULONG_PTR)&pcphd, (LPOVERLAPPED*)&ppiod, INFINITE);

				if (!bQueuedCompletionStatus)
				{
					DWORD dwIoError = WSAGetLastError();
					if (dwIoError == WAIT_TIMEOUT)
					{
						continue;
					}
					else if (ppiod != NULL)
					{
						//ȡ���ȴ�ִ�е��첽����
						CancelIo((HANDLE)pcphd->s);
						closesocket(pcphd->s);
						FREE(pcphd);
						pcphd = NULL;
					}
					else
					{
						thiz->Running(FALSE);
						break;
					}
				}
				else
				{
					if (!dwNumberOfBytesTransferred && ((IOT_MINI < ppiod->t) && (ppiod->t < IOT_MAXI)))
					{
						printf("�ͻ�[%s:%d]�Ͽ�������!\n", inet_ntoa(pcphd->i.sin_addr), pcphd->i.sin_port);

						//ȡ���ȴ�ִ�е��첽����
						CancelIo((HANDLE)pcphd->s);
						closesocket(pcphd->s);
						FREE(pcphd);
						pcphd = NULL;

						FREE(ppiod);
						ppiod = NULL;

						continue;
					}
					else
					{
						thiz->IODispatchSchedule(pcphd, ppiod);
					}
				}
			}

			return dwResult;
		}

		// ��������
		BOOL StartRun()
		{
			BOOL bResult = (FALSE);
			INT nOptVal = (1);
			DWORD dwServerAcceptNumber = (0L);
			DWORD dwWorkerThreadNumber = (0L);
			SYSTEM_INFO systeminfo = { 0 };
			HANDLE hThread = NULL;
			DWORD dwThreadId = (0L);
			SOCKADDR_IN sockadrdin = { 0 };
			//���
			LP_PER_HANDLE_DATA pphd = NULL;

			std::vector<CThreadHelper*> threadVector;

			m_hIOCP = NULL;
			m_sListen = INVALID_SOCKET;

			//��ȡCPU����������
			GetNativeSystemInfo(&systeminfo);

			//������ɶ˿�
			m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

			//�׽��ֳ�ʼ��
			bResult = NetworkIoStartup();

			//�ȴ���һ���׽������ڼ���  
			m_sListen = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
			 
			//����socket�˿�Ϊ������
			setsockopt(m_sListen, SOL_SOCKET, SO_REUSEADDR, (const char *)&nOptVal, sizeof(nOptVal));

			//�趨�߳�����״̬
			m_bRunning = TRUE;

			// ����CPU����������������(dwNumberOfProcessors * 2 + 2)�Ĺ����߳� 
			dwWorkerThreadNumber = (systeminfo.dwNumberOfProcessors * 2 + 2);
			for(auto dwIndex = 0; dwIndex < dwWorkerThreadNumber; dwIndex++)
			{
				threadVector.push_back(new CThreadHelper(WorkerThread, this));
				threadVector.at(dwIndex)->Run();
			}
			
			//�������׽�������ɶ˿ڰ�	
			pphd = (LP_PER_HANDLE_DATA)MALLOC(sizeof(PER_HANDLE_DATA));
			if (pphd)
			{
				pphd->s = m_sListen;
				//�������׽�������ɶ˿ڰ�(����Ҫ�ͷŷ��ؾ��)
				CreateIoCompletionPort((HANDLE)m_sListen, m_hIOCP, (ULONG_PTR)pphd, 0);

				//�����׽��ְ󶨼���  
				sockadrdin.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
				sockadrdin.sin_family = AF_INET;
				sockadrdin.sin_port = htons(m_nServerListenPort);

				//�󶨼����׽���
				bind(m_sListen, (SOCKADDR*)&sockadrdin, sizeof(SOCKADDR));

				//�����׽��ּ���
				listen(m_sListen, SOMAXCONN);

				if (m_sListen == SOCKET_ERROR)
				{
					goto __STOPED_SERVER__;
				}

				/////////////////////////////////////////////////////////////////////  
				//ʹ��WSAIoctl��ȡAcceptEx����ָ��  
				if (true)
				{
					DWORD dwBytesReturned = (0L);
					//Accept function GUID  
					GUID guidAcceptEx = WSAID_ACCEPTEX;

					if (0 != WSAIoctl(m_sListen, SIO_GET_EXTENSION_FUNCTION_POINTER,
						&guidAcceptEx, sizeof(guidAcceptEx),
						&m_lpfnAcceptEx, sizeof(m_lpfnAcceptEx),
						&dwBytesReturned, NULL, NULL))
					{
					}

					// ��ȡGetAcceptExSockAddrs����ָ�룬Ҳ��ͬ��  
					GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
					if (0 != WSAIoctl(m_sListen, SIO_GET_EXTENSION_FUNCTION_POINTER,
						&guidGetAcceptExSockaddrs,
						sizeof(guidGetAcceptExSockaddrs),
						&m_lpfnGetAcceptExSockaddrs,
						sizeof(m_lpfnGetAcceptExSockaddrs),
						&dwBytesReturned, NULL, NULL))
					{
					}
				}

				//������յ��첽����
				dwServerAcceptNumber = 65536;//SOMAXCONN;
				printf("PER_HANDLE_DATA=%ld-PER_IO_OPERATION_DATA=%ld\n", sizeof(PER_HANDLE_DATA), sizeof(PER_IO_OPERATION_DATA));
				getchar();
				while (dwServerAcceptNumber > 0L)
				{
					if (AcceptClient(m_sListen))
					{
						dwServerAcceptNumber--;
						ShowMemoryInfo();
					}
					else
					{
						break;
					}
				}

				//�������߳��˳�
				::Sleep(INFINITE);
			}
			else
			{
				//printf("PER_HANDLE_DATA MALLOC failure!(%d).", GetLastError());
			}

		__STOPED_SERVER__:
			m_bRunning = FALSE;
			if (pphd)
			{
				closesocket(pphd->s);
				FREE(pphd);
				pphd = NULL;
			}
			CloseHandle(m_hIOCP);
			NetworkIoCleanup();

			return bResult;
		}

	public:
		void ShowMemoryInfo(void)
		{
			PROCESS_MEMORY_COUNTERS pmc = { 0 };
			GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
			printf("Memory usage:%ldK/%ldK + %ldK/%ldK\n", pmc.WorkingSetSize / 1000, pmc.PeakWorkingSetSize / 1000, pmc.PagefileUsage / 1000, pmc.PeakPagefileUsage / 1000);
		}
		HANDLE HICOP()
		{
			return m_hIOCP;
		}
		void Running(BOOL bRunning)
		{
			m_bRunning = bRunning;
		}
		BOOL Running() {
			return m_bRunning;
		}
	private:
		WSADATA m_wsaData;
		USHORT m_nServerListenPort;

		//////////////////////////////////////////////////  
		//AcceptEx����ָ��  
		LPFN_ACCEPTEX m_lpfnAcceptEx;
		//����GetAcceptExSockaddrs����ָ��
		LPFN_GETACCEPTEXSOCKADDRS m_lpfnGetAcceptExSockaddrs;

		BOOL m_bRunning;
		SOCKET m_sListen;
		HANDLE m_hIOCP;
	};

}