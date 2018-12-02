#pragma once

#include <windows.h>
#include <process.h>
//+------------------------------------------------------------------+
//| Thread manipulation class                                        |
//+------------------------------------------------------------------+
class CThreadHelper
{
public:
	//+------------------------------------------------------------------+
	//| Constructor                                                      |
	//+------------------------------------------------------------------+
	CThreadHelper(void) : m_hThread(NULL),m_lpThreadStartRoutine(NULL),m_lpThreadParams(NULL), m_stThreadStackSize(0L) {
	}	
	//+------------------------------------------------------------------+
	//| Constructor                                                      |
	//+------------------------------------------------------------------+
	CThreadHelper(LPTHREAD_START_ROUTINE lpThreadStartRoutine, LPVOID lpThreadParams, SIZE_T stThreadStackSize = 0L) : m_hThread(NULL),m_lpThreadStartRoutine(lpThreadStartRoutine), m_lpThreadParams(lpThreadParams), m_stThreadStackSize(stThreadStackSize) {
	}
	//+------------------------------------------------------------------+
	//| Destructor                                                       |
	//+------------------------------------------------------------------+
	~CThreadHelper(void) {
		//--- wait for shutdown
		Shutdown();
		//--- close handle
		if (m_hThread != NULL)
		{
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}
	//+------------------------------------------------------------------+
	//| Thread run start                                                 |
	//+------------------------------------------------------------------+
	__inline BOOL Run() {
		DWORD dwThreadId = 0L;
		DWORD dwThreadExitCode = 0L;
		//--- thread has been started...
		if (m_hThread != NULL)
		{
			GetExitCodeThread(m_hThread, &dwThreadExitCode);
			//--- still active
			if (dwThreadExitCode == STILL_ACTIVE)
			{
				return(FALSE);
			}
			//--- close handle
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
		//--- start thread
		if ((m_hThread = CreateThread(NULL, m_stThreadStackSize, m_lpThreadStartRoutine, m_lpThreadParams, STACK_SIZE_PARAM_IS_A_RESERVATION, &dwThreadId)) == NULL)
		{
			return(FALSE);
		}
		//--- ok
		return(TRUE);
	}
	//+------------------------------------------------------------------+
	//| Thread shutdown                                                  |
	//+------------------------------------------------------------------+
	__inline BOOL Shutdown(DWORD dwTimeout = INFINITE) {
		//--- wait for thread shutdown
		return ((m_hThread == NULL) || (WaitForSingleObject(m_hThread, dwTimeout) == WAIT_OBJECT_0));
	}
	//+------------------------------------------------------------------+
	//| Thread termination                                               |
	//+------------------------------------------------------------------+
	__inline void Terminate(void) {
		//--- check
		if (m_hThread)
		{
			//--- kill thread
			TerminateThread(m_hThread, (0L));
			//--- close handle
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
		//---
	}
	//+------------------------------------------------------------------+
	//| Check thread activity                                            |
	//+------------------------------------------------------------------+
	__inline BOOL IsActive(void) {
		//--- check
		if (m_hThread)
		{
			//--- request state
			DWORD dwThreadExitCode = 0L;
			GetExitCodeThread(m_hThread, &dwThreadExitCode);
			if (dwThreadExitCode == STILL_ACTIVE)
			{
				return(TRUE);
			}
			//--- close handle
			CloseHandle(m_hThread);
			m_hThread = NULL;
			//--- thread finished
		}
		return(FALSE);
	}

	//+------------------------------------------------------------------+
	//| Thread priority modification                                     |
	//+------------------------------------------------------------------+
	__inline BOOL Priority(INT nPriority) {
		return (m_hThread && SetThreadPriority(m_hThread, nPriority));
	}

public:
	__inline void SetThreadStartRoutine(LPTHREAD_START_ROUTINE lpThreadStartRoutine) {
		m_lpThreadStartRoutine = lpThreadStartRoutine;
	}
	__inline void SetThreadParams(LPVOID lpThreadParams) {
		m_lpThreadParams = lpThreadParams;
	}
	__inline void SetThreadStackSize(SIZE_T stThreadStackSize) {
		m_stThreadStackSize = stThreadStackSize;
	}
	__inline HANDLE GetThreadHandle(void) const {
		return (m_hThread); 
	}

private:
	HANDLE	m_hThread;

	SIZE_T m_stThreadStackSize;
	LPTHREAD_START_ROUTINE m_lpThreadStartRoutine;
	LPVOID m_lpThreadParams;
};
