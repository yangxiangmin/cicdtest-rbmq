#ifndef __RQBBIT_MQ_MIDLIB_H__
#define __RQBBIT_MQ_MIDLIB_H__
#include <string>

#define LIB_API

#ifdef __cplusplus
    extern "C"
    {
#endif


typedef void (* CB_REQUEST)         (const char *server, const char *src, const char *msgName, int rpid, const char *msgBody, void **outArg);
typedef void (* CB_REQUEST_TIMEOUT) (const char *server, const char *dst, const char *msgName, int rpid, void *inArg);

typedef void (* CB_RESP_TEMP)      (const char *server, const char *dst, const char *msgName, int rpid, const char *pRespCode, const char *pDescr, const char *msgBody, void *inArg);

typedef void (* CB_RESP_FINAL)      (const char *server, const char *dst, const char *msgName, int rpid, const char *pRespCode, const char *pDescr, const char *msgBody, void *inArg);
typedef void (* CB_RESP_TIMEOUT)    (const char *server, const char *src, const char *msgName, int rpid, void *inArg);

typedef void (* CB_NOTIFY)          (const char *server, const char *src, const char *msgName, const char *msgBody);
typedef void (* CB_NOTIFY_TIMEOUT)  (const char *server, const char *dst, const char *msgName, int rpid, void *inArg);

typedef void (* CB_REPORT)          (const char *server, const char *src, const char *msgName, const char *msgBody);

typedef void (* CB_SUBSCRIBLE)      (const char *topic, const char *msgBody);

typedef struct
{
	CB_REQUEST         m_cbRequest;
	CB_REQUEST_TIMEOUT m_cbRequestTimeout;

	CB_RESP_FINAL      m_cbRespFinal;
	CB_RESP_TIMEOUT    m_cbRespTimeout;

	CB_NOTIFY          m_cbNotify;
    CB_NOTIFY_TIMEOUT  m_cbNotifyTimeout;

	CB_REPORT          m_cbReport;
	CB_RESP_TEMP       m_cbRespTemp;
} MID_CLIENT_HANDLER;

LIB_API int rmSetMidMsgHandler  (MID_CLIENT_HANDLER *pfn);
LIB_API int rmInitMidwareClient (const char *modName, const char *serverIp, unsigned short serverPort, const char *userName, const char *passWord, bool svrmod);
LIB_API int rmRequestMake   (const char *modName, const char *msgName, void *inArg, const char *msgBody, int *rpid, int timeout);
LIB_API int rmNotifyMake    (const char *modName, const char *msgName, void *inArg, const char *msgBody, int *rpid, int timeout);
LIB_API int rmReportMake    (const char *modName, const char *msgName, const char *msgBody);
LIB_API int rmRequestFinalResponse  (const char *modName, int rpid, const char *respCode, const char *respDescr, const char *msgBody);
LIB_API int rmRequestTempResponse  (const char *modName, int rpid, const char *respCode, const char *respDescr, const char *msgBody);
LIB_API void MidwareClientDestroy();

#ifdef __cplusplus
    }   // extern "C"
#endif


#endif

