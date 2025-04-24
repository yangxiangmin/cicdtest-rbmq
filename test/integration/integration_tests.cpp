#include "rabbitmqclient.h"
#include <gtest/gtest.h>
#include <atomic>

std::atomic<bool> message_received(false);
std::atomic<bool> fanout_received(false);
std::atomic<bool> report_received(false);

void topicCallback(const std::string& routing_key, const std::string& message, 
                 const std::string& sender, const std::string& trans) {
    if (routing_key == "test.topic" && message == "test message") {
        message_received = true;
    }
}

void ReportEventHandler(
    const char *serverName,
    const char *srcMod,
    const char *msgName,
    const char *pJsonBody)
{
    if (msgName == "p2p-report-msg") {
        report_received = true;
    }
}

#if 0
void fanoutCallback(const std::string& message, const int_props_st& intprops) {
    if (message == "fanout message") {
        fanout_received = true;
    }
}
#endif

TEST(RabbitMQIntegrationTest, TopicMessageExchange) {
    RabbitMQWrapper& pub_wrapper = RabbitMQWrapper::getInstance(topictype);
    pub_wrapper.rabbitmq_init("localhost", 5672, "guest", "guest", "test_publisher");
    
    RabbitMQWrapper& sub_wrapper = RabbitMQWrapper::getInstance(topictype);
    sub_wrapper.rabbitmq_init("localhost", 5672, "guest", "guest", "test_subscriber");
    
    // 订阅主题
    sub_wrapper.subscribleTopicMsg("test.topic", topicCallback, nullptr);
    
    // 发布消息
    pub_wrapper.publishTopicMsg("test message", "test.topic", "test_trans");
    
    // 等待消息接收
    for (int i = 0; i < 10 && !message_received; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    EXPECT_TRUE(message_received);
    
    pub_wrapper.rabbitmq_free();
    sub_wrapper.rabbitmq_free();
}

TEST(RabbitMQIntegrationTest, P2PMessageExchange) {
    int rslt = rmInitMidwareClient("mod_p2ptest", "localhost", 5672, "guest", "guest", false);

    MID_CLIENT_HANDLER stClientCBHandle = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

    stClientCBHandle.m_cbReport = ReportEventHandler;
    
    rmSetMidMsgHandler(&stClientCBHandle);

    rslt = rmReportMake("mod_p2ptest", "p2p-report-msg", "message body");

    // 等待消息接收
    for (int i = 0; i < 10 && !report_received; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    EXPECT_TRUE(report_received);

    MidwareClientDestroy();
}

#if 0
TEST(RabbitMQIntegrationTest, FanoutMessageExchange) {
    RabbitMQWrapper& wrapper1 = RabbitMQWrapper::getInstance(broadcasttype);
    wrapper1.rabbitmq_init("localhost", 5672, "guest", "guest", "test_fanout1");
    
    RabbitMQWrapper& wrapper2 = RabbitMQWrapper::getInstance(broadcasttype);
    wrapper2.rabbitmq_init("localhost", 5672, "guest", "guest", "test_fanout2");
    
    // 发布消息
    wrapper1.publishFanoutMsg("fanout message");
    
    // 等待消息接收
    for (int i = 0; i < 10 && !fanout_received; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    EXPECT_TRUE(fanout_received);
    
    wrapper1.rabbitmq_free();
    wrapper2.rabbitmq_free();
}
#endif
