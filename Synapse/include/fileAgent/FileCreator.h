#ifndef FILE_CREATOR_H
#define FILE_CREATOR_H

#include <string>
#include <vector>
#include <memory>
#include "local/local_brain.h"
#include "cloud/cloud_brain.h"
#include "judgment/JudgmentLogger.h"

enum CreatorState {
    STATE_IDLE,
    STATE_WAIT_FILENAME,    
    STATE_WAIT_EXTENSION,   // 改为：逐个或统一处理后缀
    STATE_WAIT_PATH,        
    STATE_WAIT_SELECTION,
    STATE_WAIT_PATH_OVERFLOW_CONFIRM // ✨ 新增：搜索结果太多，等待用户确认是否显示    
};



class FileCreator {
private:
    std::unique_ptr<LocalBrain> aiBrain;

    // ✨✨✨ 这里必须声明，不然 cpp 里就会报“未定义标识符” ✨✨✨
    std::unique_ptr<CloudBrain> cloudBrain;
    std::unique_ptr<JudgmentLogger> logger;
    
    CreatorState currentState;
    
    std::vector<std::string> targetNames; 
    int targetCount;                      
    std::string targetPathKey;            
    
    // 记录当前正在处理后缀的文件下标
    int currentExtIndex; 

    std::vector<std::string> candidatePaths; 
    const std::vector<std::string> commonExtensions = {
        ".txt", ".cpp", ".h", ".py", ".sh", ".md", ".json", ".cmake"
    };

    std::string getHomeDir();
    void searchPaths(const std::string& keyword);
    bool askAIForIntent(const std::string& input);
    void performCreateFile(const std::string& finalPath);
    
    std::vector<std::string> splitString(const std::string& str, char delimiter);
    // ✨ 新增：专门处理文件名的分割（自动兼容中英文逗号）
    std::vector<std::string> parseNames(const std::string& rawInput);
    void autoGenerateNames();

    // 检查是否所有文件都有后缀了
    bool checkAllExtensionsReady();

public:
    FileCreator();
    bool processInput(std::string input);
    // ✨✨✨【关键修复】告诉 Router 我是不是正在忙 ✨✨✨
    // 如果返回 true，Router 就会直接把输入传给我，而不去问 AI
    bool isBusy() {
        return currentState != STATE_IDLE;
    }
};

#endif