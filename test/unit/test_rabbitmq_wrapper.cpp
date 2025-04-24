#include "rabbitmqclient.h"
#include "rmMidLib.h"
#include <gtest/gtest.h>
#include <thread>

class RabbitMQWrapperTest : public ::testing::Test {
protected:
    void SetUp() override {
        wrapper = &RabbitMQWrapper::getInstance(p2ptype);
        wrapper->rabbitmq_init("localhost", 5672, "guest", "guest", "test_module");
    }
    
    void TearDown() override {
        wrapper->rabbitmq_free();
    }
    
    RabbitMQWrapper* wrapper;
};

TEST_F(RabbitMQWrapperTest, Initialization) {
    EXPECT_EQ(wrapper->GetInstanceType(), p2ptype);
    EXPECT_EQ(wrapper->GetModuleName(), "test_module");
    EXPECT_EQ(wrapper->GetExchangeName(), RBMQ_P2P_EXCHG);
}

TEST_F(RabbitMQWrapperTest, PropertyConversion) {
    int_props_st intprops;
    intprops.sender = "sender";
    intprops.receiver = "receiver";
    intprops.msgtype = RB_REQUEST;
    intprops.msgname = "test";
    intprops.rid = 123;
    intprops.inarg = nullptr;
    intprops.timestamp = 123456789;
    intprops.rspcode = "200";
    intprops.rspdesc = "OK";
    intprops.trans = "transparent";
    
    amqp_table_t table;
    amqp_table_entry_t entries[PROP_HEAD_LEN];
    table.entries = entries;
    
    EXPECT_TRUE(wrapper->IntProps2AmqpTable(intprops, table));
    
    int_props_st converted;
    EXPECT_TRUE(wrapper->AmqpTable2IntProps(table, converted));
    
    EXPECT_EQ(intprops.sender, converted.sender);
    EXPECT_EQ(intprops.receiver, converted.receiver);
    EXPECT_EQ(intprops.msgtype, converted.msgtype);
    EXPECT_EQ(intprops.msgname, converted.msgname);
    EXPECT_EQ(intprops.rid, converted.rid);
    EXPECT_EQ(intprops.timestamp, converted.timestamp);
    EXPECT_EQ(intprops.rspcode, converted.rspcode);
    EXPECT_EQ(intprops.rspdesc, converted.rspdesc);
    EXPECT_EQ(intprops.trans, converted.trans);
}

// 更多单元测试...