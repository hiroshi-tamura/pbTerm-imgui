#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include "imgui.h"

namespace pbterm {

class Terminal;
class SshConnection;
class SshChannel;
class TmuxController;
struct TmuxWindow;

// ターミナルタブ情報（tmuxウィンドウに対応）
// 実際のターミナル表示は共有（tmuxの仕様上、同時に複数ウィンドウを見ることはできない）
struct TerminalTabInfo {
    int id;                          // ローカルID
    int tmuxWindowIndex = -1;        // tmuxウィンドウインデックス
    std::string name;                // タブ名（tmuxウィンドウ名）
    std::string currentPath;         // アクティブな作業ディレクトリ
};

// ターミナルドック
// tmuxセッションに接続し、各ウィンドウをタブとして表示
// タブ切り替え時にtmuxウィンドウも切り替わる
class TerminalDock {
public:
    TerminalDock();
    ~TerminalDock();

    // SSH接続とtmuxコントローラを設定
    void setConnection(SshConnection* connection);
    void setTmuxController(TmuxController* tmux);

    // 言語設定
    void setLanguage(int language) { m_language = language; }

    // カラーテーマ設定
    void setColorTheme(const std::string& themeId);

    // 接続状態
    void onConnected();
    void onDisconnected();
    bool isConnected() const { return m_connected; }

    // 描画
    void render(ImFont* font);

    // タブ操作
    int addTab(const std::string& name = "");
    void closeTab(int index);
    void setActiveTab(int index);
    int activeTabIndex() const { return m_activeTab; }
    int tabCount() const { return static_cast<int>(m_tabs.size()); }
    std::string activePath() const;

    // アクティブターミナル
    Terminal* activeTerminal();

    // テキストを送信（コマンドショートカット用）
    void sendText(const std::string& text);

    // tmuxウィンドウ一覧が更新された時のコールバック
    void onTmuxWindowListChanged(const std::vector<TmuxWindow>& windows);

private:
    void renderTabs(ImFont* font);
    void renderTerminal(ImFont* font);
    std::string generateTabName();

    // tmuxセッションにアタッチするSSHチャンネルを開く
    bool openTmuxSession();

    // tmuxなしで直接シェルを開く
    bool openDirectShell();

    // 現在のタブに対応するtmuxウィンドウを選択
    void selectTmuxWindow(int windowIndex);

    std::vector<TerminalTabInfo> m_tabs;
    int m_activeTab = -1;
    int m_nextTabId = 1;
    SshConnection* m_connection = nullptr;
    TmuxController* m_tmuxController = nullptr;
    bool m_connected = false;
    int m_language = 0;

    // 共有のターミナル（tmuxの画面を表示）
    std::unique_ptr<Terminal> m_terminal;
    std::shared_ptr<SshChannel> m_channel;

    // タブ幅計算用
    float m_tabHeight = 0;
    float m_closeButtonSize = 16.0f;
    float m_addButtonSize = 20.0f;

    std::chrono::steady_clock::time_point m_lastWindowPoll;
};

} // namespace pbterm
