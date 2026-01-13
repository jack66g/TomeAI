Synapse: Self-Evolving Linux AI Agent
Synapse is an experimental C++ Linux Agent designed to demonstrate a Real-time Knowledge Distillation architecture. It solves the "stupid local model" problem by allowing a lightweight local model (SLM) to learn from a high-intelligence cloud model (DeepSeek) through continuous interaction and error correction.

⚠️ Current Status: This is a Proof of Concept. It currently supports CREATE and DELETE operations only and contains bugs. However, these bugs are intentional features—they generate the "failure cases" needed for the system to learn.

🏗️ Core Architecture: The Data Flywheel
Synapse is not just a tool; it is an automated data factory for training local AI agents.

Dual-Brain Routing:

Local Brain (C++): Fast, offline, handles simple commands.

Cloud Brain (DeepSeek): Acts as the "Teacher". It intervenes when the Local Brain fails or gets blocked by security rules.

Security Interception: A hard-coded C++ SecurityGuard prevents destructive commands (like rm -rf /), ensuring safe evolution.

Self-Correction Loop: Every interaction is logged. Cloud corrections are automatically harvested to fine-tune the local model.

⚡ Quick Start & Data Harvesting
1. Build
Bash

mkdir build && cd build
cmake ..
make
2. Run the Agent
Bash

./synapse
# Try commands like: "Create a file named test.txt on desktop"
3. 🔥 Automated Data Generation (Key Feature)
Want to make the local AI smarter without manual typing? We have included a chaos engineering script.

Run the stress test to automatically bombard the agent with random "Create File" requests. The system will attempt to execute them, fail, ask the Cloud Brain for help, and automatically save the correct logic as training data.

Bash

# This script will generate high-quality dataset for 'CREATE' operations
python3 ../tools/stress_test.py
Check training_data/ after running the script to see your newly harvested dataset!

Synapse: 自我进化的 Linux AI 智能体 (中文介绍)
Synapse 是一个极客向的 C++ Linux 智能体，旨在验证**“端云协同 + 知识蒸馏”**的架构思想。它的核心目标是解决本地小模型（SLM）不够聪明的问题，通过实时引入云端大模型（DeepSeek）的指导，实现“越用越强”的自我进化闭环。

⚠️ 现状说明： 这是一个验证性项目（PoC）。目前仅支持“创建”和“删除”文件，且代码中包含 Bug。但请注意，这些 Bug 是系统进化的养料——每一次报错和纠正，都会被系统转化为训练数据。

🏗️ 核心架构：数据飞轮
Synapse 不仅仅是一个助手，它是一个自动化的训练数据生产工厂。

双脑路由 (Dual-Brain)：

本地大脑 (C++)：响应快，负责处理简单指令。

云端大脑 (DeepSeek)：扮演“导师”角色。当本地大脑听不懂或犯错时，云端介入纠正。

安全卫士 (Security Guard)：C++ 写死的底层拦截层，防止 AI 生成毁灭性指令（如删库），确保进化过程安全。

闭环蒸馏 (Distillation)：所有的交互日志（包括失败和拦截）都会被自动清洗，生成的标准数据可直接用于微调本地模型。

⚡ 快速开始与数据获取
1. 编译
Bash

mkdir build && cd build
cmake ..
make
2. 运行助手
Bash

./synapse
# 尝试输入: "帮我在桌面建个1.txt" 或 "把 logs 文件夹删了"
3. 🔥 自动化获取训练数据 (核心功能)
你不需要手动打字来训练它。我们提供了一个压力测试脚本，可以模拟大量随机用户请求。

运行此脚本，系统会自动进行“创建文件”的压力测试。本地模型可能会失败，但云端模型会给出正确答案，这些“错误-修正”对会被自动保存为高质量的训练数据。

Bash

# 运行此脚本，自动获取关于“创建文件”意图的训练数据
python3 ../tools/stress_test.py
运行后，请查看 training_data/ 目录，你会发现数据集正在自动增长！