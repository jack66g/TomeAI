#include "local_brain.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <curl/curl.h>

using namespace std;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static std::string escapeJsonString(const std::string& input) {
    std::ostringstream ss;
    for (char c : input) {
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                } else {
                    ss << c;
                }
        }
    }
    return ss.str();
}

LocalBrain::LocalBrain() {}
LocalBrain::~LocalBrain() {}

// ✨✨✨ 究极进化版解析器 ✨✨✨
// 不再死板匹配字符串，而是智能查找
std::string LocalBrain::extractResponse(const std::string& json) {
    // 1. 先检查有没有报错
    if (json.find("\"error\"") != std::string::npos) {
        return "[Ollama Error] JSON contains error field: " + json;
    }

    // 2. 查找 "response" 键
    size_t keyPos = json.find("\"response\"");
    if (keyPos == std::string::npos) return "[Error: Missing 'response' field]";

    // 3. 从 keyPos 开始往后找，找到冒号后的第一个双引号
    size_t startQuote = std::string::npos;
    bool foundColon = false;
    
    for (size_t i = keyPos + 10; i < json.length(); ++i) {
        if (!foundColon && json[i] == ':') {
            foundColon = true;
            continue;
        }
        if (foundColon && json[i] == '"') {
            startQuote = i;
            break;
        }
    }

    if (startQuote == std::string::npos) return "[Error: Cannot find value start quote]";

    // 4. 提取内容（处理转义）
    std::string result = "";
    for (size_t i = startQuote + 1; i < json.length(); ++i) {
        if (json[i] == '"' && json[i-1] != '\\') {
            break; // 结束
        }
        
        if (json[i] == '\\') {
            i++; 
            if (i >= json.length()) break;
            char next = json[i];
            if (next == 'n') result += '\n';
            else if (next == 't') result += '\t';
            else if (next == '"') result += '"';
            else if (next == '\\') result += '\\';
            else result += next;
        } else {
            result += json[i];
        }
    }
    return result;
}

std::string LocalBrain::talk(const std::string& prompt) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:11434/api/generate");
        
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        std::string safePrompt = escapeJsonString(prompt);
        // ⚠️ 确保你的模型名字正确，常用名: qwen2.5:1.5b, qwen:1.5b, qwen2:1.5b
       // 新代码 (加上 -coder)
        std::string jsonBody = "{\"model\": \"qwen2.5-coder:1.5b\", \"prompt\": \"" + safePrompt + "\", \"stream\": false}";

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L); 

        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK) {
            std::cerr << "curl error: " << curl_easy_strerror(res) << std::endl;
            return "[Error: Connection failed]";
        }
        
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    if (readBuffer.empty()) return "[Error: Empty response]";

    // ✨✨✨ 关键调试：打印 Ollama 到底回了什么 ✨✨✨
    // 如果再出错，请把这行打印出来的东西发给我，我一眼就能看出问题
    //std::cout << "[DEBUG-RAW-JSON] " << readBuffer << std::endl;
    
    return extractResponse(readBuffer);
}