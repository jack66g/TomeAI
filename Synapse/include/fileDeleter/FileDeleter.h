#ifndef FILE_DELETER_H
#define FILE_DELETER_H

#include <string>
#include <vector>
#include <memory>
#include "local/local_brain.h"
#include "cloud/cloud_brain.h"
#include "TrashManager.h"
#include "JudgmentLogger.h"
#include "security_guard.h" 

class FileDeleter {
public:
    FileDeleter();
    ~FileDeleter() = default;

    // 统一处理入口
    bool processInput(std::string input);
    
    // 简单的忙碌状态
    bool isBusy() { return false; } 

private:
    // 1. 意图识别：提取文件名列表
    bool parseDeleteIntent(const std::string& input, std::vector<std::string>& rawTargets);
    
    // 2. [新增] 路径解析核心：将模糊文件名转换为绝对路径
    // 返回值：解析后的完整路径列表。如果用户取消或找不到，返回空。
    std::vector<std::string> resolveTargetPaths(const std::vector<std::string>& rawTargets);

    // 3. [新增] 辅助函数：在系统中搜索文件
    // 返回找到的所有路径候选
    std::vector<std::string> searchFileInSystem(const std::string& filename);

    // 4. [新增] 用户交互：最终确认
    // isDirectory: 是否包含目录操作（触发额外警告）
    bool getUserConfirmation(const std::vector<std::string>& finalPaths);

    // 5. 执行逻辑
    void executeDelete(const std::vector<std::string>& finalPaths);

    std::unique_ptr<LocalBrain> aiBrain;
    std::unique_ptr<CloudBrain> cloudBrain;
    std::unique_ptr<TrashManager> trashManager;
    std::unique_ptr<JudgmentLogger> logger;
    std::unique_ptr<SecurityGuard> securityGuard;
};

#endif