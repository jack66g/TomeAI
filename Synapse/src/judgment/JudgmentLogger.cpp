#include "JudgmentLogger.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <ctime>
#include <iomanip>

using namespace std;
namespace fs = std::filesystem;

JudgmentLogger::JudgmentLogger() {}

string JudgmentLogger::currentTimestamp() {
    auto now = time(nullptr);
    auto tm = *localtime(&now);
    ostringstream oss;
    oss << put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

void JudgmentLogger::record(const string& actor, const string& action) {
    // 1. 打印到屏幕 (保持原有交互体验)
    // 注意：这里我们只负责存日志，屏幕打印通常由 FileCreator 自己做
    // 但为了日志完整，我们将 Actor 和 Action 存入流
    sessionLog << "[" << actor << "] " << action << endl;
}

void JudgmentLogger::finalizeSession(CloudBrain* cloudBrain) {
    string finalLog = sessionLog.str();
    if (finalLog.empty()) return;

    cout << "\n[System] 正在请求 DeepSeek 审计本轮操作..." << endl;
    
    // 1. 调用云端大脑进行判别
    string judgment = cloudBrain->evaluateLog(finalLog);
    
    // 2. 构造完整存档内容
    stringstream fileContent;
    fileContent << "========= INTERACTION LOG =========" << endl;
    fileContent << finalLog << endl;
    fileContent << "========= DEEPSEEK JUDGMENT =========" << endl;
    fileContent << judgment << endl;

    // 3. 写入文件
    string logDir = "training_data";
    if (!fs::exists(logDir)) fs::create_directory(logDir);
    
    string filename = logDir + "/log_" + currentTimestamp() + ".txt";
    ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << fileContent.str();
        outfile.close();
        cout << "[System] 审计完成。训练数据已保存至: " << filename << endl;
        cout << ">>> 裁判意见: " << judgment << endl; // 把 AI 的评价打出来看看
    } else {
        cerr << "[Error] 无法保存日志文件。" << endl;
    }
    
    clear(); // 清理内存，防止污染下一轮
}

void JudgmentLogger::clear() {
    sessionLog.str("");
    sessionLog.clear();
}