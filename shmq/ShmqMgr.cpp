#include "ShmqMgr.h"

ShmqMgr::ShmqMgr(void)
{
	InitializeCriticalSection(&m_MsgQueMapCS);
	/*
	wchar_t szAppName[MAX_PATH] = L"";
	::GetModuleFileName(NULL, szAppName, MAX_PATH);
	wchar_t* pSep = wcsrchr(szAppName, L'\\') + 1;
	wchar_t* pDot = wcsrchr(szAppName, L'.');
	*pDot = 0;

	int nErr = Create(pSep);
	if(nErr != ERROR_MQ_OK)
	{
		throw nErr;
	}*/
}

ShmqMgr::~ShmqMgr(void)
{
	DeleteCriticalSection(&m_MsgQueMapCS);
	Close();
}

int ShmqMgr::Create(const wchar_t* szName)
{
	if(m_RecvQue.IsOpen())
		return ERROR_MQ_OK;

	int nErr = m_RecvQue.Create(szName);
	if(nErr == ERROR_MQ_OK)
	{
		m_strQueName = szName;
		m_RecvQue.EnablePush();
	}
	
	return nErr;
}

void ShmqMgr::Close()
{
	m_RecvQue.DisenablePush();
	m_RecvQue.Close();

	auto iter = m_MsgQueMap.begin();
	for (; iter != m_MsgQueMap.end(); iter++){
		MSGQ* p = iter->second;
		if(p)
		{
			p->Close();
			delete p;
		}
	}

	m_MsgQueMap.clear();
}

int  ShmqMgr::Send(const wchar_t* lpQueueName, unsigned long ulId, unsigned long ulType/* = 0*/, const void* lpData/* = 0*/, int nDataLen/* = 0*/)
{
	int nErr = ERROR_MQ_OK;
	MSGQ* pMQ = NULL;

	if(nDataLen > MAX_MSG_DATA_SIZE)
		return ERROR_MQ_INVALID_PARAM;

	if(m_strQueName.compare(lpQueueName) == 0 || lpQueueName == NULL || lpQueueName[0] == 0)
	{
		pMQ = &m_RecvQue;
	}
	else
	{
		EnterCriticalSection(&m_MsgQueMapCS);
		auto iter = m_MsgQueMap.find(lpQueueName);
		if(iter == m_MsgQueMap.end())
		{
			pMQ = new MSGQ;
			nErr = pMQ->Open(lpQueueName);
			if(nErr == ERROR_MQ_OK)
			{
				m_MsgQueMap.insert(std::make_pair(lpQueueName, pMQ));
			}
			else
			{
				delete pMQ;
				pMQ = NULL;
			}
		}
		else
		{
			pMQ = iter->second;
		}
		LeaveCriticalSection(&m_MsgQueMapCS);
	}

	if(pMQ == NULL)
		return nErr;

	IPC_MSG msg;
	wcscpy_s(msg.szFrom, m_strQueName.c_str());
	msg.SendTime = GetTickCount();
	msg.Id = ulId;
	msg.Type = ulType;
	memcpy(msg.Data, lpData, nDataLen);

	return pMQ->Push(msg);
}

int  ShmqMgr::Recv(IPC_MSG* msg, int timeout/* = 1000*/)
{
	if(msg == NULL)
		return ERROR_MQ_INVALID_PARAM;

	int nErr = m_RecvQue.Pop(*msg, timeout);
	if(nErr == ERROR_MQ_OK)
	{
		(*msg).RecvTime = GetTickCount();
	}

	return nErr;
}