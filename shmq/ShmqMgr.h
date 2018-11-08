#pragma once

#include <map>
#include <string>
#include "../include/shmq.h"
#include "msgqueue.h"


class ShmqMgr
{
public:
	ShmqMgr(void);
	~ShmqMgr(void);
	
	int  Create(const wchar_t* szName);
	void Close();
	int  Send(const wchar_t* lpQueueName, unsigned long ulId, unsigned long ulType = 0, const void* lpData = 0, int nDataLen = 0);
	int  Recv(IPC_MSG* msg, int timeout = 1000);

private:
	typedef MsgQueue<IPC_MSG> MSGQ;
	CRITICAL_SECTION m_MsgQueMapCS;
	std::map<std::wstring, MSGQ*> m_MsgQueMap;
	MSGQ m_RecvQue;
	std::wstring m_strQueName;
};

