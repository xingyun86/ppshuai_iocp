
#ifndef UNICODE
#define UNICODE
#endif

#ifndef _UNICODE
#define _UNICODE
#endif

#define WIN32_LEAN_AND_MEAN

#include "IocpCore.h"

int main(int argc, char ** argv)
{
	int nResult = (0);
		
	IOCP::CIocpCore iocpCore(6000);
	
	iocpCore.StartRun();

	return nResult;
}