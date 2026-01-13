#ifndef JUDGMENT_LOGGER_H
#define JUDGMENT_LOGGER_H

#include <string>
#include <vector>
#include <sstream>
#include "../cloud/cloud_brain.h"

class JudgmentLogger {
private:
    std::stringstream sessionLog; // 内存中的日志流
    std::string currentTimestamp();

public:
    JudgmentLogger();
    
    // 核心记录接口
    void record(const std::string& actor, const std::string& action);
    
    // 结束当前会话：保存文件并请求 AI 判别
    void finalizeSession(CloudBrain* cloudBrain);
    
    // 清空日志，准备下一次指令
    void clear();
};

#endif