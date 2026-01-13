#include "FileCreator.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <vector>
#include <cstdio>

using namespace std;
namespace fs = std::filesystem;

// ==========================================
// 辅助函数工具区
// ==========================================

static string trimString(const string& str) {
    if (str.empty()) return "";
    size_t first = str.find_first_not_of(" \t\n\r");
    if (string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

static string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// ✨ 垃圾词过滤器
static bool isGarbage(const string& raw) {
    string s = toLower(trimString(raw));
    if (s == "null" || s == "none") return true;
    if (s == "path" || s == "quantity" || s == "name" || s == "names") return true;
    if (s == "file" || s == "files") return true;
    return false;
}

static string cleanMarkdown(string raw) {
    raw.erase(remove(raw.begin(), raw.end(), '`'), raw.end());
    string lowers = toLower(raw);
    if (lowers.find("plaintext") == 0) raw = raw.substr(9);
    else if (lowers.find("json") == 0) raw = raw.substr(4);
    return trimString(raw);
}

// ==========================================
// FileCreator 类实现
// ==========================================

FileCreator::FileCreator() {
    currentState = STATE_IDLE;
    targetCount = 0;
    currentExtIndex = -1;
    aiBrain = make_unique<LocalBrain>();
    cloudBrain = make_unique<CloudBrain>(); 
    logger = make_unique<JudgmentLogger>();
}

vector<string> FileCreator::splitString(const string& str, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(str);
    while (getline(tokenStream, token, delimiter)) {
        string t = trimString(token);
        if (!t.empty()) tokens.push_back(t);
    }
    return tokens;
}

// ✨ 智能名字解析
vector<string> FileCreator::parseNames(const string& rawInput) {
    string processed = rawInput;
    size_t pos = 0;
    while ((pos = processed.find("，", pos)) != string::npos) {
        processed.replace(pos, 3, ","); 
        pos += 1;
    }
    pos = 0;
    while ((pos = processed.find("、", pos)) != string::npos) {
        processed.replace(pos, 3, ","); 
        pos += 1;
    }
    return splitString(processed, ',');
}

string FileCreator::getHomeDir() {
    const char* home = getenv("HOME");
    return home ? string(home) : "/tmp";
}

void FileCreator::autoGenerateNames() {
    int startIdx = 1;
    if (!targetNames.empty()) startIdx = targetNames.size() + 1;
    
    int needed = targetCount - targetNames.size();
    if (needed <= 0) return;

    for (int i = 0; i < needed; ++i) {
        targetNames.push_back("file_" + to_string(startIdx + i));
    }
    logger->record("Action", "System Auto-Generated " + to_string(needed) + " filenames");
}

bool FileCreator::checkAllExtensionsReady() {
    for (size_t i = 0; i < targetNames.size(); ++i) {
        if (!fs::path(targetNames[i]).has_extension()) {
            currentExtIndex = i; 
            return false;
        }
    }
    return true;
}

// ✨✨✨ 核心：意图识别 ✨✨✨
bool FileCreator::askAIForIntent(const string& input) {
    string prompt = 
        "任务：参数提取\n"
        "输入：" + input + "\n"
        "格式：Names|Quantity|Path\n"
        "规则：\n"
        "1. Names: 提取文件名。识别'叫xxx'、'名为xxx'，或'搞个/弄个/建个xxx'中的xxx。没提填 NULL。\n"
        "2. Quantity: 提取数量(转阿拉伯数字)。没提填 0。\n"
        "3. Path: 提取目标位置。没提填 NULL。\n"
        "4. 严格输出一行，不要废话。\n"
        "\n"
        "Input: 创建文件\n"
        "Output: NULL|0|NULL\n"
        "Input: 在桌面创建5个文件\n"
        "Output: NULL|5|桌面\n"
        "Input: 建立一个 test.txt\n"
        "Output: test.txt|1|NULL\n"
        "Input: 在桌面搞个 backup\n" 
        "Output: backup|1|桌面\n"
        "Input: 弄三个名为 report 的文件\n"
        "Output: report|3|NULL\n"
        "\n"
        "Input: " + input + "\n"
        "Output: "; 

    logger->record("System", "Prompting Local Brain for intent extraction...");
    string result = aiBrain->talk(prompt);
    logger->record("LocalBrain", "Raw Response: " + result);

    result = cleanMarkdown(result);
    if (result.find("|") == string::npos) {
        logger->record("Error", "AI response format invalid (missing '|')");
        return false;
    }

    vector<string> parts = splitString(result, '|');
    if (parts.size() < 3) return false;

    string namesRaw = parts[0];
    string countRaw = parts[1];
    string p = parts[2];

    logger->record("Parser", "Parsed Names: " + namesRaw + ", Count: " + countRaw + ", Path: " + p);

    vector<string> rawNames;
    if (!isGarbage(namesRaw)) {
         rawNames = parseNames(namesRaw);
    }

    targetNames.clear();
    for (const auto& name : rawNames) {
        targetNames.push_back(name);
        logger->record("Security", "Accepted filename: " + name);
    }

    try { 
        if (isGarbage(countRaw)) {
            targetCount = 0;
        } else {
            targetCount = stoi(countRaw); 
        }
    } catch (...) { 
        targetCount = 0; 
    }

    if (targetCount == 0) {
        for (int i = 1; i <= 9; ++i) {
            if (input.find(to_string(i) + "个") != string::npos) { targetCount = i; break; }
        }
        if (targetCount == 0) {
            if (input.find("一个") != string::npos) targetCount = 1;
            else if (input.find("两个") != string::npos || input.find("两份") != string::npos) targetCount = 2;
            else if (input.find("三个") != string::npos) targetCount = 3;
            else if (input.find("四个") != string::npos) targetCount = 4;
            else if (input.find("五个") != string::npos) targetCount = 5;
        }
        if (targetCount > 0) {
            cout << "[THINK] AI 未提取到数量，C++ 规则引擎已强制修正为: " << targetCount << endl;
            logger->record("System", "Rule-based quantity correction: " + to_string(targetCount));
        }
    }

    if (!targetNames.empty() && targetCount < (int)targetNames.size()) {
        targetCount = targetNames.size();
    }
    if (targetNames.empty() && targetCount == 0) {
        targetCount = 1;
    }

    if (isGarbage(p)) {
        targetPathKey = "";
    } else {
        targetPathKey = p;
    }

    logger->record("State", "Final Target Count: " + to_string(targetCount));
    return true;
}

void FileCreator::performCreateFile(const string& finalPath) {
    if (targetNames.empty()) return;

    for (const auto& name : targetNames) {
        fs::path p = fs::path(finalPath) / name;
        if (fs::exists(p)) {
            logger->record("Execution", "Failed (Exists): " + p.string());
        } else {
            ofstream outfile(p);
            if (outfile.is_open()) {
                outfile << "// Created by Synapse" << endl;
                outfile.close();
                cout << "[RESULT] ✅ 创建成功: " << p.filename().string() << endl; 
                logger->record("Execution", "Success: " + p.string());
            }
        }
    }
    logger->finalizeSession(cloudBrain.get());

    currentState = STATE_IDLE;
    targetNames.clear();
    targetCount = 0;
    targetPathKey = "";
    candidatePaths.clear();
}

void FileCreator::searchPaths(const string& keyword) {
    candidatePaths.clear();
    string cleanKey = trimString(keyword);
    string home = getHomeDir();

    cout << "[THINK] 正在全盘(Home)深度搜索路径: " << cleanKey << "..." << endl;

    if (fs::exists(cleanKey) && fs::is_directory(cleanKey)) {
        candidatePaths.push_back(cleanKey);
        return;
    }

    vector<string> dirs = {"Desktop", "Downloads", "Documents", "桌面", "下载", "文档"};
    for (const auto& d : dirs) {
        if (toLower(d).find(toLower(cleanKey)) != string::npos) {
            fs::path p = fs::path(home) / d;
            if (fs::exists(p)) candidatePaths.push_back(p.string());
        }
    }

    string cmd = "find " + home + " -maxdepth 4 -type d -name '*" + cleanKey + "*' 2>/dev/null";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        char buffer[256];
        while (fgets(buffer, 256, pipe) != NULL) {
            string pathStr = trimString(buffer);
            bool exists = false;
            for(const auto& existing : candidatePaths) {
                if(existing == pathStr) { exists = true; break; }
            }
            if (!exists && !pathStr.empty()) {
                candidatePaths.push_back(pathStr);
            }
        }
        pclose(pipe);
    }
}

// ✨✨✨ 修复核心：ProcessInput 扁平化 ✨✨✨
bool FileCreator::processInput(string input) {
    string cleanInput = trimString(input);

    // 1. 变量定义必须在 goto 之前
    bool isCreateCommand = false;

    if (!cleanInput.empty()) {
        logger->record("User", cleanInput);
    }

    // === 阶段 5: 结果太多，等待确认 ===
    if (currentState == STATE_WAIT_PATH_OVERFLOW_CONFIRM) {
        if (cleanInput == "y" || cleanInput == "Y" || cleanInput == "yes" || cleanInput == "是") {
            currentState = STATE_WAIT_SELECTION;
            cout << "🤔 找到多个位置，请选择：" << endl;
            for(size_t i=0; i<candidatePaths.size(); ++i)
                cout << "[" << (i+1) << "] " << candidatePaths[i] << endl;
        } else {
            cout << "已取消列表显示。请重新输入更精确的路径关键词：" << endl;
            targetPathKey = "";
            candidatePaths.clear();
            currentState = STATE_WAIT_PATH;
        }
        return true;
    }

    // === 阶段 4: 路径多选 ===
    if (currentState == STATE_WAIT_SELECTION) {
        int choice = -1;
        try { choice = stoi(cleanInput); } catch(...) {}
        if (choice > 0 && choice <= (int)candidatePaths.size()) {
            performCreateFile(candidatePaths[choice-1]);
            return true;
        }
        cout << "[ERROR] 选项无效。" << endl;
        return true;
    }

    // === 阶段 1: 收集文件名 ===
    if (currentState == STATE_WAIT_FILENAME) {
        if (cleanInput.empty()) {
            cout << "请输入文件名 (或输入 '自动' )：" << endl;
            return true;
        }
        if (cleanInput == "自动" || toLower(cleanInput) == "auto") {
            logger->record("Action", "User triggered Auto-Generate");
            autoGenerateNames();
            goto CHECK_EXTENSION; 
        }
        vector<string> newNames = parseNames(cleanInput);
        targetNames.insert(targetNames.end(), newNames.begin(), newNames.end());
        if ((int)targetNames.size() < targetCount) {
            int remain = targetCount - targetNames.size();
            cout << "✅ 已记录，还需 " << remain << " 个文件名 (继续输入 / 批量输入 / 输入'自动'):" << endl;
            return true; 
        } else {
            goto CHECK_EXTENSION;
        }
    }

    // === 阶段 2: 处理后缀 ===
    if (currentState == STATE_WAIT_EXTENSION) {
        string ext = cleanInput;
        if (ext.empty()) ext = ".txt"; 

        if (ext.find("all ") == 0 || ext.find("所有 ") == 0) {
            string uniExt = ext.substr(ext.find(" ") + 1);
            if (uniExt.empty()) uniExt = ".txt";
            if (uniExt[0] != '.') uniExt = "." + uniExt;
            logger->record("Action", "User applied Batch Extension: " + uniExt);
            for (auto& name : targetNames) {
                if (!fs::path(name).has_extension()) name += uniExt;
            }
        } else {
            int choice = -1;
            try { choice = stoi(ext); } catch(...) {}
            if (choice > 0 && choice <= (int)commonExtensions.size()) {
                ext = commonExtensions[choice - 1];
            }
            if (ext[0] != '.') ext = "." + ext;
            if (currentExtIndex >= 0 && currentExtIndex < (int)targetNames.size()) {
                targetNames[currentExtIndex] += ext;
                cout << "✅ 文件 [" << targetNames[currentExtIndex] << "] 命名完成。" << endl;
                logger->record("Action", "Renamed file index " + to_string(currentExtIndex) + " with ext: " + ext);
            }
        }
        goto CHECK_EXTENSION;
    }

    // === 阶段 3: 路径 ===
    if (currentState == STATE_WAIT_PATH) {
        if (cleanInput.empty()) {
            targetPathKey = "桌面";
            logger->record("Action", "Default path used: Desktop");
        } else {
            targetPathKey = cleanInput;
        }
        goto HANDLE_EXECUTION;
    }

    // === 阶段 0: 初始入口 ===
    if (currentState == STATE_IDLE) {
        if (cleanInput.empty()) return false;
        
        // 2. 这里的判断使用前面定义的变量，安全
        if (cleanInput.find("创建") != string::npos) isCreateCommand = true;
        else if (cleanInput.find("建") != string::npos) isCreateCommand = true;
        else if (cleanInput.find("搞") != string::npos) isCreateCommand = true;
        else if (cleanInput.find("弄") != string::npos) isCreateCommand = true;
        else if (cleanInput.find("整") != string::npos) isCreateCommand = true;

        if (isCreateCommand) {
            logger->clear(); 
            logger->record("Session", "=== New Command Started ===");
            logger->record("User Input", cleanInput);

            if (!askAIForIntent(cleanInput)) {
                targetCount = 1;
                targetNames.clear();
                currentState = STATE_WAIT_FILENAME;
                cout << "收到创建指令。请问文件要叫什么名字？" << endl;
                return true;
            }

            if ((int)targetNames.size() < targetCount) {
                currentState = STATE_WAIT_FILENAME;
                int remain = targetCount - targetNames.size();
                cout << "准备创建 " << targetCount << " 个文件。" << endl;
                cout << "还缺 " << remain << " 个名字，请输入 (例如: a,b | 或输入 '自动'):" << endl;
                return true;
            }
            
            // 3. 这里的 goto 现在跳转到函数最外层，合法
            goto CHECK_EXTENSION;
        }
    }

    // 如果不是创建指令，返回 false 让 ShellAgent 处理
    return false;

    // ==========================================
    // 👇 公共逻辑区 (已提至函数主作用域) 👇
    // ==========================================

    CHECK_EXTENSION:
    if (!checkAllExtensionsReady()) {
        currentState = STATE_WAIT_EXTENSION;
        string problematicFile = targetNames[currentExtIndex];
        cout << "🤔 文件 [" << problematicFile << "] 缺少后缀。" << endl;
        cout << "请输入后缀 (如 .cpp)，或者输入 'all .txt' 统一应用：" << endl;
        for (size_t i = 0; i < commonExtensions.size(); ++i) {
                cout << "[" << (i + 1) << "] " << commonExtensions[i] << " ";
        }
        cout << endl;
        return true;
    }

    if (targetPathKey.empty()) {
        currentState = STATE_WAIT_PATH;
        cout << "所有文件名已就绪，请问放在哪里？(支持模糊搜索，例如 'test' 或 '/home/user/...')" << endl;
        return true;
    }

    HANDLE_EXECUTION:
    searchPaths(targetPathKey);
    
    if (candidatePaths.empty()) {
        cout << "[ERROR] 找不到类似 '" << targetPathKey << "' 的路径，请重新输入：" << endl;
        targetPathKey = ""; 
        currentState = STATE_WAIT_PATH;
        return true;
    } else if (candidatePaths.size() == 1) {
        performCreateFile(candidatePaths[0]);
    } else {
        if (candidatePaths.size() > 10) {
            currentState = STATE_WAIT_PATH_OVERFLOW_CONFIRM;
            cout << "⚠️  找到了 " << candidatePaths.size() << " 个匹配路径，是否全部显示？(y/n)" << endl;
        } else {
            currentState = STATE_WAIT_SELECTION;
            cout << "🤔 找到多个位置，请选择：" << endl;
            for(size_t i=0; i<candidatePaths.size(); ++i)
                cout << "[" << (i+1) << "] " << candidatePaths[i] << endl;
        }
    }
    return true;
}