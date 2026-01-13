#include "SystemExecutor.h" // 注意路径根据实际情况调整
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>

// 定义一些输出前缀，方便前端解析颜色
const std::string PREFIX_THINK = "[THINK] ";
const std::string PREFIX_RESULT = "[RESULT] ";
const std::string PREFIX_ERROR = "[ERROR] ";

using namespace std;

// 静态辅助函数：去除首尾空格
static string trim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

SystemExecutor::SystemExecutor() {
    localBrain = make_unique<LocalBrain>();
    cloudBrain = make_unique<CloudBrain>();
    grokBrain  = make_unique<GrokBrain>(); // [新增] 初始化 Grok
    
    // 初始化干活的特种兵
    fileCreator = make_unique<FileCreator>();
    fileDeleter = make_unique<FileDeleter>();
}

SystemExecutor::~SystemExecutor() {}

// 辅助函数：加载 Prompt
string SystemExecutor::loadPrompt(const string& filename) {
    // 尝试多个路径加载 prompt
    vector<string> paths = { "prompts/" + filename, "../prompts/" + filename, "../../prompts/" + filename };
    for (const auto& path : paths) {
        ifstream file(path);
        if (file.is_open()) {
            stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }
    return "";
}

bool SystemExecutor::processInput(const string& userQuery) {
    string cleanInput = trim(userQuery);
    if (cleanInput.empty()) return false;

    // ✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨
    // 🚑【核心修复】优先查岗机制
    // 如果特种兵正在忙（比如等着你输名字），直接放行，不要过 AI！
    // ✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨
    
    if (fileCreator->isBusy()) {
        return fileCreator->processInput(cleanInput);
    }

    // if (fileDeleter->isBusy()) {
    //      return fileDeleter->processInput(cleanInput);
    // }
    
    // ✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨
    // 只有大家都不忙的时候，才往下走去问 Brain
    // ✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨

    // 1. === 处理强制 Cloud 指令 (保留逻辑) ===
    bool forceCloud = false;
    if (cleanInput.find("--deepseek") != string::npos) {
        cleanInput.replace(cleanInput.find("--deepseek"), 10, ""); 
        forceCloud = true;
    } 
    else if (cleanInput.find("深度思考") != string::npos) {
        cleanInput.replace(cleanInput.find("深度思考"), 12, ""); 
        forceCloud = true;
    }
    cleanInput = trim(cleanInput);

    // 2. === 🧠 意图判断流程 ===
    
    string promptTemplate = loadPrompt("exec_router.txt");
    string intent = "OTHER";

    if (promptTemplate.empty()) {
        cout << PREFIX_ERROR << "缺少 prompts/exec_router.txt，回退到关键词匹配..." << endl;
        if (cleanInput.find("删") != string::npos) intent = "DELETE";
        else if (cleanInput.find("建") != string::npos) intent = "CREATE";
    } 
    else {
        // --- 第一轮：Local Brain (Qwen) ---
        string prompt = promptTemplate;
        size_t pos = prompt.find("{{USER_INPUT}}");
        if (pos != string::npos) prompt.replace(pos, 14, cleanInput);

        cout << PREFIX_THINK << "Local Brain 正在思考意图..." << endl;
        string intentRaw = localBrain->talk(prompt);
        intent = trim(intentRaw);
        cout << PREFIX_THINK << "Local Brain 判定: " << intent << endl;

        // --- 第二轮：Grok (灵芽) 兜底机制 ---
        // 触发条件：Local 判不出 (OTHER) 且 用户没开强制 DeepSeek 模式
        if (intent.find("OTHER") != string::npos && !forceCloud) {
            cout << PREFIX_THINK << "⚠️ Local Brain 不确定，呼叫 Grok 进行云端仲裁..." << endl;
            
            // 构造极简 Prompt，强制 Grok 做选择题
            string grokPrompt = "你是一个意图分类器。用户输入：\"" + cleanInput + "\"。\n"
                                "请判断其意图，必须从以下三个词中选一个返回：[CREATE, DELETE, OTHER]。\n"
                                "CREATE代表创建文件/文件夹，DELETE代表删除/移除，OTHER代表其他。\n"
                                "不要解释，只输出单词。";
                                
            string grokResult = grokBrain->think(grokPrompt);
            string grokIntent = trim(grokResult);
            
            cout << PREFIX_THINK << "Grok 仲裁结果: " << grokIntent << endl;

            // 修正 intent
            if (grokIntent.find("CREATE") != string::npos) intent = "CREATE";
            else if (grokIntent.find("DELETE") != string::npos) intent = "DELETE";
            // 如果 Grok 也说是 OTHER，那就真的是 OTHER 了
        }
    }

    // 3. === 任务分发 ===
    
    if (intent.find("CREATE") != string::npos) {
        cout << PREFIX_THINK << "✅ 最终识别为【创建】意图，执行 FileCreator..." << endl;
        return fileCreator->processInput(cleanInput);
    }
    else if (intent.find("DELETE") != string::npos) {
        cout << PREFIX_THINK << "✅ 最终识别为【删除】意图，执行 FileDeleter..." << endl;
        return fileDeleter->processInput(cleanInput);
    }
    
    // 4. === 兜底逻辑：OTHER ===
    
    if (forceCloud) {
        cout << PREFIX_THINK << "🚀 意图为 OTHER，但收到强制指令，直连 Cloud..." << endl;
        string prompt = "你是一个 Linux 专家。用户需求：" + cleanInput + "\n规则：只输出 Linux 命令，不要代码块，不解释。";
        string rawCommand = cloudBrain->think(prompt);
        
        if (!rawCommand.empty()) {
            cout << PREFIX_RESULT << "AI 生成的建议命令 (未执行): " << rawCommand << endl;
        }
        return true;
    }
    else {
        cout << PREFIX_THINK << "❌ 双大脑均未识别为操作指令，且未开启深度思考，待机中。" << endl;
        return false;
    }
}