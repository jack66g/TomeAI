#include "TrashManager.h"
#include <iostream>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdlib>

using namespace std;
namespace fs = std::filesystem;

// 构造函数：初始化回收站目录
TrashManager::TrashManager() {
    string home = getHomeDir();
    // 建立一个隐蔽的目录作为专属回收站
    trashPath = home + "/.synapse_trash";
    
    try {
        if (!fs::exists(trashPath)) {
            fs::create_directories(trashPath);
        }
    } catch (const exception& e) {
        cerr << "[TrashManager] 初始化失败: " << e.what() << endl;
    }
}

// 获取用户主目录
string TrashManager::getHomeDir() {
    const char* home = getenv("HOME");
    return home ? string(home) : "/tmp";
}

// 获取时间戳，防止同名文件冲突
string TrashManager::getTimestamp() {
    auto now = time(nullptr);
    auto tm = *localtime(&now);
    ostringstream oss;
    oss << put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

// 核心功能：移入回收站
pair<bool, string> TrashManager::moveToTrash(const string& rawPath) {
    try {
        fs::path target(rawPath);
        
        // 1. 检查文件是否存在
        if (!fs::exists(target)) {
            return {false, "目标不存在: " + rawPath};
        }

        // 2. 构造新名字：原文件名_时间戳
        // 例如: test.txt -> test.txt_20231010_123000
        string filename = target.filename().string();
        string safeName = filename + "_" + getTimestamp();
        fs::path dest = fs::path(trashPath) / safeName;

        // 3. 移动文件 (相当于 rename)
        fs::rename(target, dest);

        // 4. 记入账本 (为了能撤销)
        // 必须保存绝对路径，否则换个目录就找不到了
        historyStack.push_back({fs::absolute(target).string(), dest.string()});

        return {true, "已安全移入回收站: " + safeName};
    } catch (const exception& e) {
        return {false, string("移动失败 (权限不足?): ") + e.what()};
    }
}

// 核心功能：后悔药 (撤销)
pair<bool, string> TrashManager::undoLastDelete() {
    // 1. 检查有没有后悔药吃
    if (historyStack.empty()) {
        return {false, "没有可撤销的操作。"};
    }

    // 2. 取出最近一次记录
    DeleteRecord last = historyStack.back();
    historyStack.pop_back();

    try {
        // 3. 检查回收站里的那个文件还在不在
        if (!fs::exists(last.trashPath)) {
            return {false, "撤销失败: 回收站里的文件不见了 (可能被手动清理了)！"};
        }

        // 4. 检查原位置是不是已经有人占了
        if (fs::exists(last.originalPath)) {
            return {false, "撤销失败: 原位置已有同名文件存在！"};
        }

        // 5. 移回去
        fs::rename(last.trashPath, last.originalPath);

        return {true, "已恢复文件: " + last.originalPath};
    } catch (const exception& e) {
        // 如果失败，把记录塞回去，以免丢失
        historyStack.push_back(last);
        return {false, string("撤销异常: ") + e.what()};
    }
}