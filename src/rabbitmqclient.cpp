#include "rabbitmqclient.h"
#include "rbloger.h"
#include <chrono>


// 静态成员初始化
static std::mutex instanceMutex;	// getInstance 操作线程安全
std::unique_ptr<RabbitMQWrapper> RabbitMQWrapper::instance = nullptr;
ExchangeType RabbitMQWrapper::instanceType = p2ptype; // 默认值

RbmqLogger logger;

// 定义 getInstance 函数
//static RabbitMQWrapper& RabbitMQWrapper::getInstance(ExchangeType type) {
RabbitMQWrapper& RabbitMQWrapper::getInstance(ExchangeType type) {
	std::lock_guard<std::mutex> lock(instanceMutex);
    if (instance) {
        if (instanceType != type) {
            throw std::runtime_error("RabbitMQWrapper instance already created with a different type!");
        }
    } else {
        instance = std::unique_ptr<RabbitMQWrapper>(new RabbitMQWrapper(type));
        instanceType = type; // 动态设置 instanceType
    }
    return *instance;
}

// 辅助函数：将 amqp_bytes_t 转换为 std::string
std::string amqp_bytes_to_string(const amqp_bytes_t& bytes) {
    if (bytes.bytes && bytes.len > 0) {
        //return std::string(static_cast<char*>(bytes.bytes), bytes.len);
        return std::string(reinterpret_cast<char*>(bytes.bytes), bytes.len);
    }
    return "";
}

bool RabbitMQWrapper::AmqpTable2IntProps(const amqp_table_t& table, int_props_st& intprops) {
    for (int i = 0; i < table.num_entries; ++i) {
        const amqp_table_entry_t& entry = table.entries[i];
        std::string key = amqp_bytes_to_string(entry.key);

        if (key == PROP_SENDER_STR) {
            if (entry.value.kind == AMQP_FIELD_KIND_BYTES) {
                intprops.sender = amqp_bytes_to_string(entry.value.value.bytes);
            } else {
				logger.Log("Invalid type for sender: expected bytes");
                return false;
            }
        } else if (key == PROP_RECVIVER_STR) {
            if (entry.value.kind == AMQP_FIELD_KIND_BYTES) {
                intprops.receiver = amqp_bytes_to_string(entry.value.value.bytes);
            } else {
				logger.Log("Invalid type for receiver: expected bytes");
                return false;
            }
        } else if (key == PROP_MSGTYPE_STR) {
            if (entry.value.kind == AMQP_FIELD_KIND_I32) {
                intprops.msgtype = static_cast<message_type_t>(entry.value.value.i32);
            } else {
				logger.Log("Invalid type for msgtype: expected i32");
                return false;
            }
        } else if (key == PROP_MSGNAME_STR) {
            if (entry.value.kind == AMQP_FIELD_KIND_BYTES) {
                intprops.msgname = amqp_bytes_to_string(entry.value.value.bytes);
            } else {
				logger.Log("Invalid type for msgname: expected bytes");
                return false;
            }
        } else if (key == PROP_RELATID_STR) {
            if (entry.value.kind == AMQP_FIELD_KIND_I32) {
                intprops.rid = entry.value.value.i32;
            } else {
				logger.Log("Invalid type for rid: expected i32");
                return false;
            }
        } else if (key == PROP_INARG_STR) {
            if (entry.value.kind == AMQP_FIELD_KIND_I64) {
                intprops.inarg = reinterpret_cast<void*>(entry.value.value.i64);
            } else {
				logger.Log("Invalid type for inarg: expected i64");
                return false;
            }
        } else if (key == PROP_SENDTIME_STR) {
            if (entry.value.kind == AMQP_FIELD_KIND_I64) {
                intprops.timestamp = entry.value.value.i64;
            } else {
				logger.Log("Invalid type for timestamp: expected i64");
                return false;
            }
        } else if (key == PROP_RESPCODE_STR) {
            if (entry.value.kind == AMQP_FIELD_KIND_BYTES) {
                intprops.rspcode = amqp_bytes_to_string(entry.value.value.bytes);;
            } else {
				logger.Log("Invalid type for rspcode: expected bytes");
                return false;
            }
        } else if (key == PROP_RESPDESC_STR) {
            if (entry.value.kind == AMQP_FIELD_KIND_BYTES) {
                intprops.rspdesc = amqp_bytes_to_string(entry.value.value.bytes);
            } else {
				logger.Log("Invalid type for rspdesc: expected bytes");
                return false;
            }
        } else if (key == PROP_TRANS_STR) {
            if (entry.value.kind == AMQP_FIELD_KIND_BYTES) {
                intprops.trans = amqp_bytes_to_string(entry.value.value.bytes);
            } else {
				logger.Log("Invalid type for transparent: expected bytes");
                return false;
            }
        } else {
			std::string logstring = "Unknown key: " + key;
			logger.Log(logstring);
            return false;
        }
    }

    return true;
}


bool RabbitMQWrapper::IntProps2AmqpTable(const int_props_st& intprops, amqp_table_t& table) {
    int index = 0;

    // 添加 sender
    table.entries[index].key = amqp_cstring_bytes(PROP_SENDER_STR);
    table.entries[index].value.kind = AMQP_FIELD_KIND_BYTES;
    table.entries[index].value.value.bytes = amqp_cstring_bytes(intprops.sender.c_str());
    index++;
	
	// 添加receiver
    table.entries[index].key = amqp_cstring_bytes(PROP_RECVIVER_STR);
    table.entries[index].value.kind = AMQP_FIELD_KIND_BYTES;
    table.entries[index].value.value.bytes = amqp_cstring_bytes(intprops.receiver.c_str());
    index++;
	
    // 添加 msgtype
    table.entries[index].key = amqp_cstring_bytes(PROP_MSGTYPE_STR);
    table.entries[index].value.kind = AMQP_FIELD_KIND_I32;
    table.entries[index].value.value.i32 = static_cast<int32_t>(intprops.msgtype);
    index++;

    // 添加 msgname
    table.entries[index].key = amqp_cstring_bytes(PROP_MSGNAME_STR);
    table.entries[index].value.kind = AMQP_FIELD_KIND_BYTES;
    table.entries[index].value.value.bytes = amqp_cstring_bytes(intprops.msgname.c_str());
    index++;

    // 添加 rid
    table.entries[index].key = amqp_cstring_bytes(PROP_RELATID_STR);
    table.entries[index].value.kind = AMQP_FIELD_KIND_I32;
    table.entries[index].value.value.i32 = intprops.rid;
    index++;

    // 添加 inarg
    table.entries[index].key = amqp_cstring_bytes(PROP_INARG_STR);
    table.entries[index].value.kind = AMQP_FIELD_KIND_I64;
    table.entries[index].value.value.i64 = reinterpret_cast<int64_t>(intprops.inarg);
    index++;

    // 添加 timestamp
    table.entries[index].key = amqp_cstring_bytes(PROP_SENDTIME_STR);
    table.entries[index].value.kind = AMQP_FIELD_KIND_I64;
    table.entries[index].value.value.i64 = intprops.timestamp;
    index++;

    // 添加 rspcode
    table.entries[index].key = amqp_cstring_bytes(PROP_RESPCODE_STR);
    table.entries[index].value.kind = AMQP_FIELD_KIND_BYTES;
    table.entries[index].value.value.bytes = amqp_cstring_bytes(intprops.rspcode.c_str());
    index++;

    // 添加 rspdesc
    table.entries[index].key = amqp_cstring_bytes(PROP_RESPDESC_STR);
    table.entries[index].value.kind = AMQP_FIELD_KIND_BYTES;
    table.entries[index].value.value.bytes = amqp_cstring_bytes(intprops.rspdesc.c_str());
    index++;

    // 添加 transinfo
    table.entries[index].key = amqp_cstring_bytes(PROP_TRANS_STR);
    table.entries[index].value.kind = AMQP_FIELD_KIND_BYTES;
    table.entries[index].value.value.bytes = amqp_cstring_bytes(intprops.trans.c_str());
    index++;

    // 更新 table 的条目数量
    table.num_entries = index;

    return true;
}


// Constructor
RabbitMQWrapper::RabbitMQWrapper(ExchangeType type)
    : exchange_type(type) {
    stopThreads = false;
	connection = nullptr;
	socket = nullptr;
	channel = -1;
	initialized = false;
	m_next_timer_id = 0;
	heartbeaterr = 0;

	std::string logstring = "start use RabbitMQWrapper: " + std::to_string(type) + ", " + GetVersion();
	logger.Log(logstring);
}

// Destructor
RabbitMQWrapper::~RabbitMQWrapper() {
	rabbitmq_free();
}

void RabbitMQWrapper::rabbitmq_free() {
    // Cleanup and close connection
    if (initialized) {
		std::cout << "start wrapper destroy." << std::endl;
		logger.Log("start wrapper destroy.");
		stopThreads = true;
		
		// Join threads
		if (send_thread.joinable()) send_thread.join();
		logger.Log("send_thread complete.");
		if (recv_thread.joinable()) recv_thread.join();
		logger.Log("recv_thread complete.");
		if (timer_thread.joinable()) timer_thread.join();
		logger.Log("timer_thread complete.");

		if (connection != nullptr) {
		    std::lock_guard<std::mutex> conn_lock(conn_mutex);
		    amqp_connection_close(connection, AMQP_REPLY_SUCCESS);
		    amqp_destroy_connection(connection);
			connection = nullptr;
		}
		logger.Log("close socket and destroy connect complete.");

		initialized = false;
		std::cout << "wrapper destroy complete." << std::endl;
		logger.Log("RabbitMQWrapper destroy complete.");
    }
}

ExchangeType RabbitMQWrapper::GetInstanceType() const {
	return instanceType;
}

bool RabbitMQWrapper::rabbitmq_init(const std::string &host, int port, const std::string &username, const std::string &password, const std::string &module_name) {

	if (initialized) {
		logger.Log("RabbitMQWrapper has already been initialized!");
        return false;
    }

	m_host = host;
	m_port = port;
	m_username = username;
	m_password = password;
	m_module_name = module_name;
	
    // Initialize connection
    if (!ConnectInit()) {
		return false;
	
}

    // Start threads for sending and receiving
    send_thread = std::thread(&RabbitMQWrapper::SendThreadProc, this);
    recv_thread = std::thread(&RabbitMQWrapper::ReceiveThreadProc, this);
	timer_thread = std::thread(&RabbitMQWrapper::TimerThreadProc, this);
	initialized = true;
	
	return true;
}

bool RabbitMQWrapper::subscribleTopicMsg(const std::string& topic, CallbackFunction callback, const char* instance) {
	if (exchange_type != topictype) {
		return false;
	}
	
	return topicAddBind(topic, instance, callback);
}

bool RabbitMQWrapper::unSubscribleTopicMsg(const std::string &topic) {
	if (exchange_type != topictype) {
		return false;
	}

	return topicRemoveBinding(topic);
}

bool RabbitMQWrapper::publishTopicMsg(const std::string &message, const std::string &topic, const std::string &transinfo) {
	amqp_basic_properties_t props;
    amqp_table_t headers;
    amqp_table_entry_t entries[PROP_HEAD_LEN];
	struct timespec ts;
	
	if (exchange_type != topictype) {
		return false;
	}

    // 设置消息属性
    memset(&props, 0, sizeof(props));
    props.content_type = amqp_cstring_bytes("text/plain");

    // 设置附加的头部信息
	headers.entries = entries;
	int_props_st intprops;

	intprops.sender = m_module_name;
	intprops.receiver = "subscriber";
	intprops.msgtype = RB_TOPIC;
	intprops.msgname = topic;
	intprops.rid = -1;
	intprops.inarg = nullptr;
	
    clock_gettime(CLOCK_REALTIME, &ts);  // 获取当前时间（秒和纳秒）
    int64_t timestamp_ms = (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	intprops.timestamp = timestamp_ms;
	intprops.rspcode = "no code";
	intprops.rspdesc = "no description";

	if (transinfo.empty()) {
		intprops.trans = "no transparent";
	} else {
		intprops.trans =  transinfo;
	}

	bool rslt = IntProps2AmqpTable(intprops, headers);
	if(rslt == true) {
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_HEADERS_FLAG;
		props.headers = headers;
	}
	else {
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG;
	}
	
	return SendRabbitmqMessage(topic, message, &props);
}

bool RabbitMQWrapper::publishFanoutMsg(const std::string &message) {
	amqp_basic_properties_t props;
    amqp_table_t headers;
    amqp_table_entry_t entries[PROP_HEAD_LEN];
	struct timespec ts;
	
	if (exchange_type != broadcasttype) {
		return false;
	}

    // 设置消息属性
    memset(&props, 0, sizeof(props));
    props.content_type = amqp_cstring_bytes("text/plain");

    // 设置附加的头部信息
	headers.entries = entries;
	int_props_st intprops;

	intprops.sender = m_module_name;
	intprops.receiver = "allclient";
	intprops.msgtype = RB_BROADCAST;
	intprops.msgname = "broadcast";
	intprops.rid = -1;
	intprops.inarg = nullptr;
	
    clock_gettime(CLOCK_REALTIME, &ts);  // 获取当前时间（秒和纳秒）
    int64_t timestamp_ms = (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	intprops.timestamp = timestamp_ms;
	intprops.rspcode = "no code";
	intprops.rspdesc = "no description";
	intprops.trans = "no transparent";

	bool rslt = IntProps2AmqpTable(intprops, headers);
	if(rslt == true) {
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_HEADERS_FLAG;
		props.headers = headers;
	}
	else {
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG;
	}
	
	return SendRabbitmqMessage(message, &props);
}

// Get m_module_name
std::string RabbitMQWrapper::GetModuleName() const {
	return m_module_name;
}

bool RabbitMQWrapper::MatchRoutingKey(const std::string& pattern, const std::string& routing_key) {
	// 转换通配符为正则表达式
	std::string regex_pattern = pattern;
	
	// 替换 "*" 为 [^\.]+（匹配一个单词）
	size_t pos = 0;
	while ((pos = regex_pattern.find('*', pos)) != std::string::npos) {
		regex_pattern.replace(pos, 1, "[^\\.]+");
		pos += 6;  // "[^\\.]+".length() == 6
	}
	
	// 替换 "#" 为 .*（匹配零个或多个单词）
	pos = 0;
	while ((pos = regex_pattern.find('#', pos)) != std::string::npos) {
		regex_pattern.replace(pos, 1, ".*");
		pos += 2;  // ".*".length() == 2
	}
	
	// 创建正则表达式
	std::regex reg("^" + regex_pattern + "$");
	
	// 使用正则表达式进行匹配
	return std::regex_match(routing_key, reg);
}

void RabbitMQWrapper::topicRegisterCallback(const std::string& routing_key_pattern, CallbackFunction callback) {
    routing_key_callbacks[routing_key_pattern] = callback;
}

void RabbitMQWrapper::topicUnregisterCallback(const std::string& routing_key_pattern) {
     routing_key_callbacks.erase(routing_key_pattern);
}

void RabbitMQWrapper::topicHandleMessage(const std::string& routing_key, const std::string& message, const int_props_st& intprops) {
	for (const auto& binding : routing_key_callbacks) {
		const std::string& pattern = binding.first;
		if (MatchRoutingKey(pattern, routing_key)) {
			//std::cout << "MatchRoutingKey: " << pattern << "  " << routing_key << std::endl;
			binding.second(routing_key, message, intprops.sender, intprops.trans); // 调用回调函数
			return;
		} else {
			//std::cout << routing_key << " callback function is null" << std::endl;
		}
	}
}

void RabbitMQWrapper::p2pHandleMessage(const std::string& routing_key, const std::string& message, const int_props_st& intprops) {
	if (intprops.msgtype == RB_REQUEST && stClientCBHandle.m_cbRequest != NULL) {
		void * voidPtr = intprops.inarg;
		stClientCBHandle.m_cbRequest("RMQ-BUS", 
					intprops.sender.c_str(),
					intprops.msgname.c_str(),
					intprops.rid,
					message.c_str(),
					&voidPtr);
	} else if (intprops.msgtype == RB_NOTIFICATION && stClientCBHandle.m_cbNotify  != NULL) {
		stClientCBHandle.m_cbNotify("RMQ-BUS", 
					intprops.sender.c_str(),
					intprops.msgname.c_str(),
					message.c_str());

		//自动回响应
		rmRequestFinalResponse(intprops.sender.c_str(), intprops.rid, "200", "Auto Rsp Ok", "{}");
	} else if (intprops.msgtype == RB_REPORT && stClientCBHandle.m_cbReport != NULL) {
		stClientCBHandle.m_cbReport("RMQ-BUS", 
					intprops.sender.c_str(),
					intprops.msgname.c_str(),
					message.c_str());		
	} else if (intprops.msgtype == RB_TEMPORARY_RESPONSE && stClientCBHandle.m_cbRespTemp != NULL) {
		TimerTask *pTaskInfo = p2pGetTimerTaskInfo(intprops.rid);
		if (pTaskInfo != nullptr) {
			stClientCBHandle.m_cbRespTemp("RMQ-BUS", 
						pTaskInfo->intprops.receiver.c_str(),
						pTaskInfo->intprops.msgname.c_str(),
						intprops.rid,
						intprops.rspcode.c_str(),
						intprops.rspdesc.c_str(),
						message.c_str(),
						pTaskInfo->intprops.inarg);

			p2pRestartTimerTask(intprops.rid);
		} else {
			std::string logstring = "tmprsp can not get timer task info,id: " + std::to_string(intprops.rid);
			logger.Log(logstring);
		}
	} else if (intprops.msgtype == RB_FINAL_RESPONSE && stClientCBHandle.m_cbRespFinal != NULL) {
		TimerTask *pTaskInfo = p2pGetTimerTaskInfo(intprops.rid);
		
		if (pTaskInfo != nullptr) {
			if (pTaskInfo->intprops.msgtype == RB_REQUEST) {
				stClientCBHandle.m_cbRespFinal("RMQ-BUS", 
							pTaskInfo->intprops.receiver.c_str(),
							pTaskInfo->intprops.msgname.c_str(),
							intprops.rid,
							intprops.rspcode.c_str(),
							intprops.rspdesc.c_str(),
							message.c_str(),
							pTaskInfo->intprops.inarg);
			} else if (pTaskInfo->intprops.msgtype == RB_NOTIFICATION) {
				// notify message,do nothing
				// std::cout << "notify receive ok" << std::endl;
			}

			p2pRemoveTimerTask(intprops.rid);
		} else {
			std::string logstring = "finalrsp can not get timer task info,id: " + std::to_string(intprops.rid);
			logger.Log(logstring);
		}	
	}
}

void RabbitMQWrapper::fanoutHandleMessage(const std::string& message, const int_props_st& intprops) {
	std::cout << "receive fanout messages: " << message <<std::endl;
	// 增加相应的回调处理
}

// Setup channel
bool RabbitMQWrapper::SetupExchgQueChannel() {
    switch (exchange_type) {
        case p2ptype:
            channel = P2P_CHAHNEL;
			queue_name = m_module_name + "-" + std::string(RBMQ_P2P_QUE_PRE);
			exchange_name = std::string(RBMQ_P2P_EXCHG);
			rbmq_exchange_type = std::string("direct");
            break;
        case topictype:
            channel = TOPIC_CHANNEL;
			queue_name = m_module_name + "-" + std::string(RBMQ_TOPIC_QUE_PRE);

			if (queue_name_self.empty()) {
				queue_name_self = m_module_name + "-" + topicCrtSelfQueName();
			}
			
			exchange_name = std::string(RBMQ_TOPIC_EXCHG);
			rbmq_exchange_type = std::string("topic");
            break;
        case broadcasttype:
            channel = BROADCAST_CHANNEL;
			queue_name = m_module_name + "-" + std::string(RBMQ_BROAD_QUE_PRE);
			exchange_name = std::string(RBMQ_BROAD_EXCHG);
			rbmq_exchange_type = std::string("fanout");
            break;
        default:
			logger.Log("Unknown exchange type");
            return false;
    }

    // Open channels based on the exchange type
    amqp_channel_open(connection, channel);
    amqp_rpc_reply_t reply = amqp_get_rpc_reply(connection);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		logger.Log("Failed to open channel");
        return false;
    }

	// Setup for p2p/topic/broadcast exchange and queue
    amqp_queue_declare(connection, channel, amqp_cstring_bytes(queue_name.c_str()), 0, 0, 0, 1, amqp_empty_table);
	if (exchange_type == topictype) {
		amqp_queue_declare(connection, channel, amqp_cstring_bytes(queue_name_self.c_str()), 0, 0, 0, 1, amqp_empty_table);
	}
	
	//amqp_queue_declare(connection, channel, amqp_cstring_bytes(queue_name.c_str()), 0, 1, 0, 0, amqp_empty_table);
	amqp_exchange_declare(connection, channel, amqp_cstring_bytes(exchange_name.c_str()), amqp_cstring_bytes(rbmq_exchange_type.c_str()), 0, 1, 0, 0, amqp_empty_table);

	if (exchange_type == p2ptype) {
		amqp_queue_bind(connection, channel, amqp_cstring_bytes(queue_name.c_str()), amqp_cstring_bytes(exchange_name.c_str()), amqp_cstring_bytes(m_module_name.c_str()), amqp_empty_table);
		bound_routing_keys_share.insert(m_module_name);
	} else if (exchange_type == broadcasttype) {
		amqp_queue_bind(connection, channel, amqp_cstring_bytes(queue_name.c_str()), amqp_cstring_bytes(exchange_name.c_str()), amqp_empty_bytes, amqp_empty_table);	//队列不绑定指定路由字
	}
	
	return true;
}

// Timer service logic
void RabbitMQWrapper::TimerThreadProc() {
    while (!stopThreads) {
        {
            std::lock_guard<std::mutex> lock(timer_mutex);
            auto now = std::chrono::steady_clock::now();

            // 用于存储需要删除的任务ID
            std::vector<int> tasks_to_remove;

            // 遍历任务列表，执行到期的任务
            for (auto& task : timer_tasks) {
                if (now >= task.expire_time) {
					// 将超时任务标记为待删除
					tasks_to_remove.push_back(task.id);
                }
            }

            // 删除所有超时的任务
			if (!tasks_to_remove.empty()) {
				auto it = timer_tasks.begin();
				while (it != timer_tasks.end()) {
					if (std::find(tasks_to_remove.begin(), tasks_to_remove.end(), it->id) != tasks_to_remove.end()) {
						p2pTimeoutProc(*it);
						it = timer_tasks.erase(it); // 删除任务并更新迭代器
						} else {
						++it; // 继续检查下一个任务
						}
				}
			}
        }

        // 检查任务的间隔时间
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}



// Send message logic
void RabbitMQWrapper::SendThreadProc() {
	int count = 0;
    while (!stopThreads) {
		//std::this_thread::sleep_for(std::chrono::seconds(MAX_HEARTBEAT_TM));
		std::this_thread::sleep_for(std::chrono::seconds(1));
		count ++;
		if (count < MAX_HEARTBEAT_TM) {
			continue;
		} else {
			count = 0;
		}

		if (connection != nullptr)
    	{
    		std::lock_guard<std::mutex> conn_lock(conn_mutex);
			if (amqp_get_sockfd(connection) == -1) {
				heartbeaterr ++;
			} else {
				heartbeaterr = 0;
			}
		}
#if 0
		if (exchange_type == p2ptype) {
	        // message sending logic
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

			intprops.sender = m_module_name;
			
			intprops.receiver = m_module_name;
			intprops.msgtype = RB_ECHOTEST;
			intprops.msgname = "echo-test";
			intprops.rid = -1;
			intprops.inarg = nullptr;
			
		    clock_gettime(CLOCK_REALTIME, &ts);  // 获取当前时间（秒和纳秒）
		    int64_t timestamp_ms = (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

			intprops.timestamp = timestamp_ms;
			intprops.rspcode = "no code";
			intprops.rspdesc = "no description";
			intprops.trans = "no transparent";

			IntProps2AmqpTable(intprops, headers);
			props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_HEADERS_FLAG;
			props.headers = headers;

			SendRabbitmqMessage(m_module_name, "{}", &props);
			logger.Log("send echo test...");
		}
#endif
    }
}

// Receive message logic
void RabbitMQWrapper::ReceiveThreadProc() {
    amqp_rpc_reply_t reply;
	struct timeval timeout = { 0, 100 };

thread_restart:
	{
		std::lock_guard<std::mutex> conn_lock(conn_mutex);
		heartbeaterr = 0;
		amqp_basic_consume(connection, channel, amqp_cstring_bytes(queue_name.c_str()), amqp_cstring_bytes("share"), 0, 1, 0, amqp_empty_table);
		if (exchange_type == topictype) {
			amqp_basic_consume(connection, channel, amqp_cstring_bytes(queue_name_self.c_str()), amqp_cstring_bytes("self"), 0, 1, 0, amqp_empty_table);
		}
	}
	reply = amqp_get_rpc_reply(connection);
	if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		std::string logstring = "amqp_basic_consume err: " + queue_name;
		logger.Log(logstring);
		//return;
		std::this_thread::sleep_for(std::chrono::seconds(3));
		ReconnectProc();
		goto thread_restart;
	}
	
    while (!stopThreads) {
        amqp_envelope_t envelope;
        amqp_maybe_release_buffers(connection);
	
        {
			std::lock_guard<std::mutex> conn_lock(conn_mutex);
			
			if (heartbeaterr > MAX_HEARTBEAT_ERR) {
				std::string logstring = "heartbeat error: " + std::to_string(heartbeaterr);
				logger.Log(logstring);

				ReconnectProc();
				goto thread_restart;
			}
			
	        //reply = amqp_consume_message(connection, &envelope, nullptr, 0);
	        reply = amqp_consume_message(connection, &envelope, &timeout, 0);
        }
		if (reply.reply.id == AMQP_BASIC_GET_EMPTY_METHOD) {
			// no message
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		else if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
			//AMQP_STATUS_SOCKET_ERROR
			//AMQP_STATUS_TIMEOUT
			if (reply.reply_type == AMQP_RESPONSE_LIBRARY_EXCEPTION) {
				if (reply.library_error == AMQP_STATUS_TIMEOUT)	{
					// receive timeout
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
				else {
					// receive error
					std::string logstring = "AMQP_RESPONSE_LIBRARY_EXCEPTION: " + std::to_string(reply.library_error) + "[" + amqp_error_string2(reply.library_error) + "]";
					logger.Log(logstring);
					
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					ReconnectProc();
					goto thread_restart;
				}
			}
			else {
				//AMQP_RESPONSE_SERVER_EXCEPTION
				logger.Log("AMQP_RESPONSE_SERVER_EXCEPTION");
			
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				ReconnectProc();
				goto thread_restart;
			}
		}
		else {
			int_props_st intprops;
			bool prop_ok = false;
		
			std::string routing_key(reinterpret_cast<char*>(envelope.routing_key.bytes), envelope.routing_key.len);
            std::string message(reinterpret_cast<char*>(envelope.message.body.bytes), envelope.message.body.len);
			std::string consumer_name(reinterpret_cast<char*>(envelope.consumer_tag.bytes), envelope.consumer_tag.len);
            //std::cout << "received message:" << message << ",routing_key:" << routing_key << ",consumer:" << consumer_name <<std::endl;
			
			if (envelope.message.properties._flags & AMQP_BASIC_HEADERS_FLAG) {
				if (!AmqpTable2IntProps(envelope.message.properties.headers, intprops)) {
					logger.Log("AmqpTable2IntProps err!");
				} else {
					prop_ok = true;
				}
			} else {
				logger.Log("no heads field err!");
			}
			
            amqp_destroy_envelope(&envelope);

			if (prop_ok) {
				if (exchange_type == topictype) {
					std::lock_guard<std::mutex> conn_lock(handle_mutex);
					topicHandleMessage(routing_key, message, intprops);
				} else if (exchange_type == p2ptype) {
					std::lock_guard<std::mutex> conn_lock(handle_mutex);
					p2pHandleMessage(routing_key, message, intprops);
				} else if (exchange_type == broadcasttype) {
					std::lock_guard<std::mutex> conn_lock(handle_mutex);
					fanoutHandleMessage(message, intprops);
				} else {
					std::string logstring = "Unknow exchange type :" + std::to_string(exchange_type);
					logger.Log(logstring);
				}
			}
        }
    }
}

// Return exchange name based on type
std::string RabbitMQWrapper::GetExchangeName() const {
	return exchange_name;
}

std::string RabbitMQWrapper::topicCrtSelfQueName() {
	char hostname[256];
	gethostname(hostname, sizeof(hostname));
	
	pid_t pid = getpid();
	
	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);
	char timestamp[20];
	strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", tm_info);
	
	std::string unique_name = std::string(hostname) + "-" + std::to_string(pid) + "-" + timestamp;
	
	return unique_name;
}

bool RabbitMQWrapper::topicAddBind(const std::string& routing_key, const char* instance) {
    if (exchange_type != topictype) {
        return false;
    }
    {
	    std::lock_guard<std::mutex> conn_lock(conn_mutex);
		if (instance == nullptr) {
		    amqp_queue_bind(connection, channel, amqp_cstring_bytes(queue_name.c_str()),
		                    amqp_cstring_bytes(exchange_name.c_str()),
		                    amqp_cstring_bytes(routing_key.c_str()),
		                    amqp_empty_table);
			bound_routing_keys_share.insert(routing_key);
		} else {
		    amqp_queue_bind(connection, channel, amqp_cstring_bytes(queue_name_self.c_str()),
		                    amqp_cstring_bytes(exchange_name.c_str()),
		                    amqp_cstring_bytes(routing_key.c_str()),
		                    amqp_empty_table);
			bound_routing_keys_self.insert(routing_key);
		} 
    }
	return true;
}

bool RabbitMQWrapper::topicAddBind(const std::string& routing_key, const char* instance, CallbackFunction callback) {
    if (exchange_type != topictype) {
        return false;
    }
	if (routing_key.empty()) {
		return false;
	}
	
    {
	    std::lock_guard<std::mutex> conn_lock(conn_mutex);
		if (instance == nullptr) {
		    amqp_queue_bind(connection, channel, amqp_cstring_bytes(queue_name.c_str()),
		                    amqp_cstring_bytes(exchange_name.c_str()),
		                    amqp_cstring_bytes(routing_key.c_str()),
		                    amqp_empty_table);
			bound_routing_keys_share.insert(routing_key);
		} else {
		    amqp_queue_bind(connection, channel, amqp_cstring_bytes(queue_name_self.c_str()),
		                    amqp_cstring_bytes(exchange_name.c_str()),
		                    amqp_cstring_bytes(routing_key.c_str()),
		                    amqp_empty_table);
			bound_routing_keys_self.insert(routing_key);
		}

		topicRegisterCallback(routing_key, callback);
    }
	return true;
}

bool RabbitMQWrapper::topicRemoveBinding(const std::string& routing_key) {
    if (exchange_type != topictype) {
        return false;
    }
	
	if (routing_key.empty()) {
		return false;
	}
	
    {
	    std::lock_guard<std::mutex> conn_lock(conn_mutex);
		
		auto it = bound_routing_keys_share.find(routing_key);
		if (it != bound_routing_keys_share.end()) {
			amqp_queue_unbind(connection, channel, amqp_cstring_bytes(queue_name.c_str()),
							  amqp_cstring_bytes(exchange_name.c_str()),
							  amqp_cstring_bytes(routing_key.c_str()),
							  amqp_empty_table);
			bound_routing_keys_share.erase(routing_key);
			topicUnregisterCallback(routing_key);
			return true;
		}

		it = bound_routing_keys_self.find(routing_key);
		if (it != bound_routing_keys_self.end()) {
			amqp_queue_unbind(connection, channel, amqp_cstring_bytes(queue_name_self.c_str()),
							  amqp_cstring_bytes(exchange_name.c_str()),
							  amqp_cstring_bytes(routing_key.c_str()),
							  amqp_empty_table);
			bound_routing_keys_self.erase(routing_key);
			topicUnregisterCallback(routing_key);
			return true;
		}		
    }
	
	return false;
}

bool RabbitMQWrapper::SendRabbitmqMessage(const std::string& routing_key, const std::string& msg, const amqp_basic_properties_t* inprops) {
	amqp_rpc_reply_t reply;
	amqp_basic_properties_t props;

	if (routing_key.empty() || connection == nullptr) {
		return false;
	}
	
	if (inprops == nullptr) {
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG;
		props.content_type = amqp_cstring_bytes("text/plain");
	} else {
		props = *inprops;
	}
	
	{
		std::lock_guard<std::mutex> lock(conn_mutex);
	    amqp_basic_publish(connection, channel, amqp_cstring_bytes(exchange_name.c_str()), amqp_cstring_bytes(routing_key.c_str()), 0, 0, &props, amqp_cstring_bytes(msg.c_str()));
		reply = amqp_get_rpc_reply(connection);
    }

	// reply = amqp_get_rpc_reply(connection);  // 20250402 修正
	if (reply.reply_type == AMQP_RESPONSE_NORMAL) {
		//std::cout << "Message published successfully." << std::endl;
		return true;
	} else {
		std::string logstring = "Publish failed: " + std::to_string(reply.reply.id);
		logger.Log(logstring);
		return false;
	}
}

bool RabbitMQWrapper::SendRabbitmqMessage(const std::string& msg, const amqp_basic_properties_t* inprops) {
	amqp_rpc_reply_t reply;
	amqp_basic_properties_t props;

	if (connection == nullptr) {
		return false;
	}

	if (exchange_type != broadcasttype) {
		return false;
	}
	
	if (inprops == nullptr) {
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG;
		props.content_type = amqp_cstring_bytes("text/plain");
	} else {
		props = *inprops;
	}
	
	{
		std::lock_guard<std::mutex> lock(conn_mutex);
	    amqp_basic_publish(connection, channel, amqp_cstring_bytes(exchange_name.c_str()), amqp_cstring_bytes(""), 0, 0, &props, amqp_cstring_bytes(msg.c_str()));
		reply = amqp_get_rpc_reply(connection);
    }
	
	// reply = amqp_get_rpc_reply(connection);  // 20250402 修正
	if (reply.reply_type == AMQP_RESPONSE_NORMAL) {
		//std::cout << "Message published successfully." << std::endl;
		return true;
	} else {
		std::string logstring = "Publish failed: " + std::to_string(reply.reply.id);
		logger.Log(logstring);
		return false;
	}
}

bool RabbitMQWrapper::ConnectInit() {
	if (connection != nullptr) {
		logger.Log("RabbitMQWrapper has already been connected!");
		return false;
	}
	
    connection = amqp_new_connection();
    socket = amqp_tcp_socket_new(connection);
    if (amqp_socket_open(socket, m_host.c_str(), m_port) != AMQP_STATUS_OK) {
		logger.Log("Error opening socket");
        return false;
    }

    // Login to RabbitMQ server
    //amqp_rpc_reply_t reply = amqp_login(connection, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, m_username.c_str(), m_password.c_str());
    amqp_rpc_reply_t reply = amqp_login(connection, "/", 0, 131072, MAX_HEARTBEAT_TM, AMQP_SASL_METHOD_PLAIN, m_username.c_str(), m_password.c_str());
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
		logger.Log("Login failed!");
        return false;
    }

    // Open exchange and queue and channels based on the exchange type
    bool rslt = SetupExchgQueChannel();

	return rslt;
}

void RabbitMQWrapper::ReconnectProc() {
	logger.Log("reconnect proc...");
	
	std::lock_guard<std::mutex> conn_lock(conn_mutex);
    if (connection != nullptr) {
	    amqp_connection_close(connection, AMQP_REPLY_SUCCESS);
	    amqp_destroy_connection(connection);
		connection = nullptr;
    }

	if (!ConnectInit()) {
		return;
	}

	// rebound
	if (exchange_type == topictype) {
		for (const auto& key : bound_routing_keys_share) {
			amqp_queue_bind(connection, channel, amqp_cstring_bytes(queue_name.c_str()), amqp_cstring_bytes(exchange_name.c_str()), amqp_cstring_bytes(key.c_str()), amqp_empty_table);
		}
		for (const auto& key : bound_routing_keys_self) {
			amqp_queue_bind(connection, channel, amqp_cstring_bytes(queue_name_self.c_str()), amqp_cstring_bytes(exchange_name.c_str()), amqp_cstring_bytes(key.c_str()), amqp_empty_table);
		}
	} else if (exchange_type == p2ptype) {
		amqp_queue_bind(connection, channel, amqp_cstring_bytes(queue_name.c_str()), amqp_cstring_bytes(exchange_name.c_str()), amqp_cstring_bytes(m_module_name.c_str()), amqp_empty_table);
	} else if (exchange_type == broadcasttype) {
		amqp_queue_bind(connection, channel, amqp_cstring_bytes(queue_name.c_str()), amqp_cstring_bytes(exchange_name.c_str()), amqp_empty_bytes, amqp_empty_table);	//队列不绑定指定路由字
	}

	logger.Log("reconnect proc complete.");
}

std::set<std::string> RabbitMQWrapper::GetBoundRoutingShare() {
    std::lock_guard<std::mutex> lock(conn_mutex);
    return bound_routing_keys_share;
}

std::set<std::string> RabbitMQWrapper::GetBoundRoutingSelf() {
    std::lock_guard<std::mutex> lock(conn_mutex);
    return bound_routing_keys_self;
}

int RabbitMQWrapper::p2pReqTimerId() {
	if (exchange_type != p2ptype)
		return -1;

	std::lock_guard<std::mutex> lock(timer_mutex);
	int timer_id = m_next_timer_id++;
	return timer_id;
}

void RabbitMQWrapper::p2pAddTimerTask(const int &timer_id, const std::string& routing_key, const std::string& message, 
					const int& interval, const int_props_st& props) {
	if (exchange_type != p2ptype)
		return;

	std::lock_guard<std::mutex> lock(timer_mutex);
	TimerTask timer_task(timer_id, routing_key, message, interval, props);
	timer_tasks.push_back(timer_task);
}


void RabbitMQWrapper::p2pRemoveTimerTask(const int& timer_id) {
	if (exchange_type != p2ptype)
		return;
	
	std::lock_guard<std::mutex> lock(timer_mutex);
	auto it = std::find_if(timer_tasks.begin(), timer_tasks.end(), [timer_id](const TimerTask& timer) {
		return timer.id == timer_id;
	});

	if (it != timer_tasks.end()) {
		timer_tasks.erase(it);
	}
}

void RabbitMQWrapper::p2pRestartTimerTask(const int& timer_id) {
	if (exchange_type != p2ptype)
		return;

	std::lock_guard<std::mutex> lock(timer_mutex);
	auto it = std::find_if(timer_tasks.begin(), timer_tasks.end(), [timer_id](const TimerTask& timer) {
		return timer.id == timer_id;
	});

	if (it != timer_tasks.end()) {
		it->expire_time = std::chrono::steady_clock::now() + std::chrono::seconds(it->interval);;
	}
}

TimerTask* RabbitMQWrapper::p2pGetTimerTaskInfo(const int& timer_id) {
	if (exchange_type != p2ptype)
		return nullptr;

	std::lock_guard<std::mutex> lock(timer_mutex);
    for (auto& task : timer_tasks) {
        if (task.id == timer_id) {
            return &task; // 返回指向匹配元素的指针
        }
    }
    return nullptr; // 如果没有找到匹配的元素，返回 nullptr
}

void RabbitMQWrapper::p2pTimeoutProc(const TimerTask& ptimetask) {
	if (exchange_type != p2ptype)
		return;

	std::string logstring = "timerout, id: " + std::to_string(ptimetask.id);
	logger.Log(logstring);

	// 执行超时任务
#if 0
	std::cout << "timeout, id: " << ptimetask.id << std::endl;
	std::cout << "sender: " << ptimetask.intprops.sender << std::endl;
	std::cout << "receiver: " << ptimetask.intprops.receiver << std::endl;
	std::cout << "msgtype: " << ptimetask.intprops.msgtype << std::endl;
	std::cout << "msgname: " << ptimetask.intprops.msgname << std::endl;
	std::cout << "rpid: " << ptimetask.intprops.rid << std::endl;
	std::cout << "inarg: " << ptimetask.intprops.inarg << std::endl;
	std::cout << "timestamp: " << ptimetask.intprops.timestamp << std::endl;
	std::cout << "rspcode: " << ptimetask.intprops.rspcode << std::endl;
	std::cout << "rspdesc: " << ptimetask.intprops.rspdesc << std::endl;
	std::cout << "transinfo: " << ptimetask.intprops.trans << std::endl;
#endif

	if((ptimetask.intprops.msgtype == RB_REQUEST) && (stClientCBHandle.m_cbRequestTimeout) != NULL)	{
		stClientCBHandle.m_cbRequestTimeout("RMQ-BUS", 
									ptimetask.intprops.receiver.c_str(),
									ptimetask.intprops.msgname.c_str(),
									ptimetask.intprops.rid,
									ptimetask.intprops.inarg);
	}else if((ptimetask.intprops.msgtype == RB_NOTIFICATION) && (stClientCBHandle.m_cbNotifyTimeout) != NULL)	{
		stClientCBHandle.m_cbNotifyTimeout("RMQ-BUS", 
									ptimetask.intprops.receiver.c_str(),
									ptimetask.intprops.msgname.c_str(),
									ptimetask.intprops.rid,
									ptimetask.intprops.inarg);
	}
}

void RabbitMQWrapper::p2pSetCallback(const MID_CLIENT_HANDLER& handle) {
	if (exchange_type != p2ptype)
		return;

	stClientCBHandle = handle;
}

std::string RabbitMQWrapper::GetVersion() const {
	//return "RabbitMQWrapper Version 2.1.2.20250402"; // 返回版本信息
	std::string versionInfo = 
	    std::string(VERSION) + " " + 
	    std::string(CMP_IPADDR) + " " + 
	    std::string(BUILD_TIME) + " " + 
	    std::string(ARCH_TYPE);
	return versionInfo;
}

