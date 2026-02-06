#pragma once

#include <string>
#include <memory>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "SettingsDialog.h"
#include <vector>

namespace pbterm {

class TerminalDock;
class ConnectionDialog;
class ProfileManager;
class SshConnection;
class TmuxController;
class CommandDock;
class FolderTreeDock;

// アプリケーションメインクラス
class App {
public:
    App();
    ~App();

    bool init();
    void run();
    void shutdown();

private:
    void setupImGui();
    void setupDocking();
    void renderMenuBar();
    void renderUI();
    void loadFont();
    void reloadFont();

    // GLFWコールバック
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
    void onFramebufferResize(int width, int height);

    // 接続管理
    void connect();
    void disconnect();

    // 設定適用
    void onSettingsApplied(const AppSettings& settings);

    GLFWwindow* m_window = nullptr;
    ImFont* m_font = nullptr;

    std::unique_ptr<TerminalDock> m_terminalDock;
    std::unique_ptr<ConnectionDialog> m_connectionDialog;
    std::unique_ptr<ProfileManager> m_profileManager;
    std::unique_ptr<SshConnection> m_sshConnection;
    std::unique_ptr<TmuxController> m_tmuxController;
    std::unique_ptr<SettingsDialog> m_settingsDialog;
    std::unique_ptr<CommandDock> m_commandDock;
    std::unique_ptr<FolderTreeDock> m_folderTreeDock;

    AppSettings m_appSettings;

    bool m_showTerminal = true;
    bool m_showCommands = true;
    bool m_showFolders = true;

    // アコーディオン状態
    bool m_commandsCollapsed = false;
    bool m_foldersCollapsed = false;
    float m_commandsExpandedWidth = 280.0f;
    float m_foldersExpandedWidth = 280.0f;
    float m_commandsExpandRequestWidth = 0.0f;
    float m_foldersExpandRequestWidth = 0.0f;

    ImGuiID m_dockspaceId = 0;
    ImGuiID m_foldersDockNodeId = 0;
    ImGuiID m_commandsDockNodeId = 0;
    bool m_showConnectionDialog = false;
    bool m_showSettings = false;
    bool m_connected = false;
    std::string m_connectedProfileName;  // 接続中のプロファイル名
    bool m_needFontReload = false;
    bool m_showTmuxMissing = false;

    int m_windowWidth = 1280;
    int m_windowHeight = 720;

    // ImGuiのiniファイルパス（文字列はヒープに保持する必要がある）
    std::string m_imguiIniPath;
};

} // namespace pbterm
