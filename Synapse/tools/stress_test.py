import pexpect
import time
import sys
import os
import random
import string
from datetime import datetime
from openai import OpenAI

# ================= ⚙️ 核心配置 =================
SYNAPSE_PATH = "../build/synapse"
API_KEY = "灵芽密钥" # 👈 记得填 Key
BASE_URL = "https://api.lingyaai.cn/v1"
MODEL_NAME = "grok-4-1-fast-non-reasoning"

# 休眠时间 (测试时设短点，挂机建议 300)
INTERVAL_SECONDS = 10 
# ===============================================

client = OpenAI(api_key=API_KEY, base_url=BASE_URL)

# 🎭 定义角色画像 (Personas)
PERSONAS = {
    "developer": {
        "role_desc": "你是一个急躁的程序员。",
        "topics": ["python脚本", "C++源码", "配置文件", "接口文档", "测试用例"],
        "filenames": ["main", "utils", "config", "test_api", "app", "schema"],
        "extensions": [".py", ".cpp", ".json", ".yaml", ".js"],
        "paths": ["project", "src", "dev", "code", "workspace"]
    },
    "office": {
        "role_desc": "你是一个行政文员，不懂技术，说话很客气。",
        "topics": ["会议记录", "周报", "待办事项", "简历", "通知"],
        "filenames": ["2026会议记录", "张三简历", "1月周报", "todo", "notice"],
        "extensions": [".txt", ".docx", ".md", ".xlsx"],
        "paths": ["Desktop", "桌面", "Documents", "文档"]
    },
    "sysadmin": {
        "role_desc": "你是一个Linux系统管理员，指令简练。",
        "topics": ["系统日志", "数据库备份", "错误报告", "临时文件"],
        "filenames": ["syslog", "db_backup", "error", "temp_check", "auth"],
        "extensions": [".log", ".bak", ".tar.gz", ".sh"],
        "paths": ["/var/log", "/tmp", "/etc/conf", "backup"]
    }
}

# 当前轮次的角色上下文
current_persona = None

def check_ai_connectivity():
    print(f"\n📡 [System] 正在连接云端大脑...")
    try:
        client.chat.completions.create(model=MODEL_NAME, messages=[{"role":"user","content":"Hi"}], max_tokens=1, timeout=10)
        print(f"✅ [Online] 连接成功！")
        return True
    except Exception as e:
        print(f"❌ [Error] 连接失败: {e}")
        return False

def get_contextual_filename():
    """根据当前角色生成有意义的文件名"""
    if current_persona:
        base = random.choice(current_persona["filenames"])
        # 30% 概率加个随机后缀让它不重复
        if random.random() < 0.3:
            return f"{base}_{random.randint(1,99)}"
        return base
    return "file_" + str(random.randint(100,999))

def get_contextual_path():
    """根据当前角色生成路径"""
    if current_persona:
        return random.choice(current_persona["paths"])
    return "桌面"

def get_contextual_extension():
    """根据当前角色生成后缀"""
    if current_persona:
        return random.choice(current_persona["extensions"])
    return ".txt"

def generate_human_prompt(round_id):
    global current_persona
    
    # 随机选一个角色
    persona_key = random.choice(list(PERSONAS.keys()))
    current_persona = PERSONAS[persona_key]
    
    topic = random.choice(current_persona["topics"])
    
    print(f"\n🎭 [Roleplay] 当前扮演: {persona_key.upper()} (话题: {topic})")
    
    system_prompt = f"""
    {current_persona['role_desc']}
    请生成一个**创建文件**的口语化指令，关于"{topic}"。
    要求：
    1. 像人类一样说话，可以包含"帮我"、"弄个"、"整一个"等词。
    2. 有时候带路径，有时候不带。
    3. 有时候带后缀，有时候不带。
    4. 不要带引号，只输出指令文本。
    """
    
    try:
        completion = client.chat.completions.create(
            model=MODEL_NAME,
            messages=[
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": "生成一条指令"}
            ], timeout=15
        )
        cmd = completion.choices[0].message.content.strip().replace('"', '')
        print(f"🔥 [Human] 发射指令: {cmd}")
        return cmd
    except:
        return "创建文件"

def run_human_test():
    if not os.path.exists(SYNAPSE_PATH): return
    if not check_ai_connectivity(): return

    print(f"🚀 启动 Synapse 拟人化测试...")
    child = pexpect.spawn(SYNAPSE_PATH, encoding='utf-8', timeout=60)
    child.logfile_read = sys.stdout 

    try:
        child.expect("Ready.")
        print("✅ 内核就绪...")

        round_count = 1
        while True:
            print(f"\n{'='*20} Round {round_count} {'='*20}")
            prompt = generate_human_prompt(round_count)
            child.sendline(prompt)
            
            while True:
                index = child.expect([
                    "请输入后缀",            # 0
                    "请选择",                # 1
                    "审计完成",              # 2
                    "Ready.",               # 3
                    pexpect.TIMEOUT,        # 4
                    "叫什么名字",            # 5
                    "请输入文件名",          # 6
                    "请问放在哪里",          # 7
                    r"还[缺需].*个",          # 8
                    "找不到.*请重新输入"      # 9
                ])

                # === 🤖 更加智能的应答逻辑 ===
                
                if index == 0: # 问后缀
                    # 优先用角色习惯的后缀
                    ext = get_contextual_extension()
                    print(f"\n🧠 [Context] 根据角色习惯，输入后缀 -> {ext}")
                    child.sendline(ext)
                
                elif index == 1: # 路径选择
                    print("\n👀 [Auto] 路径多选 -> 1")
                    child.sendline("1") 
                
                elif index == 2: # 成功
                    print("\n🎉 [Success] 交互完成！")
                    break 
                
                elif index == 3: # 结束
                    break 
                
                elif index == 4: # 超时
                    print("\n⚠️ 超时重置...")
                    child.sendline("") 
                    break

                elif index == 5 or index == 6 or index == 8: # 问文件名
                    # 优先用角色习惯的文件名
                    name = get_contextual_filename()
                    
                    # 20% 概率触发 "自动"
                    if random.random() < 0.2:
                        print(f"\n⚡ [Action] 懒得想名字 -> '自动'")
                        child.sendline("自动")
                    else:
                        print(f"\n🧠 [Context] 根据角色习惯，输入文件名 -> {name}")
                        child.sendline(name)

                elif index == 7: # 问路径
                     # 优先用角色习惯的路径
                    path = get_contextual_path()
                    print(f"\n🧠 [Context] 根据角色习惯，输入路径 -> {path}")
                    child.sendline(path)

                elif index == 9: # 路径错误
                    print("\n🛡️ [Safe] 路径错误保底 -> 桌面")
                    child.sendline("桌面")

            print(f"\n💤 休眠 {INTERVAL_SECONDS} 秒...")
            time.sleep(INTERVAL_SECONDS)
            round_count += 1
            if round_count % 10 == 0:
                child.sendline("") 
                child.expect("Ready.")

    except KeyboardInterrupt:
        print("\n🛑 测试停止")
    finally:
        child.close()

if __name__ == "__main__":
    run_human_test()