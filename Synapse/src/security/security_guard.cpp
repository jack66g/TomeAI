#include "security_guard.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <cctype> // 必须加这个，不然 std::tolower 会报错

using namespace std;

// 辅助函数：转小写 (✅ 加了 static，防止和 FileCreator 里的冲突)
static string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}

SecurityGuard::SecurityGuard() {
    // 1. 白名单 (允许 AI 用的工具)
    allowedPrefixes = {
        "ls", "cd", "pwd", "mkdir", "touch", "cp", "mv", 
        "rm", "cat", "echo", "grep", "find", "nano", "vim", 
        "head", "tail", "whoami", "date", "df", "free", "ip", "ifconfig"
    };

    // 2. 黑名单 (绝对禁止的操作)
    dangerousPatterns = {
        "rm -rf /",       // 删根目录
        ":(){ :|:& };:",  // Fork炸弹
        "mkfs",           // 格式化
        "dd if=",         // 写磁盘
        "wget ",          // 下载脚本
        "curl ",          // 外连
        "> /etc/",        // 覆盖配置
        "> /boot/",       // 覆盖引导
        "chmod 777",      // 满权限
        "sudo ",          // 提权
        "/dev/sda"        // 操作物理盘
    };

    // 3. 受保护的关键路径 (禁止删这些目录本身)
    // 注意：用小写匹配
    protectedPaths = {
        "/bin", "/boot", "/dev", "/etc", "/lib", "/proc", "/root", "/sbin", "/sys", "/usr", "/var",
        "/home/ubuntu/desktop",    // 保护桌面
        "/home/ubuntu/documents",  // 保护文档
        "/home/ubuntu/downloads"   // 保护下载
    };
    
    cout << "[System] Security Guard initialized (Enhanced Mode)." << endl;
}

SecurityGuard::~SecurityGuard() {}

bool SecurityGuard::check(const string& cmd) {
    if (cmd.empty()) return false;

    string lowerCmd = toLower(cmd);

    // 1. 检查指令是否有效
    if (cmd.find("UNKNOWN_CMD") != string::npos) {
        cerr << "[Security] 拦截：AI 无法生成有效指令。" << endl;
        return false;
    }

    // 2. 格式检查 (白名单)
    if (!isFormatValid(cmd)) {
        cerr << "[Security] 拦截：指令不在白名单中 (" << cmd << ")" << endl;
        return false;
    }

    // 3. 深度危险检查 (黑名单)
    if (containsDangerousPattern(cmd)) {
        cerr << "[Security] 🔴 严重警告：检测到毁灭性指令！已拦截！" << endl;
        return false;
    }

    // 4. ✨ 专门针对删除命令的智能审查 ✨
    // 只有通过了这里的检查，才允许 SystemExecutor 去处理 (移入回收站)
    if (lowerCmd.find("rm ") == 0) {
        if (!isSafeDeletion(cmd)) {
            return false; 
        }
    }

    // 5. 针对 mv 的检查
    if (lowerCmd.find("mv ") == 0) {
        if (lowerCmd.find("/dev/null") != string::npos) {
            cerr << "[Security] 拦截：禁止将文件移动到黑洞。" << endl;
            return false;
        }
    }

    return true; // 检查通过
}

bool SecurityGuard::isFormatValid(const string& cmd) {
    for (const auto& prefix : allowedPrefixes) {
        // 精确匹配 "ls" 或 "ls " 开头
        if (cmd == prefix || cmd.find(prefix + " ") == 0) {
            return true;
        }
    }
    return false;
}

bool SecurityGuard::containsDangerousPattern(const string& cmd) {
    string lowerCmd = toLower(cmd);
    for (const auto& pattern : dangerousPatterns) {
        if (lowerCmd.find(pattern) != string::npos) {
            return true;
        }
    }
    return false;
}

// ✨✨✨ 核心逻辑：智能判断是否安全删除 ✨✨✨
bool SecurityGuard::isSafeDeletion(const string& cmd) {
    string lowerCmd = toLower(cmd);

    // 🔒 规则 A: 绝对禁止通配符 '*'
    // 防止 "rm -rf ./*" 这种删库操作
    if (lowerCmd.find("*") != string::npos) {
        cerr << "[Security] 🔴 拦截批量删除: 检测到通配符 '*'" << endl;
        cerr << "[Security] 建议: 请指定具体文件名。" << endl;
        return false;
    }

    // 🔒 规则 B: 检查受保护路径
    // 防止 AI 删掉 "桌面" 这个文件夹本身
    for (const auto& path : protectedPaths) {
        size_t pos = lowerCmd.find(path);
        
        // 如果命令里包含受保护路径
        if (pos != string::npos) {
            // 我们要看路径后面跟了什么
            size_t endOfPath = pos + path.length();
            
            // 情况 1: 命令以路径结尾 -> rm ... /desktop (拦截)
            if (endOfPath >= lowerCmd.length()) {
                cerr << "[Security] 🔴 拦截：禁止删除受保护的系统/根目录 [" << path << "]" << endl;
                return false;
            }

            char nextChar = lowerCmd[endOfPath];
            
            // 情况 2: 后面跟空格 -> rm ... /desktop -rf (拦截)
            if (nextChar == ' ') {
                cerr << "[Security] 🔴 拦截：禁止删除受保护的系统/根目录 [" << path << "]" << endl;
                return false;
            }
            
            // 情况 3: 后面跟斜杠 -> rm ... /desktop/
            if (nextChar == '/') {
                // 如果斜杠后面还有内容 -> rm ... /desktop/file.txt (这是删文件，允许！)
                // 如果斜杠后面是空的或者只有空格 -> rm ... /desktop/ (这是删目录，拦截！)
                
                string remaining = lowerCmd.substr(endOfPath + 1);
                // 去掉剩下的空格
                remaining.erase(0, remaining.find_first_not_of(" "));
                
                if (remaining.empty()) {
                     cerr << "[Security] 🔴 拦截：禁止删除受保护目录的根 [" << path << "]" << endl;
                     return false;
                }
            }
        }
    }

    return true;
}