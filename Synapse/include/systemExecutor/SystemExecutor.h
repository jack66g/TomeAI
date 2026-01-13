#ifndef SYSTEM_EXECUTOR_H
#define SYSTEM_EXECUTOR_H

#include <string>
#include <memory>
#include <vector>

// 确保引用路径正确，根据你的实际目录结构可能需要调整 ../
#include "local_brain.h" 
#include "cloud_brain.h"
#include "FileCreator.h"
#include "FileDeleter.h"
#include "GrokBrain.h"

class SystemExecutor {
public:
    SystemExecutor();
    ~SystemExecutor();

    // 唯一的入口
    bool processInput(const std::string& userQuery);

private:
    std::unique_ptr<LocalBrain> localBrain;
    std::unique_ptr<CloudBrain> cloudBrain;
    std::unique_ptr<GrokBrain> grokBrain; // [新增] Grok 大脑
    
    // 特种兵
    std::unique_ptr<FileCreator> fileCreator;
    std::unique_ptr<FileDeleter> fileDeleter;

    // ✨✨✨ 补上了这个声明 ✨✨✨
    std::string loadPrompt(const std::string& filename);
};

#endif