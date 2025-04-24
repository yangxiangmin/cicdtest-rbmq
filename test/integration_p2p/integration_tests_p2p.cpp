#include "rabbitmqclient.h"
#include <gtest/gtest.h>
#include <atomic>

std::atomic<bool> report_received(false);

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
