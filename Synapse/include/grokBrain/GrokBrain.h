#ifndef GROK_BRAIN_H
#define GROK_BRAIN_H

#include <string>
#include <vector>

class GrokBrain {
public:
    GrokBrain();
    ~GrokBrain();

    // 核心接口：发送 prompt，返回 Grok 的回答
    // 如果是兜底意图识别，建议 prompt 里限制它只输出 json 或特定关键词
    std::string think(const std::string& prompt);

private:
    // 灵芽平台的 API 配置
    std::string apiKey;
    std::string apiUrl;
    std::string modelName; // 例如 "grok-beta" 或灵芽支持的其他模型名

    // 内部使用的 HTTP 发送函数
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    std::string sendRequest(const std::string& jsonBody);
};

#endif // GROK_BRAIN_H