#include "GrokBrain.h" // 或者是 "GrokBrain.h"，视你的include路径而定
#include <iostream>
#include <curl/curl.h>
#include <sstream>
#include <iomanip>

using namespace std;

GrokBrain::GrokBrain() {
    // ==========================================
    // 🔧 配置区域
    // ==========================================
    this->apiKey = "灵芽密钥"; // 你的 Key
    // 确保 URL 完整且正确
    this->apiUrl = "https://api.lingyaai.cn/v1/chat/completions"; 
    this->modelName = "grok-4-1-fast-non-reasoning"; // 或 gpt-4o-mini 等
}

GrokBrain::~GrokBrain() {}

// 🛡️ [新增] JSON 转义函数：专门处理换行符和引号
std::string jsonEscape(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"':  ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break; // 👈 关键：把换行变成字面量 \n
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    ss << "\\u"
                       << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

size_t GrokBrain::WriteCallback(void* contents, size_t size, size_t nmemb, string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

string GrokBrain::think(const string& prompt) {
    // 🛡️ [调用] 在构造 JSON 前先转义
    string safePrompt = jsonEscape(prompt);

    stringstream jsonSs;
    jsonSs << "{";
    jsonSs << "  \"model\": \"" << this->modelName << "\",";
    jsonSs << "  \"messages\": [";
    jsonSs << "    {\"role\": \"user\", \"content\": \"" << safePrompt << "\"}"; 
    jsonSs << "  ],";
    jsonSs << "  \"temperature\": 0.1"; 
    jsonSs << "}";

    return sendRequest(jsonSs.str());
}

string GrokBrain::sendRequest(const string& jsonBody) {
    CURL* curl;
    CURLcode res;
    string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        
        string authHeader = "Authorization: Bearer " + this->apiKey;
        headers = curl_slist_append(headers, authHeader.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, this->apiUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); 

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            cerr << "[GrokBrain] Request failed: " << curl_easy_strerror(res) << endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
    return readBuffer; 
}