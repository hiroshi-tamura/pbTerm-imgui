#pragma once

#include <string>
#include <vector>
#include <functional>

namespace pbterm {

// アプリケーション設定
struct AppSettings {
    // 言語設定（0: English, 1: 日本語）
    int language = 0;

    // フォント設定
    std::string fontPath = "resources/fonts/HackGenConsole-Regular.ttf";
    float fontSize = 18.0f;

    // ウィンドウ設定
    int windowX = -1;       // -1 = 中央配置
    int windowY = -1;
    int windowWidth = 1280;
    int windowHeight = 720;
    bool windowMaximized = false;

    // 保存/読み込み
    void save();
    void load();

    // 設定ディレクトリのパスを取得
    static std::string configDir();

private:
    std::string configPath() const;
};

// ローカライズ文字列
struct Localization {
    // メニュー
    const char* menuConnect;
    const char* menuStart;
    const char* menuStop;
    const char* menuSetting;
    const char* menuView;
    const char* menuTerminal;
    const char* menuCommands;
    const char* menuFolders;
    const char* menuOption;

    // 接続状態
    const char* statusConnected;
    const char* statusDisconnected;

    // 接続ダイアログ
    const char* dlgConnectionTitle;
    const char* dlgProfile;
    const char* dlgNewProfile;
    const char* dlgSave;
    const char* dlgRename;
    const char* dlgDelete;
    const char* dlgHost;
    const char* dlgPort;
    const char* dlgUsername;
    const char* dlgAuthMethod;
    const char* dlgPublicKey;
    const char* dlgPrivateKey;
    const char* dlgPassword;
    const char* dlgAutoConnect;
    const char* dlgConnect;
    const char* dlgCancel;
    const char* dlgSaveProfile;
    const char* dlgProfileName;
    const char* dlgRenameProfile;
    const char* dlgRenameProfileName;

    // 自動接続確認ダイアログ
    const char* dlgAutoConnectConfirm;
    const char* dlgAutoConnectMessage;
    const char* dlgYes;
    const char* dlgNo;

    // 設定ダイアログ
    const char* dlgSettingsTitle;
    const char* dlgLanguage;
    const char* dlgFont;
    const char* dlgFontSize;
    const char* dlgOK;
    const char* dlgApply;

    // ターミナル
    const char* termPleaseConnect;

    // フォルダツリー
    const char* dockFoldersTitle;
    const char* folderNew;
    const char* folderDelete;
    const char* folderNewTitle;
    const char* folderNewName;
    const char* folderCreate;
    const char* folderCancel;
    const char* folderDeleteTitle;
    const char* folderDeleteMessage;
    const char* folderDeleteYes;
    const char* folderDeleteNo;
};

// 言語取得
const Localization& getLocalization(int language);

// 設定ダイアログ
class SettingsDialog {
public:
    using ApplyCallback = std::function<void(const AppSettings&)>;

    SettingsDialog();

    void setSettings(const AppSettings& settings);
    AppSettings getSettings() const { return m_settings; }

    void setApplyCallback(ApplyCallback callback) { m_applyCallback = std::move(callback); }

    void render(bool* open);

private:
    void renderLanguageSettings();
    void renderFontSettings();
    void renderButtons(bool* open);

    AppSettings m_settings;
    ApplyCallback m_applyCallback;

    // UI状態
    int m_selectedLanguage = 0;
    int m_selectedFont = 0;
    float m_fontSize = 18.0f;

    // 利用可能なフォント
    std::vector<std::string> m_availableFonts;
};

} // namespace pbterm
