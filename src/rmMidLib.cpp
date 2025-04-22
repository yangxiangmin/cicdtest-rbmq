#include "rabbitmqclient.h"
#include "rmMidLib.h"
#include <string.h>
#include <unistd.h>

int rmRequestMake (const char *modName, const char *msgName, void *inArg, const char *msgBody, int *rpid, int timeout)
{
	RabbitMQWrapper& client = RabbitMQWrapper::getInstance(p2ptype);

	if ((modName == NULL) || (msgName == NULL) || (msgBody == NULL)) {
		return -1;
	}

	amqp_basic_properties_t props;
    amqp_table_t headers;
    amqp_table_entry_t entries[PROP_HEAD_LEN];
	struct timespec ts;

    // 设置消息属性
    memset(&props, 0, sizeof(props));
    props.content_type = amqp_cstring_bytes("text/plain");

    // 设置附加的头部信息
	headers.entries = entries;
	int_props_st intprops;

	int timer_id = client.p2pReqTimerId();

	intprops.sender = client.GetModuleName();
	
	intprops.receiver = modName;
	intprops.msgtype = RB_REQUEST;
	intprops.msgname = msgName;
	intprops.rid = timer_id;
	intprops.inarg = inArg;
	
    clock_gettime(CLOCK_REALTIME, &ts);  // 获取当前时间（秒和纳秒）
    int64_t timestamp_ms = (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	intprops.timestamp = timestamp_ms;
	intprops.rspcode = "no code";
	intprops.rspdesc = "no description";
	intprops.trans = "no transparent";

	client.IntProps2AmqpTable(intprops, headers);
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_HEADERS_FLAG;
	props.headers = headers;

	client.p2pAddTimerTask(timer_id, std::string(modName), std::string(msgBody), timeout/1000, intprops);

	*rpid = timer_id;
	
	bool rslt = client.SendRabbitmqMessage(std::string(modName), std::string(msgBody), &props);

	if (rslt == true) {
		return 0;
	} else {
		client.p2pRemoveTimerTask(timer_id);
		return -1;
	}
}

LIB_API int rmNotifyMake (const char *modName, const char *msgName, void *inArg, const char *msgBody, int *rpid, int timeout)
{
	RabbitMQWrapper& client = RabbitMQWrapper::getInstance(p2ptype);

	if ((modName == NULL) || (msgName == NULL) || (msgBody == NULL)) {
		return -1;
	}

	amqp_basic_properties_t props;
    amqp_table_t headers;
    amqp_table_entry_t entries[PROP_HEAD_LEN];
	struct timespec ts;

    // 设置消息属性
    memset(&props, 0, sizeof(props));
    props.content_type = amqp_cstring_bytes("text/plain");

    // 设置附加的头部信息
	headers.entries = entries;
	int_props_st intprops;

	int timer_id = client.p2pReqTimerId();

	intprops.sender = client.GetModuleName();
	
	intprops.receiver = modName;
	intprops.msgtype = RB_NOTIFICATION;
	intprops.msgname = msgName;
	intprops.rid = timer_id;
	intprops.inarg = inArg;
	
    clock_gettime(CLOCK_REALTIME, &ts);  // 获取当前时间（秒和纳秒）
    int64_t timestamp_ms = (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	intprops.timestamp = timestamp_ms;
	intprops.rspcode = "no code";
	intprops.rspdesc = "no description";
	intprops.trans = "no transparent";

	client.IntProps2AmqpTable(intprops, headers);
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_HEADERS_FLAG;
	props.headers = headers;

	client.p2pAddTimerTask(timer_id, std::string(modName), std::string(msgBody), timeout/1000, intprops);

	*rpid = timer_id;
	
	bool rslt = client.SendRabbitmqMessage(std::string(modName), std::string(msgBody), &props);

	if (rslt == true) {
		return 0;
	} else {
		client.p2pRemoveTimerTask(timer_id);
		return -1;
	}
}

LIB_API int rmReportMake (const char *modName, const char *msgName, const char *msgBody)
{
	RabbitMQWrapper& client = RabbitMQWrapper::getInstance(p2ptype);

	if ((modName == NULL) || (msgName == NULL) || (msgBody == NULL)) {
		return -1;
	}
	
	amqp_basic_properties_t props;
    amqp_table_t headers;
    amqp_table_entry_t entries[PROP_HEAD_LEN];
	struct timespec ts;

    // 设置消息属性
    memset(&props, 0, sizeof(props));
    props.content_type = amqp_cstring_bytes("text/plain");

    // 设置附加的头部信息
	headers.entries = entries;
	int_props_st intprops;

	intprops.sender = client.GetModuleName();
	
	intprops.receiver = modName;
	intprops.msgtype = RB_REPORT;
	intprops.msgname = msgName;
	intprops.rid = -1;
	intprops.inarg = nullptr;
	
    clock_gettime(CLOCK_REALTIME, &ts);  // 获取当前时间（秒和纳秒）
    int64_t timestamp_ms = (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	intprops.timestamp = timestamp_ms;
	intprops.rspcode = "no code";
	intprops.rspdesc = "no description";
	intprops.trans = "no transparent";

	client.IntProps2AmqpTable(intprops, headers);
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_HEADERS_FLAG;
	props.headers = headers;

	bool rslt = client.SendRabbitmqMessage(std::string(modName), std::string(msgBody), &props);

	if (rslt == true) {
		return 0;
	} else {
		return -1;
	}
}

LIB_API int rmRequestTempResponse  (const char *modName, int rpid, const char *respCode, const char *respDescr, const char *msgBody)
{
	RabbitMQWrapper& client = RabbitMQWrapper::getInstance(p2ptype);

	if ((respCode == NULL) || (msgBody == NULL) || (modName == NULL)) {
		return -1;
	}
	std::string receiver = std::string(modName);

	amqp_basic_properties_t props;
    amqp_table_t headers;
    amqp_table_entry_t entries[PROP_HEAD_LEN];
	struct timespec ts;

    // 设置消息属性
    memset(&props, 0, sizeof(props));
    props.content_type = amqp_cstring_bytes("text/plain");

    // 设置附加的头部信息
	headers.entries = entries;
	int_props_st intprops;

	intprops.sender = client.GetModuleName();
	
	intprops.receiver = receiver;
	intprops.msgtype = RB_TEMPORARY_RESPONSE;
	intprops.msgname = "temprsp";
	intprops.rid = rpid;
	intprops.inarg = nullptr;
	
    clock_gettime(CLOCK_REALTIME, &ts);  // 获取当前时间（秒和纳秒）
    int64_t timestamp_ms = (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	intprops.timestamp = timestamp_ms;
	intprops.rspcode = respCode;
	intprops.rspdesc = respDescr;

	intprops.trans = "no transparent";

	client.IntProps2AmqpTable(intprops, headers);
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_HEADERS_FLAG;
	props.headers = headers;

	bool rslt = client.SendRabbitmqMessage(receiver, std::string(msgBody), &props);	
	if (rslt == true) {
		return 0;
	} else {
		return -1;
	}
}

LIB_API int rmRequestFinalResponse  (const char *modName, int rpid, const char *respCode, const char *respDescr, const char *msgBody)
{
	RabbitMQWrapper& client = RabbitMQWrapper::getInstance(p2ptype);

	if ((respCode == NULL) || (msgBody == NULL) || (modName == NULL)) {
		return -1;
	}
	std::string receiver = std::string(modName);

	amqp_basic_properties_t props;
    amqp_table_t headers;
    amqp_table_entry_t entries[PROP_HEAD_LEN];
	struct timespec ts;

    // 设置消息属性
    memset(&props, 0, sizeof(props));
    props.content_type = amqp_cstring_bytes("text/plain");

    // 设置附加的头部信息
	headers.entries = entries;
	int_props_st intprops;

	intprops.sender = client.GetModuleName();
	
	intprops.receiver = receiver;
	intprops.msgtype = RB_FINAL_RESPONSE;
	intprops.msgname = "finalrsp";
	intprops.rid = rpid;
	intprops.inarg = nullptr;
	
    clock_gettime(CLOCK_REALTIME, &ts);  // 获取当前时间（秒和纳秒）
    int64_t timestamp_ms = (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	intprops.timestamp = timestamp_ms;
	intprops.rspcode = respCode;
	intprops.rspdesc = respDescr;

	intprops.trans = "no transparent";

	client.IntProps2AmqpTable(intprops, headers);
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_HEADERS_FLAG;
	props.headers = headers;

	bool rslt = client.SendRabbitmqMessage(receiver, std::string(msgBody), &props);	

	if (rslt == true) {
		return 0;
	} else {
		return -1;
	}
}


int rmInitMidwareClient (const char *modName, const char *serverIp, unsigned short serverPort, const char *userName, const char *passWord, bool svrmod)
{
	if ((modName == NULL) || (serverIp == NULL) || (serverIp == NULL) || (userName == NULL) || (passWord == NULL)) {
		return -1;
	}

	RabbitMQWrapper& client = RabbitMQWrapper::getInstance(p2ptype);

	bool rslt = client.rabbitmq_init(std::string(serverIp), serverPort, std::string(userName), std::string(passWord), std::string(modName));
	
	if (rslt == true) {
		return 0;
	} else {
		return -1;
	}
}


int rmSetMidMsgHandler(MID_CLIENT_HANDLER *pfn)
{
	RabbitMQWrapper& client = RabbitMQWrapper::getInstance(p2ptype);

	client.p2pSetCallback(*pfn);

	return 0;
}


void MidwareClientDestroy()
{
	RabbitMQWrapper& client = RabbitMQWrapper::getInstance(p2ptype);

	client.rabbitmq_free();
}


