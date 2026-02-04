#pragma once

#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <memory>
#include <libssh/libssh.h>

namespace pbterm {

// SSH接続設定
struct SshConfig {
    std::string host;
    int port = 22;
    std::string username;
    std::string password;
    std::string privateKeyPath;
    bool useKeyAuth = true;
    bool autoConnect = false;

    bool isValid() const {
        return !host.empty() && !username.empty();
    }
};

// SSHチャンネル（個別のシェルセッション）
class SshChannel {
public:
    using DataCallback = std::function<void(const char*, size_t)>;

    SshChannel(ssh_session session, std::mutex* sessionMutex);
    ~SshChannel();

    // シェルを開く
    bool openShell(int cols, int rows);
    // コマンド実行用チャンネルを開く（PTYなし）
    bool openExec();
    void close();
    bool isOpen() const { return m_channel != nullptr && m_running; }

    // コマンドを実行して結果を取得（execモード用）
    std::string exec(const std::string& cmd, int timeoutMs = 5000);

    // データ送受信
    void write(const char* data, size_t len);
    void setDataCallback(DataCallback callback) { m_dataCallback = callback; }

    // ターミナルサイズ変更
    void resize(int cols, int rows);

private:
    void readerThread();

    ssh_session m_session = nullptr;
    ssh_channel m_channel = nullptr;
    std::mutex* m_sessionMutex = nullptr;  // セッション全体のミューテックス（共有）
    std::thread m_readerThread;
    std::atomic<bool> m_running{false};
    DataCallback m_dataCallback;
    std::mutex m_writeMutex;
};

// SSH接続クラス
class SshConnection {
public:
    using DataCallback = std::function<void(const char*, size_t)>;

    SshConnection();
    ~SshConnection();

    // 接続/切断
    bool connect(const SshConfig& config);
    void disconnect();
    bool isConnected() const;

    // 新しいチャンネル（シェル）を作成
    std::shared_ptr<SshChannel> createChannel(int cols, int rows);

    // 後方互換性のための旧API（最初のチャンネルを使用）
    bool openShell(int cols, int rows);
    void closeShell();
    void write(const char* data, size_t len);
    void setDataCallback(DataCallback callback);
    void resize(int cols, int rows);

    // コマンドを実行して結果を取得（execモード、PTYなし）
    std::string exec(const std::string& cmd, int timeoutMs = 5000);

    // エラーメッセージ
    std::string lastError() const { return m_lastError; }

private:
    ssh_session m_session = nullptr;
    std::vector<std::shared_ptr<SshChannel>> m_channels;
    std::shared_ptr<SshChannel> m_defaultChannel;
    std::atomic<bool> m_connected{false};
    std::string m_lastError;
    std::mutex m_mutex;
};

} // namespace pbterm
