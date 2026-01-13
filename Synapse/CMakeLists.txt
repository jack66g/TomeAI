cmake_minimum_required(VERSION 3.10)

# 项目名称
project(Synapse)

# ==========================================
# 1. 编译设置
# ==========================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra)

# ==========================================
# 2. 查找必要的库 (关键修复！)
# ==========================================
# 查找系统线程库
find_package(Threads REQUIRED)

# ⚠️ 必须查找 CURL 库，否则后面的链接会报错
find_package(CURL REQUIRED)

# ==========================================
# 3. 头文件路径 (关键修复！)
# ==========================================
# 告诉编译器去这些文件夹里找 .h 文件
include_directories(
    include             # 找全局头文件
    include/fileAgent       # 找 FileCreator.h
    include/systemExecutor  # 找 SystemExecutor.h
    include/local           # 找 local_brain.h
    include/cloud           # 找 cloud_brain.h
    include/security        # 找 security_guard.h
    include/utils          # 找 utils.h
    include/judgment       # 找 judgment_module.h
    include/fileDeleter     # 找 FileDeleter.h
    include/grokBrain       # 找 GrokBrain.h
    ${CURL_INCLUDE_DIRS} # 找 curl/curl.h
)

# ==========================================
# 4. 源文件扫描
# ==========================================
# 自动扫描 src 文件夹下所有的 .cpp 文件
file(GLOB_RECURSE SOURCES "src/*.cpp")

message(STATUS "Found source files: ${SOURCES}")

# ==========================================
# 5. 生成目标与链接
# ==========================================
add_executable(synapse ${SOURCES})

# 链接所有需要的库
target_link_libraries(synapse 
    Threads::Threads    # 线程库
    stdc++fs            # 文件系统库 (C++17 filesystem)
    ${CURL_LIBRARIES}   # CURL 网络库
)