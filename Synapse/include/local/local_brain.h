#ifndef LOCAL_BRAIN_H
#define LOCAL_BRAIN_H

#include <string>

class LocalBrain {
public:
    LocalBrain();
    ~LocalBrain();

    // 核心接口：与本地模型对话
    // 参数 prompt: 用户的输入或系统指令
    // 返回: 模型的回复文本
    std::string talk(const std::string& prompt);

private:
    // 配置部分 (方便后续修改)
    const std::string modelName = "qwen2.5:1.5b"; // 请确保 `ollama list` 里有这个名字
    const std::string apiUrl = "http://localhost:11434/api/generate";

    // 内部工具函数：JSON 清洗与解析
    std::string jsonEscape(const std::string& input);
    std::string extractResponse(const std::string& jsonResponse);
    std::string executeCurl(const std::string& cmd);
};

#endif // LOCAL_BRAIN_H