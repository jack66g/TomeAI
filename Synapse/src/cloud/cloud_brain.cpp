#include "cloud_brain.h"
#include <iostream>
#include <fstream>
#include <sstream> // ✨ 必须引入，用于读取文件流
#include <memory>
#include <array>
#include <cstdio>
#include <filesystem> // 用于路径检查
#include <vector>

namespace fs = std::filesystem;

CloudBrain::CloudBrain() {
    // std::cout << "[System] Cloud Brain (DeepSeek) Initialized." << std::endl;
}

CloudBrain::~CloudBrain() {}

// ✨✨✨ 新增：从文件加载 Prompt ✨✨✨
std::string CloudBrain::loadPromptTemplate(const std::string& filename) {
    // 尝试多个路径，兼容 build 目录运行和根目录运行的情况
    std::vector<std::string> paths = {
        "../prompts/" + filename,      //如果在 build/synapse 运行
        "prompts/" + filename,         //如果在根目录运行
        "../../prompts/" + filename    //防御性编程
    };

    for (const auto& path : paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }

    std::cerr << "[Error] Could not load prompt file: " << filename << std::endl;
    return ""; // 返回空字符串表示失败
}

std::string CloudBrain::think(const std::string& query) {
    if (apiKey.empty() || apiKey.find("sk-") == std::string::npos) {
        return "[Config Error] Please set your DeepSeek API Key in src/cloud/cloud_brain.h or .cpp";
    }

    std::cout << ">>> [DeepSeek] Thinking..." << std::endl;

    std::string safeQuery = jsonEscape(query);
    
    // 1. 构造 JSON 内容
    std::string jsonBody = "{"
        "\"model\": \"" + modelName + "\","
        "\"messages\": [{\"role\": \"user\", \"content\": \"" + safeQuery + "\"}],"
        "\"stream\": false"
    "}";

    // 2. 将 JSON 写入临时文件 (解决 Shell 特殊字符问题)
    std::string tempFileName = "deepseek_request.json";
    std::ofstream file(tempFileName);
    if (file.is_open()) {
        file << jsonBody;
        file.close();
    } else {
        return "[Error] Failed to create temporary request file.";
    }

    // 3. 构造 curl 命令
    // 使用 @符号读取文件，避免命令行长度限制
    std::string cmd = "curl -s -X POST " + apiUrl + 
                      " -H \"Content-Type: application/json\"" +
                      " -H \"Authorization: Bearer " + apiKey + "\"" +
                      " -d @" + tempFileName; 

    std::string rawJson = executeCurl(cmd);
    
    // 清理临时文件 (调试时可注释掉)
    // remove(tempFileName.c_str());

    if (rawJson.empty()) return "[Error] Network failure connecting to DeepSeek.";

    return extractContent(rawJson);
}

std::string CloudBrain::executeCurl(const std::string& cmd) {
    std::array<char, 4096> buffer; 
    std::string result;
    // 使用 unique_ptr 管理 pipe 指针，防止内存泄漏
    using PcloseType = int(*)(FILE*);
    std::unique_ptr<FILE, PcloseType> pipe(popen(cmd.c_str(), "r"), pclose);
    
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::string CloudBrain::jsonEscape(const std::string& input) {
    std::string output;
    for (char c : input) {
        if (c == '"') output += "\\\"";
        else if (c == '\\') output += "\\\\";
        else if (c == '\n') output += "\\n"; 
        else output += c;
    }
    return output;
}

std::string CloudBrain::extractContent(const std::string& json) {
    std::string key = "\"content\":\"";
    size_t start = json.find(key);
    if (start == std::string::npos) return "Error: Could not parse DeepSeek response. (" + json + ")";
    
    start += key.length();
    std::string result;
    bool escape = false;
    
    for (size_t i = start; i < json.length(); ++i) {
        char c = json[i];
        if (escape) {
            if (c == 'n') result += '\n'; 
            else result += c;
            escape = false;
        } else {
            if (c == '\\') escape = true;
            else if (c == '"') break;
            else result += c;
        }
    }
    return result;
}

// ✨✨✨ 模块化审计版：加载外部 Prompt 文件 ✨✨✨
std::string CloudBrain::evaluateLog(const std::string& logContext) {
    // 1. 加载系统通用原则
    std::string systemPrompt = loadPromptTemplate("audit_system.txt");
    
    // 2. 根据日志内容，智能选择加载哪一个任务的规则
    std::string taskPrompt;
    
    // 简单的关键词匹配来判断任务类型
    // 未来如果 FileDeleter 写入了 "TaskType: DELETE"，这里就会自动切换
    if (logContext.find("DELETE") != std::string::npos || 
        logContext.find("TaskType: DELETE_OPERATION") != std::string::npos) {
        taskPrompt = loadPromptTemplate("audit_task_delete.txt");
    } else {
        // 默认认为是创建任务
        taskPrompt = loadPromptTemplate("audit_task_create.txt");
    }

    if (systemPrompt.empty() || taskPrompt.empty()) {
        return "[System Error] 缺少 Prompt 模板文件，请检查 prompts/ 文件夹是否存在 audit_system.txt 和 audit_task_create.txt";
    }

    // 3. 组合 Prompt
    std::string fullPrompt = systemPrompt + "\n\n" + taskPrompt;

    // 4. 替换占位符 {{LOG_CONTEXT}}
    std::string placeholder = "{{LOG_CONTEXT}}";
    size_t pos = fullPrompt.find(placeholder);
    if (pos != std::string::npos) {
        fullPrompt.replace(pos, placeholder.length(), logContext);
    } else {
        // 如果模板里忘写占位符了，就追加在最后作为保底
        fullPrompt += "\n\n=== LOG START ===\n" + logContext + "\n=== LOG END ===";
    }

    // 5. 发送给 DeepSeek
    return think(fullPrompt); 
}