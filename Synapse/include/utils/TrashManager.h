#pragma once
#include <string>
#include <vector>
#include <utility> // for std::pair

class TrashManager {
public:
    TrashManager();
    
    // 移入回收站
    // 返回值: <是否成功, 提示信息>
    std::pair<bool, std::string> moveToTrash(const std::string& originalPath);

    // 撤销上一次删除
    // 返回值: <是否成功, 提示信息>
    std::pair<bool, std::string> undoLastDelete();

private:
    std::string trashPath;
    
    // 记录结构体：保存"原路径"和"回收站里的现路径"
    struct DeleteRecord { 
        std::string originalPath; 
        std::string trashPath; 
    };
    
    // 历史记录栈（后进先出，实现 Undo）
    std::vector<DeleteRecord> historyStack;

    std::string getTimestamp();
    std::string getHomeDir();
};