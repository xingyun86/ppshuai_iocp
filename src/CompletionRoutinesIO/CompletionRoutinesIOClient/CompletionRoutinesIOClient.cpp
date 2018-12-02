#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <tchar.h>
#include <stdio.h>
#include <signal.h>

#pragma comment(lib, "ws2_32.lib")

#define DATA_BUFSIZE 16384  

#define SERVER_IP_ADDRESS		"127.0.0.1"
#define SERVER_PORT				6000
#define MAX_CLIENT_CONNECT_NUM	1000

#include <map>

class CThreadHelper{
#define THREAD_QUIT_BASECODE	(WM_USER + WM_QUIT)
#define THREADID_TO_QUITCODE(x) (x + THREAD_QUIT_BASECODE)
#define QUITCODE_TO_THREADID(x) (x - THREAD_QUIT_BASECODE)
public:

	enum THREADSTATUS_TYPE{
		TSTYPE_MINIMUM = (0L),
		TSTYPE_NULLPTR = (0L),
		TSTYPE_STARTED = (1L),
		TSTYPE_SUSPEND = (2L),
		TSTYPE_RUNNING = (4L),
		TSTYPE_STOPPED = (8L),
		TSTYPE_MAXIMUM = TSTYPE_NULLPTR | TSTYPE_STARTED | TSTYPE_SUSPEND | TSTYPE_RUNNING | TSTYPE_STOPPED,
	};
	enum THREADEXITCALL_TYPE{
		TECTYPE_SSYNCHRONOUS = (0L),
		TECTYPE_ASYNCHRONOUS = (1L),
	};

	CThreadHelper(
		LPTHREAD_START_ROUTINE fnThreadStartRoutine = (0L),
		LPVOID lpThreadParameters = (0L),
		DWORD dwThreadCreationFlags = (0L),
		SIZE_T dwThreadStackSize = (0L),
		LPSECURITY_ATTRIBUTES lpThreadSecurityAttributes = (0L),
		THREADEXITCALL_TYPE nThreadExitCall = TECTYPE_SSYNCHRONOUS
		)
	{
		Startup(fnThreadStartRoutine, lpThreadParameters, dwThreadCreationFlags, dwThreadStackSize, lpThreadSecurityAttributes, nThreadExitCall);
	}

	~CThreadHelper()
	{
		Cleanup();
	}
	static DWORD WINAPI ThreadStartRoutine(LPVOID _lpThreadParameters)
	{
		DWORD dwResult = 0L;
		LPTHREAD_START_ROUTINE fnThreadStartRoutine = NULL;
		CThreadHelper * pTH = (CThreadHelper *)_lpThreadParameters;
		if (pTH)
		{
			while (pTH->GetThreadStartRoutine())
			{
				switch (pTH->GetThreadStatus())
				{
				case TSTYPE_RUNNING:
				{
					pTH->GetThreadStartRoutine()(pTH);
				}
				break;
				default:
				{
					goto __LEAVE_CLEAN__;
				}
				break;
				}
			}

		__LEAVE_CLEAN__:

			::PostThreadMessage(pTH->GetNotifyThreadId(), pTH->GetThreadExitCode(), (WPARAM)(0L), (LPARAM)(0L));
		}

		return dwResult;
	}

	VOID Start()
	{
		if (m_hThread && m_dwThreadId && m_dwThreadExitCode)
		{
			SetThreadStatus(TSTYPE_STARTED);
			Resumed();
		}
		else
		{
			Cleanup();
		}
	}
	VOID Suspend()
	{
		::SuspendThread(m_hThread);
		SetThreadStatus(TSTYPE_SUSPEND);
	}
	VOID Resumed()
	{
		::ResumeThread(m_hThread);
		SetThreadStatus(TSTYPE_RUNNING);
	}
	VOID Stopped()
	{
		ClearSsync();
	}
	VOID QueueSsync()
	{
		MSG msg = { 0 }; while (::GetMessage(&msg, 0, 0, 0) && (msg.message != m_dwThreadExitCode)){/*::TranslateMessage(&msg);::DispatchMessage(&msg);*/ };
	}

	VOID QueueAsync()
	{
		MSG msg = { 0 }; while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) || (msg.message != m_dwThreadExitCode)){/*::TranslateMessage(&msg);::DispatchMessage(&msg);*/ };
	}
	VOID ClearSsync()
	{
		if (m_hThread != (0L))
		{
			CloseHandle(m_hThread);
			m_hThread = (0L);
		}
		if (GetThreadStatus() == TSTYPE_RUNNING)
		{
			m_dwNotifyThreadId = ::GetCurrentThreadId();
			SetThreadStatus(TSTYPE_STOPPED);
			if ((m_dwThreadId != m_dwNotifyThreadId) && (m_dwThreadExitCode >= THREAD_QUIT_BASECODE))
			{
				switch (m_nThreadExitCall)
				{
				case CThreadHelper::TECTYPE_SSYNCHRONOUS:
					QueueSsync();
					break;
				case CThreadHelper::TECTYPE_ASYNCHRONOUS:
					QueueAsync();
					break;
				default:
					break;
				}
			}
		}
	}
	VOID ClearAsync(DWORD dwNotifyThreadId = ::GetCurrentThreadId())
	{
		if (m_hThread != (0L))
		{
			CloseHandle(m_hThread);
			m_hThread = (0L);
		}
		if (GetThreadStatus() == TSTYPE_RUNNING)
		{
			m_dwNotifyThreadId = dwNotifyThreadId;
			SetThreadStatus(TSTYPE_STOPPED);
		}
	}
	VOID Close()
	{
		Cleanup();
	}
	VOID Reset()
	{
		memset(&m_ThreadSecurityAttributes, 0, sizeof(m_ThreadSecurityAttributes));
		m_dwThreadStackSize = (0L);
		m_fnThreadStartRoutine = (0L);
		m_lpThreadParameters = (0L);
		m_dwThreadCreationFlags = (0L);
		m_dwThreadId = (0L);
		m_hThread = (0L);
		m_dwThreadExitCode = (0L);
		m_nThreadStatus = TSTYPE_NULLPTR;
		m_nThreadExitCall = TECTYPE_SSYNCHRONOUS;
	}

	VOID Startup(
		LPTHREAD_START_ROUTINE fnThreadStartRoutine = (0L),
		LPVOID lpThreadParameters = (0L),
		DWORD dwThreadCreationFlags = (0L),
		SIZE_T dwThreadStackSize = (0L),
		LPSECURITY_ATTRIBUTES lpThreadSecurityAttributes = (0L),
		THREADEXITCALL_TYPE nThreadExitCall = TECTYPE_SSYNCHRONOUS
		)
	{

		if (lpThreadSecurityAttributes != (0L))
		{
			memcpy(&m_ThreadSecurityAttributes, lpThreadSecurityAttributes, sizeof(m_ThreadSecurityAttributes));
		}
		else
		{
			memset(&m_ThreadSecurityAttributes, 0, sizeof(m_ThreadSecurityAttributes));
		}

		m_hThread = (0L);

		m_dwThreadStackSize = dwThreadStackSize;
		m_fnThreadStartRoutine = fnThreadStartRoutine;
		m_lpThreadParameters = lpThreadParameters;
		m_dwThreadCreationFlags = dwThreadCreationFlags | CREATE_SUSPENDED;
		m_dwThreadId = (0L);

		m_dwNotifyThreadId = (0L);
		m_dwThreadExitCode = (0L);
		m_nThreadStatus = TSTYPE_NULLPTR;
		m_nThreadExitCall = nThreadExitCall;

		m_hThread = CreateThread(&m_ThreadSecurityAttributes, m_dwThreadStackSize, &CThreadHelper::ThreadStartRoutine, this, m_dwThreadCreationFlags, &m_dwThreadId);
		if (m_hThread && m_dwThreadId)
		{
			m_dwThreadExitCode = THREADID_TO_QUITCODE(m_dwThreadId);
		}
	}
	VOID Cleanup()
	{
		ClearSsync();
		Reset();
	}

	VOID SetThreadStatus(THREADSTATUS_TYPE nThreadStatus = TSTYPE_NULLPTR)
	{
		m_nThreadStatus = (THREADSTATUS_TYPE)(nThreadStatus | TSTYPE_MINIMUM);
	}

	THREADSTATUS_TYPE GetThreadStatus()
	{
		return (THREADSTATUS_TYPE)(m_nThreadStatus & TSTYPE_MAXIMUM);
	}

	VOID SetThreadExitCall(THREADEXITCALL_TYPE nThreadExitCall = TECTYPE_SSYNCHRONOUS)
	{
		m_nThreadExitCall = nThreadExitCall;
	}

	THREADEXITCALL_TYPE GetThreadExitCall()
	{
		return m_nThreadExitCall;
	}

	VOID SetThreadPriority(INT nPriority)
	{
		::SetThreadPriority(m_hThread, nPriority);
	}

	INT GetThreadPriority()
	{
		return ::GetThreadPriority(m_hThread);
	}
	BOOL IsThreadRunning()
	{
		return ((GetThreadStatus() & TSTYPE_RUNNING) == TSTYPE_RUNNING);
	}

	LPTHREAD_START_ROUTINE GetThreadStartRoutine()
	{
		return m_fnThreadStartRoutine;
	}
	LPVOID GetThreadParameters()
	{
		return m_lpThreadParameters;
	}

	DWORD GetThreadId()
	{
		return m_dwThreadId;
	}
	DWORD GetThreadCreationFlags()
	{
		return m_dwThreadCreationFlags;
	}
	SIZE_T GetThreadStackSize()
	{
		return m_dwThreadStackSize;
	}
	LPSECURITY_ATTRIBUTES GetThreadSecurityAttributes()
	{
		return &m_ThreadSecurityAttributes;
	}
	DWORD GetNotifyThreadId()
	{
		return m_dwNotifyThreadId;
	}
	void SetNotifyThreadId(DWORD dwNotifyThreadId)
	{
		m_dwNotifyThreadId = dwNotifyThreadId;
	}
	DWORD GetThreadExitCode()
	{
		return m_dwThreadExitCode;
	}

private:

	HANDLE m_hThread;
	DWORD m_dwNotifyThreadId;
	DWORD m_dwThreadExitCode;
	THREADSTATUS_TYPE m_nThreadStatus;
	THREADEXITCALL_TYPE m_nThreadExitCall;

	LPTHREAD_START_ROUTINE m_fnThreadStartRoutine;
	LPVOID m_lpThreadParameters;
	DWORD m_dwThreadCreationFlags;
	SIZE_T m_dwThreadStackSize;
	SECURITY_ATTRIBUTES m_ThreadSecurityAttributes;
	DWORD m_dwThreadId;
};

class CManageThreadHelper
{
public:
	CManageThreadHelper(DWORD dwThreadId = GetCurrentThreadId()){ m_dwThreadId = dwThreadId; }
	~CManageThreadHelper(){}

	std::map<DWORD, CThreadHelper *>::iterator InsertThread(CThreadHelper * pTH)
	{
		auto it = m_ttmap.find(pTH->GetThreadId());
		if (it == m_ttmap.end())
		{
			m_ttmap.insert(std::map<DWORD, CThreadHelper *>::value_type(pTH->GetThreadId(), pTH));
		}
		else
		{
			it->second = pTH;
		}
		return it;
	}
	std::map<DWORD, CThreadHelper *>::iterator RemoveThread(DWORD dwThreadId)
	{
		auto it = m_ttmap.find(dwThreadId);
		if (it != m_ttmap.end())
		{
			if (it->second->IsThreadRunning())
			{
				it->second->ClearAsync(this->m_dwThreadId);
			}
			if (it->second)
			{
				delete it->second;
				it->second = NULL;
			}
			it = m_ttmap.erase(it);
		}
		return it;
	}
	void StartAll()
	{
		for (auto it = m_ttmap.begin(); it != m_ttmap.end(); it++)
		{
			if (!it->second->IsThreadRunning())
			{
				it->second->Start();
			}
		}
	}

	void ClearAll()
	{
		for (auto it = m_ttmap.begin(); it != m_ttmap.end(); it++)
		{
			if (it->second->IsThreadRunning())
			{
				it->second->ClearAsync(this->m_dwThreadId);
			}
			else
			{
				it = m_ttmap.erase(it);
			}
		}

	}
	void CleanAll()
	{
		MSG msg = { 0 };
		CThreadHelper * pTH = NULL;
		//while (::GetMessage(&msg, 0, 0, 0) && m_ttmap.size())
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) || m_ttmap.size())
		{
			RemoveThread(QUITCODE_TO_THREADID(msg.message));
		}
	}

private:
	DWORD m_dwThreadId;
	std::map<DWORD, CThreadHelper *> m_ttmap;
};

CManageThreadHelper G_ManageThreadHelper;

BOOL NetworkIoStartup(WSAData * pwsadata = (0), WORD wVersion = MAKEWORD(2, 2))
{
	WSADATA wsadata = { 0 };
	WSAData * pwsad = pwsadata ? pwsadata : &wsadata;

	return (WSAStartup(wVersion, pwsad) == 0);
}
BOOL NetworkIoCleanup()
{
	return (WSACleanup() == 0);
}

DWORD WINAPI WorkerThread(LPVOID lpParameter)
{
	CThreadHelper * pTH = (CThreadHelper *)lpParameter;
	
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	SOCKADDR_IN sockaddrin = { 0 };
	sockaddrin.sin_family = AF_INET;
	sockaddrin.sin_port = htons(SERVER_PORT);
	sockaddrin.sin_addr.S_un.S_addr = inet_addr((SERVER_IP_ADDRESS));

	int reVal = connect(sock, (SOCKADDR*)&sockaddrin, sizeof(SOCKADDR));
	if (reVal == SOCKET_ERROR)
	{
		printf("cannot connect SERVER! %d\n", WSAGetLastError());
		return 0;
	}

	CHAR buf[DATA_BUFSIZE] = ("光阴的故事!");
	
	while (pTH->IsThreadRunning())
	{
		if (SOCKET_ERROR == send(sock, buf, DATA_BUFSIZE, 0))
		{
			printf("cannot SEND message to server! %d\n", WSAGetLastError());
			//break;
		}

		memset(buf, 0, sizeof(buf));	//清空一下，体现是接收到的数据

		if (SOCKET_ERROR == recv(sock, buf, DATA_BUFSIZE, 0))
		{
			printf("cannot RECV message to server! %d\n", WSAGetLastError());
			//break;
		}

		printf("接收到的消息:%s\r\n", buf);
		Sleep(1000);
	}

	closesocket(sock);

	pTH->Stopped();
	
	return 0;
}

void signal_handle(int sign_no)
{
	switch (sign_no)
	{
	case SIGINT:
	{
		printf("Get SIGINT signal\n");
		G_ManageThreadHelper.ClearAll();
	}
	break;
	case SIGABRT:
	case SIGABRT_COMPAT:
	{
		printf("Get SIGABRT signal\n");
		G_ManageThreadHelper.ClearAll();
	}
	break;
	case SIGBREAK:
	{
		printf("Get SIGBREAK signal\n");
		G_ManageThreadHelper.ClearAll();
	}
	break;
	default:
	{

	}
		break;
	}
}

int _tmain(int argc, _TCHAR ** argv)
{
	signal(SIGINT, signal_handle);
	signal(SIGBREAK, signal_handle);
	signal(SIGABRT, signal_handle);
	signal(SIGABRT_COMPAT, signal_handle);

	NetworkIoStartup();

	for (int i = 0; i < MAX_CLIENT_CONNECT_NUM; i++)
	{
		G_ManageThreadHelper.InsertThread(new CThreadHelper(WorkerThread));
	}
	
	G_ManageThreadHelper.StartAll();

	G_ManageThreadHelper.CleanAll();

	NetworkIoCleanup();

	return 0;
}
