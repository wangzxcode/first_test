#ifndef _SHMQ_H_
#define _SHMQ_H_

#ifdef SHMQ_EXPORTS
#define SHMQ_API __declspec(dllexport)
#else
#define SHMQ_API __declspec(dllimport)
#endif

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


#define MAX_MSG_DATA_SIZE 2048

typedef struct{
	unsigned int Id;
	unsigned int Type;
	unsigned int SendTime;
	unsigned int RecvTime;
	     wchar_t szFrom[256];
	unsigned char Data[MAX_MSG_DATA_SIZE];
}IPC_MSG;

#ifdef __cplusplus
extern "C"{
#endif

SHMQ_API int __stdcall SHMQ_Send(const wchar_t* lpQueueName, unsigned long ulId, unsigned long ulType, const void* lpData, int nDataLen);
SHMQ_API int __stdcall SHMQ_Recv(IPC_MSG* msg, int timeout, const wchar_t* wzQueueName);

#define SendMsg(name, msg_id, msg_type)	SHMQ_Send(name, msg_id, msg_type, NULL, 0)
#define RecvMsg(msg, timeout)           SHMQ_Recv(msg, 1000, NULL)

#ifdef __cplusplus
};
#endif

#endif