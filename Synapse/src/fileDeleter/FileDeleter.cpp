#include "FileDeleter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <filesystem> // C++17 标准库
#include <cstdio>
#include <regex>      // 正则库

namespace fs = std::filesystem;
using namespace std;

FileDeleter::FileDeleter() {
    aiBrain = make_unique<LocalBrain>();
    cloudBrain = make_unique<CloudBrain>();
    trashManager = make_unique<TrashManager>();
    logger = make_unique<JudgmentLogger>();
    securityGuard = make_unique<SecurityGuard>();
}

// ==========================================
// [辅助] 提取输入中的路径上下文
// 用于：用户只说了路径没说文件名的情况
// ==========================================
string extractPathContext(const string& input) {
    try {
        // 匹配绝对路径 /xxx/xxx
        regex path_pattern(R"(/[^ \t\n\r"']+)");
        smatch match;
        if (regex_search(input, match, path_pattern)) {
            return match.str();
        }
        
        // 简单的中文路径别名匹配 (可以扩展)
        if (input.find("桌面") != string::npos) return "桌面";
        if (input.find("文档") != string::npos) return "文档";
        if (input.find("下载") != string::npos) return "下载";
    } catch (...) {}
    return "";
}

// ==========================================
// 1. 意图解析 (AI -> 增强正则 -> 暴力去词)
// ==========================================
bool FileDeleter::parseDeleteIntent(const string& input, vector<string>& rawTargets) {
    // 🚀 1. 尝试用 AI 提取
    string prompt = 
        "Task: Extract target files.\n"
        "Input: \"" + input + "\"\n"
        "Rules: Output filenames or paths only. Separated by '|'. No placeholders.\n"
        "Samples:\n"
        "In: 删除1.txt\nOut: 1.txt\n"  
        "In: 删了 /tmp/a.log\nOut: /tmp/a.log\n"
        "In: 把a.txt删掉\nOut: a.txt\n"
        "Out: "; 

    string result = aiBrain->talk(prompt);
    
    // 清洗结果
    result.erase(0, result.find_first_not_of(" \t\n\r"));
    result.erase(result.find_last_not_of(" \t\n\r") + 1);
    logger->record("LocalBrain", "Raw Intent: " + result);

    if (result.find("NULL") == string::npos && result.length() > 1 && result.find("File1") == string::npos) {
        stringstream ss(result);
        string segment;
        while(getline(ss, segment, '|')) {
            segment.erase(0, segment.find_first_not_of(" \t\n\r"));
            segment.erase(segment.find_last_not_of(" \t\n\r") + 1);
            if(!segment.empty() && segment != "...") {
                rawTargets.push_back(segment);
            }
        }
    }

    if (!rawTargets.empty()) return true;

    // 🚀 2. 增强正则 (Regex Fallback)
    // 只有当 AI 失败时才启用
    try {
        // 模式1: 绝对路径
        regex abs_path_pattern(R"(/[^ \t\n\r"']+)");
        // 模式2: 明确的文件名 (xxx.xx)
        regex filename_pattern(R"([a-zA-Z0-9_\u4e00-\u9fa5]+\.[a-zA-Z0-9]+)"); 

        auto begin1 = sregex_iterator(input.begin(), input.end(), abs_path_pattern);
        auto end1 = sregex_iterator();
        for (auto i = begin1; i != end1; ++i) rawTargets.push_back(i->str());

        auto begin2 = sregex_iterator(input.begin(), input.end(), filename_pattern);
        auto end2 = sregex_iterator();
        for (auto i = begin2; i != end2; ++i) rawTargets.push_back(i->str());
    } catch (...) {}

    if (!rawTargets.empty()) return true;

    // 🚀 3. 暴力去词法
    // 如果还没找到，可能用户根本没输文件名，或者文件名很不规范（无后缀）
    // 这里我们先不暴力提取，因为可能是“意图明确但参数缺失”，留给 processInput 处理交互
    
    return false; 
}

// ... (searchFileInSystem 保持不变) ...
vector<string> FileDeleter::searchFileInSystem(const string& filename) {
    vector<string> candidates;
    // 智能处理：如果是相对路径，在当前目录或常用目录搜；如果是"桌面"，映射路径
    string searchPath = "/home/ubuntu";
    if (filename.find("桌面") == 0) searchPath = "/home/ubuntu/Desktop"; // 简单映射

    string cmd = "find " + searchPath + " -maxdepth 4 -name \"" + filename + "\" 2>/dev/null";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return candidates;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        string path = buffer;
        path.erase(path.find_last_not_of(" \n\r") + 1);
        if (!path.empty()) candidates.push_back(path);
    }
    pclose(pipe);
    return candidates;
}

// ... (resolveTargetPaths 保持不变) ...
vector<string> FileDeleter::resolveTargetPaths(const vector<string>& rawTargets) {
    vector<string> resolvedPaths;
    for (const auto& target : rawTargets) {
        if (target.find("/") == 0) {
            if (fs::exists(target)) {
                resolvedPaths.push_back(target);
            } else {
                cout << "⚠️  找不到指定路径: " << target << " (已忽略)" << endl;
                logger->record("Resolution", "Path not found: " + target);
            }
            continue;
        }
        cout << "[System] 正在定位文件 [" << target << "] ..." << endl;
        vector<string> candidates = searchFileInSystem(target);

        if (candidates.empty()) {
            cout << "❌ 未找到名为 [" << target << "] 的文件。" << endl;
            logger->record("Resolution", "File not found: " + target);
        } else if (candidates.size() == 1) {
            cout << "✅ 已定位: " << candidates[0] << endl;
            resolvedPaths.push_back(candidates[0]);
        } else {
            cout << "🤔 找到多个 [" << target << "]，请选择要删除哪一个：" << endl;
            for (size_t i = 0; i < candidates.size(); ++i) {
                cout << " [" << (i + 1) << "] " << candidates[i] << endl;
            }
            cout << " [0] 跳过此文件" << endl;
            cout << "请输入序号: ";
            int choice;
            if (cin >> choice) {
                if (choice > 0 && static_cast<size_t>(choice) <= candidates.size()) {
                    resolvedPaths.push_back(candidates[choice - 1]);
                    logger->record("Resolution", "User selected: " + candidates[choice - 1]);
                } else { cout << "已跳过。" << endl; }
            }
            cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    }
    return resolvedPaths;
}

// ... (getUserConfirmation 保持不变) ...
bool FileDeleter::getUserConfirmation(const vector<string>& finalPaths) {
    if (finalPaths.empty()) return false;
    bool hasDirectory = false;
    for (const auto& path : finalPaths) {
        if (fs::is_directory(path)) { hasDirectory = true; break; }
    }
    cout << "\n================ 🗑️ 删除确认 ================" << endl;
    if (hasDirectory) {
        cout << "🔴 警告：检测到列表中包含【文件夹】！" << endl;
        cout << "🔴 删除文件夹将移除其内部所有文件！" << endl;
    }
    cout << "即将把以下 " << finalPaths.size() << " 项移入回收站：" << endl;
    for (const auto& path : finalPaths) {
        if (fs::is_directory(path)) cout << " 📁 " << path << endl;
        else cout << " 📄 " << path << endl;
    }
    cout << "============================================" << endl;
    cout << "❓ 确认执行吗？(y/n): ";
    string input;
    getline(cin, input);
    if (input == "y" || input == "Y") {
        logger->record("Interaction", "User CONFIRMED deletion.");
        return true;
    } else {
        logger->record("Interaction", "User CANCELLED deletion.");
        cout << "操作已取消。" << endl;
        return false;
    }
}

// ... (executeDelete 保持不变) ...
void FileDeleter::executeDelete(const vector<string>& finalPaths) {
    int successCount = 0;
    int failCount = 0;
    for (const auto& path : finalPaths) {
        string virtualCmd = "rm " + path; 
        if (fs::is_directory(path)) virtualCmd += " -rf"; 
        if (!securityGuard->check(virtualCmd)) {
            cout << "🛡️ [拦截] SecurityGuard 拒绝删除: " << path << endl;
            logger->record("Security", "⛔ BLOCKED: " + path);
            failCount++;
            continue;
        }
        cout << "[Action] 正在移入回收站: " << path << " ..." << endl;
        pair<bool, string> result = trashManager->moveToTrash(path);
        if (result.first) {
            cout << "✅ " << result.second << endl;
            logger->record("Execution", "Success: " + path);
            successCount++;
        } else {
            cout << "❌ " << result.second << endl;
            logger->record("Execution", "Failed: " + result.second);
            failCount++;
        }
    }
    string summary = "Result: " + to_string(successCount) + " Success, " + to_string(failCount) + " Blocked/Failed.";
    logger->record("Summary", summary);
}

// ==========================================
// 主流程 (新增：多轮追问逻辑)
// ==========================================
bool FileDeleter::processInput(string input) {
    logger->clear();
    logger->record("TaskType", "DELETE_OPERATION");
    logger->record("User Input", input);

    vector<string> rawTargets;
    
    // 1. 尝试提取意图
    bool hasTargets = parseDeleteIntent(input, rawTargets);

    // ✨✨✨ 核心逻辑：如果没提取到文件名，启动追问模式 ✨✨✨
    if (!hasTargets) {
        cout << "🤔 明白您想删除文件，但没听清具体是哪个。" << endl;
        
        // 尝试从原句中提取路径上下文 (例如 "在 /tmp 下删除...")
        string contextPath = extractPathContext(input);
        
        if (!contextPath.empty()) {
            cout << "📂 您是指在目录 [" << contextPath << "] 下删除吗？" << endl;
            cout << "👉 请输入该目录下的文件名 (如: data.log): ";
        } else {
            cout << "👉 请输入完整路径或文件名: ";
        }

        string supplement;
        getline(cin, supplement); // 获取用户补充输入

        if (!supplement.empty()) {
            logger->record("Interaction", "User supplemented: " + supplement);
            
            // 智能组合：如果之前有上下文路径，且用户输入不是绝对路径，则拼接
            if (!contextPath.empty() && supplement.find("/") != 0) {
                // 处理 "桌面" 等特殊别名
                if (contextPath == "桌面") contextPath = "/home/ubuntu/Desktop";
                // 拼接路径
                string fullPath = contextPath;
                if (fullPath.back() != '/') fullPath += "/";
                fullPath += supplement;
                
                cout << "[System] 自动组合路径: " << fullPath << endl;
                rawTargets.push_back(fullPath);
            } else {
                // 用户输入了全新内容，直接作为目标
                rawTargets.push_back(supplement);
            }
        } else {
            cout << "操作已取消。" << endl;
        }
    }

    // Step 2: 路径补全 (对 targets 进行最终检索)
    if (!rawTargets.empty()) {
        vector<string> finalPaths = resolveTargetPaths(rawTargets);
        if (!finalPaths.empty()) {
            // Step 3: 用户确认
            if (getUserConfirmation(finalPaths)) {
                // Step 4: 执行
                executeDelete(finalPaths);
            }
        } else {
            logger->record("Resolution", "No valid paths resolved.");
        }
    } else {
        logger->record("LocalBrain", "No targets provided after inquiry.");
    }

    // 上传日志
    logger->finalizeSession(cloudBrain.get());
    return true; 
}
