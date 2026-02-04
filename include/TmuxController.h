#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>

namespace pbterm {

class SshChannel;
class SshConnection;

// tmuxウィンドウ情報
struct TmuxWindow {
    int index;
    std::string name;
    bool active;
    std::string currentPath;  // pane_current_path
};

// tmuxセッション情報
struct TmuxSession {
    std::string name;
    int windowCount;
    bool attached;
    std::vector<TmuxWindow> windows;
};

// tmux制御クラス
// SSHの制御チャンネルを使ってtmuxコマンドを実行し、結果をパースする
class TmuxController {
public:
    using WindowListCallback = std::function<void(const std::vector<TmuxWindow>&)>;
    using SessionListCallback = std::function<void(const std::vector<TmuxSession>&)>;

    TmuxController();
    ~TmuxController();

    // SSH接続を設定（制御チャンネル用）
    void setConnection(SshConnection* connection);

    // 制御チャンネルを開く
    bool openControlChannel();
    void closeControlChannel();
    bool isControlChannelOpen() const { return m_controlChannelOpen; }

    // セッション名を設定/取得
    void setSessionName(const std::string& name) { m_sessionName = name; }
    std::string sessionName() const { return m_sessionName; }

    // tmuxセッションを開始またはアタッチ（非同期）
    // 成功するとonAttachedコールバックが呼ばれる
    bool startOrAttachSession();

    // tmuxセッションからデタッチ
    void detach();

    // セッションにアタッチしているか
    bool isAttached() const { return m_attached; }

    // ウィンドウ操作
    bool createWindow(const std::string& name = "");
    bool selectWindow(int index);
    bool closeWindow(int index);
    bool renameWindow(int index, const std::string& name);

    // ウィンドウ一覧を取得（同期）
    std::vector<TmuxWindow> listWindows();

    // 既存セッション一覧を取得（同期）
    std::vector<TmuxSession> listSessions();

    // 現在のウィンドウインデックス
    int currentWindowIndex() const { return m_currentWindowIndex; }

    // コールバック設定
    void setOnAttached(std::function<void()> callback) { m_onAttached = callback; }
    void setOnWindowListChanged(WindowListCallback callback) { m_onWindowListChanged = callback; }

    // ウィンドウ一覧を定期的に更新するためのタイマー処理
    void pollWindowList();

private:
    // 制御チャンネルでコマンドを実行し、結果を取得
    std::string executeCommand(const std::string& cmd, int timeoutMs = 2000);

    // tmux出力をパース
    std::vector<TmuxWindow> parseWindowList(const std::string& output);
    std::vector<TmuxSession> parseSessionList(const std::string& output);

    // 制御チャンネル読み取りスレッド
    void controlReaderThread();

    SshConnection* m_connection = nullptr;
    std::shared_ptr<SshChannel> m_controlChannel;
    std::string m_sessionName = "pbterm";
    std::string m_tmuxPath = "tmux";  // tmuxコマンドのパス
    std::vector<TmuxWindow> m_windows;
    int m_currentWindowIndex = 0;
    std::atomic<bool> m_attached{false};
    std::atomic<bool> m_controlChannelOpen{false};

    // 制御チャンネル用スレッド
    std::thread m_controlThread;
    std::atomic<bool> m_controlRunning{false};

    // コマンド応答待ち用
    std::mutex m_responseMutex;
    std::string m_responseBuffer;
    std::atomic<bool> m_waitingResponse{false};
    std::string m_responseMarker;  // 応答の終端マーカー

    // コールバック
    std::function<void()> m_onAttached;
    WindowListCallback m_onWindowListChanged;
};

} // namespace pbterm
