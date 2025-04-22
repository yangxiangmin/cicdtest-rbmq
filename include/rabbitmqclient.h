
#ifndef __RABBITMQ_CLIENT_H__
#define __RABBITMQ_CLIENT_H__
#include <iostream>
#include "amqp.h"
#include "amqp_tcp_socket.h"
#include <string>
#include <thread>
#include <mutex>
#include <set>
#include <functional>
#include <regex>
#include <unistd.h>
#include <time.h>
#include "rmMidLib.h"
#include "version.h"

#define MAX_HEARTBEAT_ERR	3
#define MAX_HEARTBEAT_TM	60

typedef enum {
    RB_REQUEST,
    RB_TEMPORARY_RESPONSE,
    RB_FINAL_RESPONSE,
    RB_NOTIFICATION,
    RB_REPORT,
    RB_TOPIC,
    RB_BROADCAST,
    RB_ECHOTEST
} message_type_t;

#define PROP_HEAD_LEN		10
#define PROP_SENDER_STR		"sender"
#define PROP_RECVIVER_STR	"recver"
#define PROP_MSGTYPE_STR	"msgtype"
#define PROP_MSGNAME_STR	"msgname"
#define PROP_RELATID_STR	"rid"
#define PROP_INARG_STR		"inarg"
#define PROP_SENDTIME_STR	"timestamp"
#define PROP_RESPCODE_STR	"rspcode"
#define PROP_RESPDESC_STR	"rspdesc"
#define PROP_TRANS_STR		"transparent"		//透传字符串

typedef struct {
	std::string sender;
	std::string receiver;
	message_type_t msgtype;
	std::string msgname;
	int rid;
	void* inarg;
	int64_t timestamp;
	std::string rspcode;
	std::string rspdesc;
	std::string trans;
}int_props_st;

enum ExchangeType {
    p2ptype,
    topictype,
    broadcasttype
};

#define P2P_CHAHNEL			1
#define TOPIC_CHANNEL		2
#define BROADCAST_CHANNEL	3

#define RBMQ_P2P_EXCHG			"ght.p2p.exchange"
#define RBMQ_TOPIC_EXCHG		"ght.topic.exchange"
#define RBMQ_BROAD_EXCHG		"ght.broadcast.exchange"

#define RBMQ_P2P_QUE_PRE		"p2pmsgque"
#define RBMQ_TOPIC_QUE_PRE		"topicmsgque"
#define RBMQ_BROAD_QUE_PRE		"broadcastmsgque"

using CallbackFunction = std::function<void(const std::string&, const std::string&, const std::string&, const std::string&)>;

struct TimerTask {
	int id;
	std::string routing_key;
	std::string message;
	int interval;
	int_props_st intprops;
	std::chrono::steady_clock::time_point expire_time;
	
	TimerTask(const int& idx, const std::string& key, const std::string& msg, const int& val, const int_props_st& props)
		: id(idx), routing_key(key), message(msg), interval(val), intprops(props) {
			expire_time = std::chrono::steady_clock::now() + std::chrono::seconds(val);
			}
};

class RabbitMQWrapper {
private:
    amqp_connection_state_t connection;
    amqp_socket_t *socket;
	amqp_channel_t channel;
	bool stopThreads;
	bool initialized;
	int heartbeaterr;
	
    std::thread send_thread;
    std::thread recv_thread;
	std::thread timer_thread;
    std::mutex conn_mutex;
	std::mutex handle_mutex;
	std::string m_host;
	int m_port;
	std::string m_username;
	std::string m_password;
	std::string m_module_name;
	ExchangeType exchange_type;
	std::string queue_name;
	std::string queue_name_self;
	std::string exchange_name;
	std::string rbmq_exchange_type;
	MID_CLIENT_HANDLER stClientCBHandle;
	
	std::set<std::string> bound_routing_keys_share;
	std::set<std::string> bound_routing_keys_self;
	std::map<std::string, CallbackFunction> routing_key_callbacks;

    std::vector<TimerTask> timer_tasks; // 定时任务列表
    std::mutex timer_mutex;            // 保护任务列表的互斥锁
    int m_next_timer_id;

    // Send message logic
    void SendThreadProc();

    // Receive message logic
    void ReceiveThreadProc();

	// Timer service logic
	void TimerThreadProc();
	
	bool SetupExchgQueChannel();

	bool MatchRoutingKey(const std::string& pattern, const std::string& routing_key);

	void topicRegisterCallback(const std::string& routing_key_pattern, CallbackFunction callback);

	void topicUnregisterCallback(const std::string& routing_key_pattern);

	void topicHandleMessage(const std::string& routing_key, const std::string& message, const int_props_st& intprops);
	void p2pHandleMessage(const std::string& routing_key, const std::string& message, const int_props_st& intprops);
	void fanoutHandleMessage(const std::string& message, const int_props_st& intprops);
	bool SendRabbitmqMessage(const std::string& msg, const amqp_basic_properties_t* clientprops);	//发送faout类型消息，不带路由关键字

	std::string topicCrtSelfQueName();

    bool topicAddBind(const std::string& routing_key, const char* instance);

	bool topicAddBind(const std::string& routing_key, const char* instance, CallbackFunction callback);

    bool topicRemoveBinding(const std::string& routing_key);

	void p2pTimeoutProc(const TimerTask& ptimetask);
	
	// connect init
	bool ConnectInit();

	void ReconnectProc();
	
    static std::unique_ptr<RabbitMQWrapper> instance; // 使用智能指针管理单例实例
    static ExchangeType instanceType; // 保存实例的类型

    // static Constructor
    RabbitMQWrapper(ExchangeType type);
	
public:
	static RabbitMQWrapper& getInstance(ExchangeType type);

    // Destructor
    ~RabbitMQWrapper();

	// Get bound routing keys
	std::set<std::string> GetBoundRoutingShare();
	std::set<std::string> GetBoundRoutingSelf();

	// rabbitmq client init
	bool rabbitmq_init(const std::string &host, int port, const std::string &username, const std::string &password, const std::string &module_name);
	
	// rabbitmq free
	void rabbitmq_free();

	// subscrible topic message
	bool subscribleTopicMsg(const std::string& topic, CallbackFunction callback, const char* instance);	

	//  unsubscrible topic message
	bool unSubscribleTopicMsg(const std::string &topic);

	bool publishTopicMsg(const std::string &message, const std::string &topic, const std::string &transinfo);

	bool publishFanoutMsg(const std::string &message); 

	// Get m_module_name
	std::string GetModuleName() const;
    std::string GetExchangeName() const;

	ExchangeType GetInstanceType() const;

	bool IntProps2AmqpTable(const int_props_st& intprops, amqp_table_t& table);

	bool AmqpTable2IntProps(const amqp_table_t& table, int_props_st& intprops);	

	// Send rabbitmq messages
	bool SendRabbitmqMessage(const std::string& routing_key, const std::string& msg, const amqp_basic_properties_t* clientprops);

	// Request timer ID
	int p2pReqTimerId();
	void p2pAddTimerTask(const int &timer_id, const std::string& routing_key, const std::string& message, 
						const int& interval, const int_props_st& props);
	void p2pRemoveTimerTask(const int& timer_id);
	void p2pRestartTimerTask(const int& timer_id);
	TimerTask* p2pGetTimerTaskInfo(const int& timer_id);

	void p2pSetCallback(const MID_CLIENT_HANDLER& handle);

    std::string GetVersion() const;
};

#endif

