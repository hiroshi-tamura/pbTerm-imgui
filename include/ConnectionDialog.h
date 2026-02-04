#pragma once

#include <string>
#include <functional>
#include "SshConnection.h"

namespace pbterm {

class ProfileManager;

// 接続設定ダイアログ
class ConnectionDialog {
public:
    using ConnectCallback = std::function<void(const SshConfig&)>;

    ConnectionDialog(ProfileManager* profileManager);
    ~ConnectionDialog() = default;

    // 言語設定
    void setLanguage(int language) { m_language = language; }

    // 描画
    void render(bool* open);

    // コールバック設定
    void setConnectCallback(ConnectCallback callback) { m_connectCallback = callback; }

    // 現在の設定を取得
    const SshConfig& config() const { return m_config; }

    // ダイアログを開く時に呼び出す（プロファイル状態を初期化）
    void initForShow();

    // プロファイルを選択
    void selectProfile(int index);

private:
    void renderProfileSelector();
    void renderConnectionSettings();
    void renderButtons(bool* open);

    ProfileManager* m_profileManager;
    SshConfig m_config;
    ConnectCallback m_connectCallback;

    int m_language = 0;

    // 入力バッファ
    char m_hostBuf[256] = {};
    char m_userBuf[128] = {};
    char m_passBuf[128] = {};
    char m_keyPathBuf[512] = {};
    int m_port = 22;
    bool m_useKeyAuth = true;
    bool m_autoConnect = false;

    int m_selectedProfile = 0;
    char m_newProfileName[64] = {};
    bool m_showSaveDialog = false;

    // 自動接続切り替え確認ダイアログ
    bool m_showAutoConnectConfirm = false;
    std::string m_existingAutoConnectProfile;  // 既に自動接続が設定されているプロファイル名
};

} // namespace pbterm
