#include "ShmqMgr.h"

static ShmqMgr* s_Shmq = NULL;

class AutoDelete
{
public:
	~AutoDelete()
	{
		if(s_Shmq)
		{
			delete s_Shmq;
			s_Shmq = NULL;
		}
	}
}ad;

int  __stdcall SHMQ_Send(const wchar_t* lpQueueName, unsigned long ulId, unsigned long ulType/* = 0*/, const void* lpData/* = 0*/, int nDataLen/* = 0*/)
{
	if(s_Shmq == NULL)
		s_Shmq = new ShmqMgr;

	return s_Shmq->Send(lpQueueName, ulId, ulType, lpData, nDataLen);
}

int  __stdcall SHMQ_Recv(IPC_MSG* msg, int timeout, const wchar_t* wzQueueName)
{
	if(s_Shmq == NULL)
	{
		wchar_t szAppName[MAX_PATH] = L"";
		s_Shmq = new ShmqMgr;
		if(wzQueueName == NULL||*wzQueueName == 0)
		{			
			::GetModuleFileName(NULL, szAppName, MAX_PATH);
			wchar_t* pSep = wcsrchr(szAppName, L'\\') + 1;
			wchar_t* pDot = wcsrchr(szAppName, L'.');
			*pDot = 0;
			wzQueueName = pSep;
		}

		s_Shmq->Create(wzQueueName);
	}

	return s_Shmq->Recv(msg, timeout);
}
