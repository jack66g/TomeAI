#include <iostream>
#include <string>
#include <memory>
#include <unistd.h>

// 只引入 Router
#include "systemExecutor/SystemExecutor.h" 

using namespace std;

string globalTrim(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

int main() {
    setbuf(stdout, NULL);
    cout << "Synapse Core Started. PID: " << getpid() << endl;

    // ❌ 删掉这行：auto fileAgent = make_unique<FileCreator>();
    
    // ✅ 只保留这行：
    auto systemAgent = make_unique<SystemExecutor>();

    cout << "[System] Ready." << endl;

    string line;
    while (getline(cin, line)) {
        if (line == "exit") break;

        string cleanLine = globalTrim(line);
        if (cleanLine.empty()) continue;

        // ❌ 删掉 fileAgent 的优先处理
        
        // ✅ 唯一的入口
        if (systemAgent->processInput(cleanLine)) {
            continue;
        }

        cout << "[THINK] 无法理解该指令 (" << cleanLine << ")" << endl;
    }

    return 0;
}