#include "TmuxController.h"
#include "SshConnection.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <random>

namespace pbterm {

TmuxController::TmuxController() = default;

TmuxController::~TmuxController() {
    closeControlChannel();
}

void TmuxController::setConnection(SshConnection* connection) {
    m_connection = connection;
}

bool TmuxController::openControlChannel() {
    if (!m_connection || !m_connection->isConnected()) {
        std::cerr << "TmuxController: SSH未接続" << std::endl;
        return false;
    }

    // execモードでは専用チャンネルは不要（exec毎に新しいチャンネルを作成）
    m_controlChannelOpen = true;
    std::cout << "TmuxController: 制御チャンネルを開きました（execモード）" << std::endl;

    return true;
}

void TmuxController::closeControlChannel() {
    m_controlRunning = false;

    if (m_controlThread.joinable()) {
        m_controlThread.join();
    }

    if (m_controlChannel) {
        m_controlChannel->close();
        m_controlChannel.reset();
    }

    m_controlChannelOpen = false;
    m_attached = false;
}

std::string TmuxController::executeCommand(const std::string& cmd, int timeoutMs) {
    if (!m_connection || !m_controlChannelOpen) {
        std::cerr << "TmuxController: 制御チャンネルが開いていません" << std::endl;
        return "";
    }

    // SshConnection::exec()を使ってコマンドを実行（クリーンな出力を取得）
    return m_connection->exec(cmd, timeoutMs);
}

bool TmuxController::startOrAttachSession() {
    if (!m_controlChannelOpen) {
        if (!openControlChannel()) {
            return false;
        }
    }

    // tmuxのパスを検出（よくあるパスを試す）
    std::vector<std::string> tmuxPaths = {
        "/opt/homebrew/bin/tmux",  // macOS ARM Homebrew
        "/usr/local/bin/tmux",      // macOS Intel Homebrew / Linux
        "/usr/bin/tmux",            // Linux system
        "tmux"                      // PATH内
    };

    m_tmuxPath.clear();
    for (const auto& path : tmuxPaths) {
        std::string testCmd = "test -x " + path + " && echo 'OK'";
        std::string result = executeCommand(testCmd);
        if (result.find("OK") != std::string::npos) {
            m_tmuxPath = path;
            break;
        }
    }

    if (m_tmuxPath.empty()) {
        std::cerr << "TmuxController: tmuxが見つかりません" << std::endl;
        m_attached = false;
        return false;
    }
    std::cout << "TmuxController: tmuxパス: " << m_tmuxPath << std::endl;

    // 既存セッションを確認
    std::string checkCmd = m_tmuxPath + " has-session -t " + m_sessionName + " 2>/dev/null && echo 'EXISTS' || echo 'NOT_EXISTS'";
    std::string result = executeCommand(checkCmd);

    bool sessionExists = (result.find("EXISTS") != std::string::npos);

    if (sessionExists) {
        std::cout << "TmuxController: 既存セッション '" << m_sessionName << "' にアタッチします" << std::endl;
    } else {
        std::cout << "TmuxController: 新規セッション '" << m_sessionName << "' を作成します" << std::endl;
        // 新規セッション作成（デタッチ状態で）
        std::string createCmd = m_tmuxPath + " new-session -d -s " + m_sessionName;
        executeCommand(createCmd);
    }

    m_attached = true;

    // ウィンドウ一覧を取得
    m_windows = listWindows();

    if (m_onAttached) {
        m_onAttached();
    }

    std::cout << "TmuxController: セッションにアタッチしました（ウィンドウ数: " << m_windows.size() << "）" << std::endl;
    return true;
}

void TmuxController::detach() {
    m_attached = false;
    m_windows.clear();
}

bool TmuxController::createWindow(const std::string& name) {
    if (!m_attached || !m_controlChannelOpen) {
        return false;
    }

    std::string cmd;
    if (name.empty()) {
        cmd = m_tmuxPath + " new-window -t " + m_sessionName;
    } else {
        cmd = m_tmuxPath + " new-window -t " + m_sessionName + " -n '" + name + "'";
    }

    std::string result = executeCommand(cmd);

    // ウィンドウ一覧を更新
    m_windows = listWindows();

    if (m_onWindowListChanged) {
        m_onWindowListChanged(m_windows);
    }

    std::cout << "TmuxController: 新しいウィンドウを作成しました" << std::endl;
    return true;
}

bool TmuxController::selectWindow(int index) {
    if (!m_attached || !m_controlChannelOpen) {
        return false;
    }

    std::string cmd = m_tmuxPath + " select-window -t " + m_sessionName + ":" + std::to_string(index);
    executeCommand(cmd);

    m_currentWindowIndex = index;
    return true;
}

bool TmuxController::closeWindow(int index) {
    if (!m_attached || !m_controlChannelOpen) {
        return false;
    }

    std::string cmd = m_tmuxPath + " kill-window -t " + m_sessionName + ":" + std::to_string(index);
    executeCommand(cmd);

    // ウィンドウ一覧を更新
    m_windows = listWindows();

    if (m_onWindowListChanged) {
        m_onWindowListChanged(m_windows);
    }

    return true;
}

bool TmuxController::renameWindow(int index, const std::string& name) {
    if (!m_attached || !m_controlChannelOpen) {
        return false;
    }

    std::string cmd = m_tmuxPath + " rename-window -t " + m_sessionName + ":" + std::to_string(index) + " '" + name + "'";
    executeCommand(cmd);

    // ウィンドウ一覧を更新
    m_windows = listWindows();

    return true;
}

std::vector<TmuxWindow> TmuxController::listWindows() {
    if (!m_controlChannelOpen) {
        return {};
    }

    // tmux list-windows でウィンドウ一覧を取得
    // フォーマット: index:name:active:pane_current_path
    std::string cmd = m_tmuxPath + " list-windows -t " + m_sessionName + " -F '#{window_index}:#{window_name}:#{window_active}:#{pane_current_path}'";
    std::string result = executeCommand(cmd);

    std::cout << "TmuxController::listWindows 結果: [" << result << "]" << std::endl;

    return parseWindowList(result);
}

std::vector<TmuxWindow> TmuxController::parseWindowList(const std::string& output) {
    std::vector<TmuxWindow> windows;

    std::istringstream iss(output);
    std::string line;

    while (std::getline(iss, line)) {
        // 空行をスキップ
        if (line.empty() || line[0] == '\r') continue;

        // フォーマット: index:name:active:path
        std::vector<std::string> parts;
        std::string part;
        std::istringstream lineStream(line);

        while (std::getline(lineStream, part, ':')) {
            parts.push_back(part);
        }

        if (parts.size() >= 3) {
            TmuxWindow win;
            try {
                win.index = std::stoi(parts[0]);
            } catch (...) {
                continue;
            }
            win.name = parts[1];
            win.active = (parts[2] == "1");
            if (parts.size() >= 4) {
                win.currentPath = parts[3];
            }
            windows.push_back(win);

            if (win.active) {
                m_currentWindowIndex = win.index;
            }
        }
    }

    m_windows = windows;
    return windows;
}

std::vector<TmuxSession> TmuxController::listSessions() {
    if (!m_controlChannelOpen) {
        return {};
    }

    // tmux list-sessions でセッション一覧を取得
    std::string cmd = m_tmuxPath + " list-sessions -F '#{session_name}:#{session_windows}:#{session_attached}'";
    std::string result = executeCommand(cmd);

    return parseSessionList(result);
}

std::vector<TmuxSession> TmuxController::parseSessionList(const std::string& output) {
    std::vector<TmuxSession> sessions;

    std::istringstream iss(output);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.empty() || line[0] == '\r') continue;

        // フォーマット: name:window_count:attached
        size_t pos1 = line.find(':');
        size_t pos2 = line.find(':', pos1 + 1);

        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            TmuxSession session;
            session.name = line.substr(0, pos1);
            try {
                session.windowCount = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
            } catch (...) {
                session.windowCount = 0;
            }
            session.attached = (line.substr(pos2 + 1) == "1");
            sessions.push_back(session);
        }
    }

    return sessions;
}

void TmuxController::pollWindowList() {
    if (!m_attached || !m_controlChannelOpen) {
        return;
    }

    // 現在のウィンドウ一覧を取得
    auto newWindows = listWindows();

    // 変更があればコールバックを呼ぶ
    bool changed = (newWindows.size() != m_windows.size());
    if (!changed) {
        for (size_t i = 0; i < newWindows.size(); ++i) {
            if (newWindows[i].index != m_windows[i].index ||
                newWindows[i].name != m_windows[i].name ||
                newWindows[i].active != m_windows[i].active) {
                changed = true;
                break;
            }
        }
    }

    if (changed && m_onWindowListChanged) {
        m_onWindowListChanged(newWindows);
    }

    m_windows = newWindows;
}

} // namespace pbterm
