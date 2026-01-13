#ifndef SECURITY_GUARD_H
#define SECURITY_GUARD_H

#include <string>
#include <vector>

class SecurityGuard {
public:
    SecurityGuard();
    ~SecurityGuard();

    bool check(const std::string& cmd);

private:
    std::vector<std::string> allowedPrefixes;   // 白名单
    std::vector<std::string> dangerousPatterns; // 黑名单
    std::vector<std::string> protectedPaths;    // ✨ 新增：受保护路径

    bool isFormatValid(const std::string& cmd);
    bool containsDangerousPattern(const std::string& cmd);
    
    // ✨ 新增：专门检查删除命令是否安全
    bool isSafeDeletion(const std::string& cmd);
};

#endif