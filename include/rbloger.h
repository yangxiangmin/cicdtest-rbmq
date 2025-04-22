#ifndef RBMSG_LOGGER_H
#define RBMSG_LOGGER_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <thread>
#include <fstream>
#include <iomanip>		//std::put_time
#include <ctime>		//std::localtime 和 std::time_t
#include <sys/stat.h>	//stat 结构体和 S_IFDIR 常量
#include <unistd.h>		//stat,mkdir等

class RbmqLogger {
public:
    RbmqLogger();  // 构造函数

    // 设置最大重复日志记录次数和时间窗口（秒）
    void setThreshold(int threshold, int time_window);

    // 设置日志文件路径
    void setLogFile(const std::string& filepath);

    // 设置日志文件最大大小，单位字节
    void setMaxLogSize(std::size_t max_size);

    // 记录错误日志
    void Log(const std::string& message);

private:
    // 写入日志的方法
    void writeLog(const std::string& message);

	// 清理过期的日志记录
	void cleanupExpiredLogs();

    // 错误记录结构
    struct LogInfo {
        std::chrono::steady_clock::time_point first_time; // 第一次出现时间
        int count; // 错误发生次数
    };

    std::unordered_map<size_t, LogInfo> LogMap; // 错误信息的哈希值与记录结构的映射
    int log_threshold; // 最大重复次数
    int log_time_window; // 时间窗口（秒）
    std::mutex mutex_; // 线程安全保护
    std::string log_filepath; // 日志文件路径
    std::size_t max_log_size; // 日志文件最大大小
};

#endif // RBMSG_LOGGER_H

