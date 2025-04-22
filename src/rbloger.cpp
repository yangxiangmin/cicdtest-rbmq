#include "rbloger.h"

//1. 引入时间限制： 通过记录最后一次日志的时间，如果相同的日志信息在短时间内重复出现，则只写入一次日志。
//2. 限制重复日志的次数： 记录每个日志信息的次数，并在一定次数之后停止写入重复的日志。
//3. 使用哈希去重： 通过哈希值标识日志信息，在短时间内遇到相同日志不重复写入。

RbmqLogger::RbmqLogger() : log_threshold(5), log_time_window(10), log_filepath("rbmsg.log"), max_log_size(4 * 1024 * 1024) {}  // 默认重复日志阈值为5次，时间窗口为10秒

// 设置最大重复日志记录次数和时间窗口（秒）
void RbmqLogger::setThreshold(int threshold, int time_window) {
    log_threshold = threshold;
    log_time_window = time_window;
}

// 设置日志文件路径
void RbmqLogger::setLogFile(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    log_filepath = filepath;
}

// 设置日志文件最大大小，单位字节
void RbmqLogger::setMaxLogSize(std::size_t max_size) {
    max_log_size = max_size;
}

// 清理过期的日志记录
void RbmqLogger::cleanupExpiredLogs() {
    auto current_time = std::chrono::steady_clock::now();
    for (auto it = LogMap.begin(); it != LogMap.end();) {
        if (std::chrono::duration_cast<std::chrono::seconds>(current_time - it->second.first_time).count() > log_time_window) {
            it = LogMap.erase(it); // 删除过期的日志记录
        } else {
            ++it;
        }
    }
}

// 记录日志
void RbmqLogger::Log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto current_time = std::chrono::steady_clock::now();
    auto log_hash = std::hash<std::string>{}(message);

    // 清理过期的日志记录
    cleanupExpiredLogs();

    // 如果这个错误已经记录过，检查时间限制和次数限制
    if (LogMap.find(log_hash) != LogMap.end()) {
        auto& log_info = LogMap[log_hash];

        // 如果距离上次记录的时间不足以满足时间窗口，增加计数器
        if (std::chrono::duration_cast<std::chrono::seconds>(current_time - log_info.first_time).count() <= log_time_window) {
            log_info.count++;
            if (log_info.count >= log_threshold) {
                // 如果超出阈值，不再写入日志
                return;
            }
        } else {
            // 如果超出时间窗口，重置计数器
            log_info.count = 1;
            log_info.first_time = current_time;
        }
    } else {
        // 新的错误，添加到日志记录中
        LogMap[log_hash] = { current_time, 1 };
    }

    // 写入日志
    writeLog(message);
}

// 写入日志到文件
void RbmqLogger::writeLog(const std::string& message) {
    if (log_filepath.empty()) {
        std::cerr << "Log file path is not set!" << std::endl;
        return;
    }

    // 检查文件大小
    std::ifstream log_file_check(log_filepath, std::ios::binary | std::ios::ate);
    if (log_file_check.is_open()) {
        std::size_t file_size = log_file_check.tellg();
        log_file_check.close();

        // 如果文件大小超出限制，重命名日志文件并创建新的日志文件
        if (file_size >= max_log_size) {
            // 获取当前时间
            auto now = std::chrono::system_clock::now();
            auto now_time = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&now_time);

            // 格式化时间为年月日时分秒
            char time_str[20];
            std::strftime(time_str, sizeof(time_str), "%Y%m%d%H%M%S", &tm);

            // 创建备份文件名
            std::string backup_filename = "logbak/log_" + std::string(time_str) + ".bak";

            // 检查 logbak 目录是否存在，如果不存在则创建
            struct stat info;
            if (stat("logbak", &info) != 0) {
                // 目录不存在，创建目录
                if (mkdir("logbak", 0777) != 0) {
                    std::cerr << "Failed to create logbak directory!" << std::endl;
                    return;
                }
            } else if (!(info.st_mode & S_IFDIR)) {
                // 路径存在但不是目录
                std::cerr << "logbak is not a directory!" << std::endl;
                return;
            }

            // 重命名文件
            if (std::rename(log_filepath.c_str(), backup_filename.c_str()) != 0) {
                std::cerr << "Failed to rename log file to backup!" << std::endl;
                return;
            }
        }
    }

    // 打开日志文件并写入日志
    std::ofstream log_file;
    log_file.open(log_filepath, std::ios::app); // 以追加模式打开文件
    if (!log_file.is_open()) {
        std::cerr << "Failed to open log file: " << log_filepath << std::endl;
        return;
    }

    // 写入日志信息
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    log_file << "[" << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S") << "] " << message << std::endl;

    log_file.close();
}

