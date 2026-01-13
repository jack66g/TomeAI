// 核心引入
const { invoke } = window.__TAURI__.core;
const { Command } = window.__TAURI__.shell;

const chatHistory = document.getElementById('chat-history');
const userInput = document.getElementById('user-input');
const sendBtn = document.getElementById('send-btn');
const statusText = document.getElementById('status-text');
const avatar = document.getElementById('avatar');

let backendProcess = null;

async function startBackend() {
  console.log('准备启动后端...');
  
  const cmd = Command.sidecar('synapse');

  cmd.stdout.on('data', (line) => {
    const cleanLine = line.trim();
    if (!cleanLine) return;
    console.log('Backend:', cleanLine);

    if (cleanLine.startsWith('[THINK]')) {
        appendThinking(cleanLine.substring(8));
        updateStatus("思考中...", true);
    } else if (cleanLine.startsWith('[RESULT]')) {
        appendAIMessage(cleanLine.substring(9));
        updateStatus("就绪", false);
    } else if (cleanLine.startsWith('[ERROR]')) {
        appendError(cleanLine.substring(8));
        updateStatus("就绪", false); // 出错后也要重置状态
    } else {
        // ✅ 修改 1: 必须加这个 else！
        // 用来显示没有前缀的普通对话，比如 "请问文件要叫什么名字？"
        appendAIMessage(cleanLine);
        updateStatus("等待输入...", false);
    }
  });

  cmd.stderr.on('data', line => console.error(`Backend Error: ${line}`));
  
  cmd.on('close', () => {
      appendError("后端断开连接");
      updateStatus("已断开", false);
  });
  
  cmd.on('error', error => {
      console.error('启动报错:', error);
      appendError("启动失败: " + error);
  });

  try {
      backendProcess = await cmd.spawn();
      console.log('后端启动成功, PID:', backendProcess.pid);
      updateStatus("就绪", false);
  } catch (e) {
      console.error(e);
      appendError("权限拒绝或启动失败: " + e);
  }
}

async function sendMessage() {
  // ✅ 修改 2: 允许发送空回车
  // 获取原始输入，不立即 trim，因为我们需要判断用户是不是只按了回车
  const rawText = userInput.value;
  
  // 如果用户输入的是空的（或者只有空格），我们发送一个空格 " " 给后端
  // 后端的 globalTrim 会把这个空格变成空字符串，从而触发“使用默认值”
  const textToSend = rawText.trim() === "" ? " " : rawText.trim();

  if (!backendProcess) return;

  // UI 显示优化：如果是空回车，显示一个符号提示用户
  if (rawText.trim() === "") {
      appendUserMessage("↩︎ (默认)");
  } else {
      appendUserMessage(rawText.trim());
  }
  
  userInput.value = '';

  try {
      await backendProcess.write(textToSend + '\n');
      updateStatus("发送中...", true);
  } catch (e) {
      appendError("发送失败: " + e);
  }
}

// === UI 函数 (保持不变) ===
function appendUserMessage(text) {
  const d = document.createElement('div');
  d.className = 'message user-message';
  d.textContent = text;
  chatHistory.appendChild(d);
  chatHistory.scrollTop = chatHistory.scrollHeight;
}

function appendAIMessage(text) {
  const d = document.createElement('div');
  d.className = 'message ai-message';
  d.innerHTML = text.replace(/\n/g, '<br>');
  chatHistory.appendChild(d);
  chatHistory.scrollTop = chatHistory.scrollHeight;
}

function appendThinking(text) {
    const d = document.createElement('div');
    d.className = 'thinking-process';
    d.textContent = '🧠 ' + text;
    chatHistory.appendChild(d);
    chatHistory.scrollTop = chatHistory.scrollHeight;
}

function appendError(text) {
    const d = document.createElement('div');
    d.className = 'thinking-process';
    d.style.color = 'red';
    d.textContent = '⚠️ ' + text;
    chatHistory.appendChild(d);
}

function updateStatus(text, isBusy) {
    statusText.textContent = text;
    if (isBusy) avatar.style.animationDuration = '1s';
    else avatar.style.animationDuration = '3s';
}

sendBtn.addEventListener('click', sendMessage);
userInput.addEventListener('keypress', (e) => { if (e.key === 'Enter') sendMessage(); });
window.addEventListener('DOMContentLoaded', startBackend);