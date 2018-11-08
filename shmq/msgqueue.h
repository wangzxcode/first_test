#ifndef _MSG_QUEUE_H_
#define _MSG_QUEUE_H_
#include <windows.h>
#include <tchar.h>
#include <assert.h>

/*固定大小256，不能修改*/
#define QSIZE 256
/*
#define ERROR_MQ_OK            0
#define ERROR_MQ_FULL          1
#define ERROR_MQ_EMPTY         2
#define ERROR_MQ_CREATE_SHM    3
#define ERROR_MQ_OPEN_SHM      4
#define ERROR_MQ_MAP_SHM       5
#define ERROR_MQ_CREATE_MTX    6
#define ERROR_MQ_OPEN_MTX      7
#define ERROR_MQ_WAIT_MTX      8
#define ERROR_MQ_CREATE_SEMP   9
#define ERROR_MQ_OPEN_SEMP     10
#define ERROR_MQ_WAIT_SEMP     11
#define ERROR_MQ_DISNABLE_PUSH 12
#define ERROR_MQ_INVALID_PARAM 13
*/

#define SHM_NAME szShmName
#define SEM_NAME szSemName
#define MTX_NAME szMtxName

#define MAKE_KERNAL_OBJ_NAME(name)\
	TCHAR SHM_NAME[MAX_PATH] = _T("");\
	TCHAR SEM_NAME[MAX_PATH] = _T("");\
	TCHAR MTX_NAME[MAX_PATH] = _T("");\
	_stprintf_s(SHM_NAME, _T("%s%s"), name, _T("_SHMQ_MEM_5367"));\
	_stprintf_s(SEM_NAME, _T("%s%s"), name, _T("_SHMQ_SEM_5367"));\
	_stprintf_s(MTX_NAME, _T("%s%s"), name, _T("_SHMQ_MTX_5367"));

template<typename T>
class MsgQueue
{
	typedef struct{
		unsigned char head;
		unsigned char tail;
		unsigned char pad0;
		unsigned char pad1;
	}QINFO;
public:
	MsgQueue():m_hMutex(NULL),m_hSemaphore(NULL),m_hSharedMemory(NULL),m_pBuf(NULL),m_bIsOpen(false){}
	~MsgQueue(){Close();}

	int Create(LPCTSTR lpName)
	{
		assert(lpName != NULL && m_hSharedMemory == NULL);
		MAKE_KERNAL_OBJ_NAME(lpName);

		BOOL bCreateFirst = TRUE;
		DWORD dwSize = sizeof(QINFO) + QSIZE*sizeof(T);
		m_hSharedMemory = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, dwSize, SHM_NAME);
		if(m_hSharedMemory == NULL)
			return ERROR_MQ_CREATE_SHM;

		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			bCreateFirst = FALSE;
		}

		m_pBuf = (CHAR*)::MapViewOfFile(m_hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if(m_pBuf == NULL)
		{
			Close();
			return ERROR_MQ_MAP_SHM;
		}

		m_hSemaphore = ::CreateSemaphore(NULL, 0, QSIZE, SEM_NAME);
		if(m_hSemaphore == NULL)
		{
			Close();
			return ERROR_MQ_CREATE_SEMP;
		}

		m_hMutex = ::CreateMutex(NULL, FALSE, MTX_NAME);
		if(m_hMutex == NULL)
		{
			Close();
			return ERROR_MQ_CREATE_MTX;
		}

		if(bCreateFirst)
		{
			QINFO* info = (QINFO*)m_pBuf;
			info->head = info->tail = 0;
		}

		m_bIsOpen = true;
		return ERROR_MQ_OK;
	}

	int Open(LPCTSTR lpName)
	{
		assert(lpName != NULL && m_hSharedMemory == NULL);
		MAKE_KERNAL_OBJ_NAME(lpName);

		m_hSharedMemory = ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
		if(m_hSharedMemory == NULL)
		{
			return ERROR_MQ_OPEN_SHM;
		}

		m_pBuf = (CHAR*)::MapViewOfFile(m_hSharedMemory, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if(m_pBuf == NULL)
		{
			Close();
			return ERROR_MQ_MAP_SHM;
		}

		m_hSemaphore = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEM_NAME);
		if(m_hSemaphore == NULL)
		{
			Close();
			return ERROR_MQ_OPEN_SEMP;
		}

		m_hMutex = ::OpenMutex(MUTEX_ALL_ACCESS, FALSE, MTX_NAME);
		if(m_hMutex == NULL)
		{
			Close();
			return ERROR_MQ_OPEN_MTX;
		}

		m_bIsOpen = true;
		return ERROR_MQ_OK;
	}

	void Close(){
		if(m_pBuf != NULL){
			assert(m_hSharedMemory != NULL);
			::UnmapViewOfFile(m_pBuf);
			m_pBuf = NULL;
		}

		if(m_hSharedMemory != NULL){
			CloseHandle(m_hSharedMemory);
			m_hSharedMemory = NULL;
		}

		if(m_hSemaphore != NULL){
			CloseHandle(m_hSemaphore);
			m_hSemaphore = NULL;		
		}

		if(m_hMutex != NULL){
			CloseHandle(m_hMutex);
			m_hMutex = NULL;		
		}
		m_bIsOpen = false;
	}

	BOOL IsOpen(){ return m_bIsOpen;}
	void EnablePush()
	{
		if(m_pBuf != NULL)
		{
			QINFO* info = (QINFO*)m_pBuf;
			info->pad0 |= 0x01;
		}
	}
	void DisenablePush()
	{
		if(m_pBuf != NULL)
		{
			QINFO* info = (QINFO*)m_pBuf;
			info->pad0 &= 0xFE;
		}
	}

	int Push(const T& msg, DWORD dwTimeOut = 3000)const
	{
		int nErr = ERROR_MQ_OK;
		assert(m_pBuf != NULL);
		QINFO* info = (QINFO*)m_pBuf;

		if(!(info->pad0 & 0x01))
			return ERROR_MQ_DISNABLE_PUSH;

		if(::WaitForSingleObject(m_hMutex, dwTimeOut) == WAIT_TIMEOUT)
			return ERROR_MQ_WAIT_MTX;

		if(info->head+1 != info->tail)
		{
			unsigned int idx = info->head;
			memcpy(m_pBuf+sizeof(QINFO)+idx*sizeof(T), &msg, sizeof(T));
			info->head++;
			::ReleaseSemaphore(m_hSemaphore, 1, NULL);
		}
		else
		{
			nErr = ERROR_MQ_FULL;
		}

		::ReleaseMutex(m_hMutex);

		return nErr;
	}

	int Pop(T& msg, DWORD dwTimeOut = 3000)const
	{
		assert(m_pBuf != NULL);
		QINFO* info = (QINFO*)m_pBuf;
		if(info->head == info->tail)
		{
			DWORD dwWait = ::WaitForSingleObject(m_hSemaphore, dwTimeOut);
			if(dwWait != WAIT_OBJECT_0)
				return ERROR_MQ_WAIT_SEMP;
		}

		if(info->head == info->tail)
			return ERROR_MQ_EMPTY;		

		//assert(info->head != info->tail);

		unsigned int idx = info->tail;
		memcpy(&msg, m_pBuf+sizeof(QINFO)+idx*sizeof(T), sizeof(T));			
		info->tail++;

		return ERROR_MQ_OK;
	}

private:
	HANDLE m_hMutex;
	HANDLE m_hSemaphore;
	HANDLE m_hSharedMemory;
	CHAR*  m_pBuf;
	BOOL   m_bIsOpen;
};

#endif