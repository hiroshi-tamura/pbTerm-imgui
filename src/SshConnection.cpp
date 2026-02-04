#include "SshConnection.h"
#include <iostream>
#include <cstring>

namespace pbterm {

// ============================================================================
// SshChannel 実装
// ============================================================================

SshChannel::SshChannel(ssh_session session, std::mutex* sessionMutex)
    : m_session(session), m_sessionMutex(sessionMutex)
{
}

SshChannel::~SshChannel() {
    close();
}

bool SshChannel::openShell(int cols, int rows) {
    if (!m_session || !m_sessionMutex) {
        return false;
    }

    std::lock_guard<std::mutex> lock(*m_sessionMutex);

    // チャンネル作成
    m_channel = ssh_channel_new(m_session);
    if (!m_channel) {
        return false;
    }

    // セッションを開く
    int rc = ssh_channel_open_session(m_channel);
    if (rc != SSH_OK) {
        ssh_channel_free(m_channel);
        m_channel = nullptr;
        return false;
    }

    // PTY要求
    rc = ssh_channel_request_pty_size(m_channel, "xterm-256color", cols, rows);
    if (rc != SSH_OK) {
        ssh_channel_close(m_channel);
        ssh_channel_free(m_channel);
        m_channel = nullptr;
        return false;
    }

    // シェル要求
    rc = ssh_channel_request_shell(m_channel);
    if (rc != SSH_OK) {
        ssh_channel_close(m_channel);
        ssh_channel_free(m_channel);
        m_channel = nullptr;
        return false;
    }

    // 読み取りスレッド開始
    m_running = true;
    m_readerThread = std::thread(&SshChannel::readerThread, this);

    return true;
}

bool SshChannel::openExec() {
    if (!m_session || !m_sessionMutex) {
        return false;
    }

    // このモードではreaderThreadは使わない
    m_running = true;
    return true;
}

std::string SshChannel::exec(const std::string& cmd, int timeoutMs) {
    if (!m_session || !m_sessionMutex) {
        return "";
    }

    std::lock_guard<std::mutex> sessionLock(*m_sessionMutex);

    // 新しいチャンネルを作成（exec用）
    ssh_channel execChannel = ssh_channel_new(m_session);
    if (!execChannel) {
        return "";
    }

    int rc = ssh_channel_open_session(execChannel);
    if (rc != SSH_OK) {
        ssh_channel_free(execChannel);
        return "";
    }

    // コマンド実行
    rc = ssh_channel_request_exec(execChannel, cmd.c_str());
    if (rc != SSH_OK) {
        ssh_channel_close(execChannel);
        ssh_channel_free(execChannel);
        return "";
    }

    // 結果を読み取る
    std::string result;
    char buffer[4096];

    while (true) {
        int nbytes = ssh_channel_read_timeout(execChannel, buffer, sizeof(buffer) - 1, 0, timeoutMs);
        if (nbytes > 0) {
            buffer[nbytes] = '\0';
            result += buffer;
        } else if (nbytes == 0 || nbytes == SSH_EOF) {
            break;
        } else {
            break;
        }
    }

    ssh_channel_send_eof(execChannel);
    ssh_channel_close(execChannel);
    ssh_channel_free(execChannel);

    return result;
}

void SshChannel::close() {
    m_running = false;

    if (m_readerThread.joinable()) {
        m_readerThread.join();
    }

    if (m_channel) {
        ssh_channel_send_eof(m_channel);
        ssh_channel_close(m_channel);
        ssh_channel_free(m_channel);
        m_channel = nullptr;
    }
}

void SshChannel::write(const char* data, size_t len) {
    if (!m_channel || !m_running) return;

    std::lock_guard<std::mutex> lock(m_writeMutex);
    if (m_sessionMutex) {
        std::lock_guard<std::mutex> sessionLock(*m_sessionMutex);
        ssh_channel_write(m_channel, data, static_cast<uint32_t>(len));
    } else {
        ssh_channel_write(m_channel, data, static_cast<uint32_t>(len));
    }
}

void SshChannel::resize(int cols, int rows) {
    if (!m_channel || !m_sessionMutex) return;
    std::lock_guard<std::mutex> lock(*m_sessionMutex);
    ssh_channel_change_pty_size(m_channel, cols, rows);
}

void SshChannel::readerThread() {
    char buffer[4096];

    while (m_running && m_channel && m_sessionMutex) {
        int nbytes = 0;
        bool isEof = false;

        {
            std::lock_guard<std::mutex> lock(*m_sessionMutex);
            if (!m_channel) break;
            nbytes = ssh_channel_read_nonblocking(m_channel, buffer, sizeof(buffer), 0);
            isEof = ssh_channel_is_eof(m_channel);
        }

        if (nbytes > 0) {
            if (m_dataCallback) {
                m_dataCallback(buffer, static_cast<size_t>(nbytes));
            }
        } else if (nbytes == SSH_ERROR || isEof) {
            break;
        }

        // CPU使用率を下げるため少し待機
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ============================================================================
// SshConnection 実装
// ============================================================================

SshConnection::SshConnection() = default;

SshConnection::~SshConnection() {
    disconnect();
}

bool SshConnection::connect(const SshConfig& config) {
    if (m_connected) {
        disconnect();
    }

    // セッション作成
    m_session = ssh_new();
    if (!m_session) {
        m_lastError = "SSHセッション作成失敗";
        return false;
    }

    // オプション設定
    ssh_options_set(m_session, SSH_OPTIONS_HOST, config.host.c_str());
    ssh_options_set(m_session, SSH_OPTIONS_PORT, &config.port);
    ssh_options_set(m_session, SSH_OPTIONS_USER, config.username.c_str());

    // 接続
    int rc = ssh_connect(m_session);
    if (rc != SSH_OK) {
        m_lastError = std::string("接続失敗: ") + ssh_get_error(m_session);
        ssh_free(m_session);
        m_session = nullptr;
        return false;
    }

    // ユーザー認証
    bool authenticated = false;

    if (config.useKeyAuth && !config.privateKeyPath.empty()) {
        // 公開鍵認証
        rc = ssh_userauth_publickey_auto(m_session, nullptr, nullptr);
        if (rc == SSH_AUTH_SUCCESS) {
            authenticated = true;
        }
    }

    if (!authenticated && !config.password.empty()) {
        // パスワード認証
        rc = ssh_userauth_password(m_session, nullptr, config.password.c_str());
        if (rc == SSH_AUTH_SUCCESS) {
            authenticated = true;
        }
    }

    if (!authenticated) {
        // エージェント認証を試す
        rc = ssh_userauth_agent(m_session, nullptr);
        if (rc == SSH_AUTH_SUCCESS) {
            authenticated = true;
        }
    }

    if (!authenticated) {
        m_lastError = "認証失敗";
        ssh_disconnect(m_session);
        ssh_free(m_session);
        m_session = nullptr;
        return false;
    }

    m_connected = true;
    std::cout << "SSH接続成功: " << config.username << "@" << config.host << std::endl;
    return true;
}

void SshConnection::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // すべてのチャンネルを閉じる
    m_defaultChannel.reset();
    m_channels.clear();

    if (m_session) {
        if (m_connected) {
            ssh_disconnect(m_session);
        }
        ssh_free(m_session);
        m_session = nullptr;
    }
    m_connected = false;
}

bool SshConnection::isConnected() const {
    return m_connected && m_session != nullptr;
}

std::shared_ptr<SshChannel> SshConnection::createChannel(int cols, int rows) {
    if (!m_session || !m_connected) {
        m_lastError = "未接続";
        return nullptr;
    }

    auto channel = std::make_shared<SshChannel>(m_session, &m_mutex);
    if (!channel->openShell(cols, rows)) {
        m_lastError = "チャンネル作成失敗";
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_channels.push_back(channel);
    }
    std::cout << "新しいシェルセッションを開きました（合計: " << m_channels.size() << "）" << std::endl;
    return channel;
}

// 後方互換性のための旧API
bool SshConnection::openShell(int cols, int rows) {
    m_defaultChannel = createChannel(cols, rows);
    return m_defaultChannel != nullptr;
}

void SshConnection::closeShell() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_defaultChannel) {
        m_defaultChannel->close();
        m_defaultChannel.reset();
    }
}

void SshConnection::write(const char* data, size_t len) {
    if (m_defaultChannel) {
        m_defaultChannel->write(data, len);
    }
}

void SshConnection::setDataCallback(DataCallback callback) {
    if (m_defaultChannel) {
        m_defaultChannel->setDataCallback(std::move(callback));
    }
}

void SshConnection::resize(int cols, int rows) {
    if (m_defaultChannel) {
        m_defaultChannel->resize(cols, rows);
    }
}

std::string SshConnection::exec(const std::string& cmd, int timeoutMs) {
    if (!m_session || !m_connected) {
        return "";
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    // 新しいチャンネルを作成（exec用）
    ssh_channel execChannel = ssh_channel_new(m_session);
    if (!execChannel) {
        return "";
    }

    int rc = ssh_channel_open_session(execChannel);
    if (rc != SSH_OK) {
        ssh_channel_free(execChannel);
        return "";
    }

    // コマンド実行
    rc = ssh_channel_request_exec(execChannel, cmd.c_str());
    if (rc != SSH_OK) {
        ssh_channel_close(execChannel);
        ssh_channel_free(execChannel);
        return "";
    }

    // 結果を読み取る
    std::string result;
    char buffer[4096];

    while (true) {
        int nbytes = ssh_channel_read_timeout(execChannel, buffer, sizeof(buffer) - 1, 0, timeoutMs);
        if (nbytes > 0) {
            buffer[nbytes] = '\0';
            result += buffer;
        } else if (nbytes == 0 || nbytes == SSH_EOF) {
            break;
        } else {
            break;
        }
    }

    ssh_channel_send_eof(execChannel);
    ssh_channel_close(execChannel);
    ssh_channel_free(execChannel);

    // 末尾の改行を削除
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }

    return result;
}

} // namespace pbterm
