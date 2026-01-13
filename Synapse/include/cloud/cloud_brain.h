#ifndef CLOUD_BRAIN_H
#define CLOUD_BRAIN_H

#include <string>
#include <vector> // ✨ 必须引入，用于路径列表

class CloudBrain {
public:
    CloudBrain();
    ~CloudBrain();

    // 核心接口：向 DeepSeek 提问
    std::string think(const std::string& query);

    // 审计接口：加载外部 Prompt 文件进行评估
    std::string evaluateLog(const std::string& logContext);

private:
    // DeepSeek API 配置
    // 【重要】请在这里填入你的 Key，或者在 cpp 文件里写死
    const std::string apiKey = "密钥"; 
    const std::string apiUrl = "https://api.deepseek.com/chat/completions";
    const std::string modelName = "deepseek-chat"; 

    // 内部工具
    std::string jsonEscape(const std::string& input);
    std::string extractContent(const std::string& jsonResponse);
    std::string executeCurl(const std::string& cmd);

    // ✨✨✨ 新增：加载 Prompt 模板文件的函数声明 ✨✨✨
    std::string loadPromptTemplate(const std::string& filename);
};

#endif // CLOUD_BRAIN_H