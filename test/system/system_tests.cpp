#include <gtest/gtest.h>
#include <curl/curl.h>
//#include <json/json.h>
#include <thread>
#include "rabbitmqclient.h"

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

TEST(RabbitMQSystemTest, EndToEndMessageFlow) {
    // 启动服务
    system("rabbitmq_wrapper --config /etc/rabbitmq_wrapper.conf &");
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 发送测试消息
    CURL* curl = curl_easy_init();
    std::string response;
    
    if(curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080/api/send");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"message\":\"test\",\"topic\":\"system.test\"}");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl);
        ASSERT_EQ(res, CURLE_OK);
        
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        
        Json::Value root;
        Json::Reader reader;
        ASSERT_TRUE(reader.parse(response, root));
        ASSERT_EQ(root["status"].asString(), "success");
    }
    
    // 验证消息处理结果
    // ...
    
    // 停止服务
    system("pkill -f rabbitmq_wrapper");
}