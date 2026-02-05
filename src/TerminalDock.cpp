#include "TerminalDock.h"
#include "Terminal.h"
#include "SshConnection.h"
#include "TmuxController.h"
#include "SettingsDialog.h"
#include <algorithm>
#include <iostream>
#include <thread>
#include <chrono>

namespace pbterm {

TerminalDock::TerminalDock() = default;

TerminalDock::~TerminalDock() = default;

void TerminalDock::setConnection(SshConnection* connection) {
    m_connection = connection;
}

void TerminalDock::setTmuxController(TmuxController* tmux) {
    m_tmuxController = tmux;
}

void TerminalDock::onConnected() {
    m_connected = true;
    m_tabs.clear();
    m_activeTab = -1;

    if (m_channel) {
        m_channel->close();
        m_channel.reset();
    }
    m_terminal.reset();

    // tmuxが利用可能かチェック
    bool tmuxAvailable = (m_tmuxController && m_tmuxController->isAttached());

    if (tmuxAvailable) {
        // tmuxセッションに接続
        if (!openTmuxSession()) {
            std::cerr << "TerminalDock: tmuxセッションへの接続失敗、通常モードで起動" << std::endl;
            tmuxAvailable = false;
        }
    }

    if (tmuxAvailable) {
        // 既存ウィンドウのタブを作成
        auto windows = m_tmuxController->listWindows();
        std::cout << "TerminalDock: tmuxウィンドウを復元中（" << windows.size() << "個）" << std::endl;

        for (const auto& win : windows) {
            std::cout << "TerminalDock: タブ作成中: index=" << win.index << " name=" << win.name << std::endl;
            TerminalTabInfo tab;
            tab.id = m_nextTabId++;
            tab.tmuxWindowIndex = win.index;
            tab.name = win.name.empty() ? ("Window " + std::to_string(win.index)) : win.name;
            tab.currentPath = win.currentPath;
            m_tabs.push_back(tab);

            if (win.active) {
                m_activeTab = static_cast<int>(m_tabs.size()) - 1;
            }
        }
        std::cout << "TerminalDock: 全タブ作成完了 (" << m_tabs.size() << "個)" << std::endl;

        // タブがない場合は最初のタブを追加
        if (m_tabs.empty()) {
            addTab();
        }

        // アクティブなウィンドウを選択
        std::cout << "TerminalDock: アクティブタブ選択中: " << m_activeTab << std::endl;
        if (m_activeTab >= 0 && m_activeTab < static_cast<int>(m_tabs.size())) {
            selectTmuxWindow(m_tabs[m_activeTab].tmuxWindowIndex);
        }
        std::cout << "TerminalDock: onConnected完了" << std::endl;
    } else {
        // tmuxなしモード：単純なSSHシェル
        std::cout << "TerminalDock: tmuxなしモードで起動" << std::endl;
        openDirectShell();
    }
}

void TerminalDock::onDisconnected() {
    m_connected = false;
    m_tabs.clear();
    m_activeTab = -1;

    if (m_channel) {
        m_channel->close();
        m_channel.reset();
    }
    m_terminal.reset();
}

bool TerminalDock::openTmuxSession() {
    if (!m_connection || !m_connection->isConnected() || !m_tmuxController) {
        return false;
    }

    // ターミナル作成
    m_terminal = std::make_unique<Terminal>(80, 24);

    // SSHチャンネル作成
    m_channel = m_connection->createChannel(80, 24);
    if (!m_channel) {
        std::cerr << "TerminalDock: チャンネル作成失敗" << std::endl;
        return false;
    }

    // ターミナルにチャンネルを設定
    m_terminal->setChannel(m_channel);

    // tmuxセッションにアタッチ（Homebrewのパスを含む）
    std::string sessionName = m_tmuxController->sessionName();
    std::string attachCmd = "export PATH=/opt/homebrew/bin:/usr/local/bin:$PATH; tmux attach-session -t " + sessionName + " || tmux new-session -s " + sessionName + "\n";

    // 少し待ってからコマンドを送信（シェルの起動を待つ）
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    m_channel->write(attachCmd.c_str(), attachCmd.size());

    std::cout << "TerminalDock: tmuxセッション '" << sessionName << "' に接続" << std::endl;
    return true;
}

bool TerminalDock::openDirectShell() {
    if (!m_connection || !m_connection->isConnected()) {
        return false;
    }

    // ターミナル作成
    m_terminal = std::make_unique<Terminal>(80, 24);

    // SSHチャンネル作成
    m_channel = m_connection->createChannel(80, 24);
    if (!m_channel) {
        std::cerr << "TerminalDock: チャンネル作成失敗" << std::endl;
        return false;
    }

    // ターミナルにチャンネルを設定
    m_terminal->setChannel(m_channel);

    // 単純なタブを追加
    TerminalTabInfo tab;
    tab.id = m_nextTabId++;
    tab.tmuxWindowIndex = -1;
    tab.name = "Shell";
    m_tabs.push_back(tab);
    m_activeTab = 0;

    std::cout << "TerminalDock: 直接シェル接続" << std::endl;
    return true;
}

void TerminalDock::selectTmuxWindow(int windowIndex) {
    if (!m_channel || windowIndex < 0) {
        return;
    }

    if (m_tmuxController) {
        m_tmuxController->selectWindow(windowIndex);

        // 画面をリフレッシュするためにCtrl-L（画面再描画）を送信
        // tmuxセッション内で画面を強制的に更新
        const char ctrlL = '\x0c';  // Ctrl-L
        m_channel->write(&ctrlL, 1);
    }
}

void TerminalDock::render(ImFont* font) {
    if (!m_connected) {
        // 未接続時のメッセージ
        const Localization& loc = getLocalization(m_language);
        ImVec2 windowSize = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize(loc.termPleaseConnect);
        ImGui::SetCursorPos(ImVec2(
            (windowSize.x - textSize.x) * 0.5f,
            (windowSize.y - textSize.y) * 0.5f
        ));
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", loc.termPleaseConnect);
        return;
    }

    // tmuxの現在パスを定期的に更新
    if (m_tmuxController && m_tmuxController->isAttached()) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastWindowPoll).count();
        if (elapsed > 1000) {
            auto windows = m_tmuxController->listWindows();
            onTmuxWindowListChanged(windows);
            m_lastWindowPoll = now;
        }
    }

    // タブバーとターミナル
    renderTabs(font);

    if (m_activeTab >= 0 && m_activeTab < static_cast<int>(m_tabs.size())) {
        renderTerminal(font);
    }
}

void TerminalDock::renderTabs(ImFont* font) {
    ImGuiStyle& style = ImGui::GetStyle();
    m_tabHeight = ImGui::GetFrameHeight();

    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    float tabBarWidth = contentRegion.x;

    // タブバー背景
    ImVec2 tabBarPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(
        tabBarPos,
        ImVec2(tabBarPos.x + tabBarWidth, tabBarPos.y + m_tabHeight),
        IM_COL32(45, 45, 45, 255)
    );

    ImGui::BeginGroup();

    float xOffset = 0.0f;

    // 閉じるタブを記録
    int tabToClose = -1;

    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        TerminalTabInfo& tab = m_tabs[i];

        // タブの幅を計算
        ImVec2 textSize = ImGui::CalcTextSize(tab.name.c_str());
        float tabWidth = textSize.x + m_closeButtonSize + style.FramePadding.x * 4;

        // タブ背景
        ImVec2 tabPos(tabBarPos.x + xOffset, tabBarPos.y);
        ImVec2 tabEnd(tabPos.x + tabWidth, tabPos.y + m_tabHeight);

        bool isActive = (i == m_activeTab);
        bool isHovered = ImGui::IsMouseHoveringRect(tabPos, tabEnd);

        ImU32 tabColor;
        if (isActive) {
            tabColor = IM_COL32(30, 30, 30, 255);
        } else if (isHovered) {
            tabColor = IM_COL32(60, 60, 60, 255);
        } else {
            tabColor = IM_COL32(50, 50, 50, 255);
        }

        drawList->AddRectFilled(tabPos, tabEnd, tabColor);

        // タブの境界線
        if (isActive) {
            drawList->AddLine(
                ImVec2(tabPos.x, tabPos.y),
                ImVec2(tabEnd.x, tabPos.y),
                IM_COL32(80, 140, 200, 255),
                2.0f
            );
        }

        // タブ名
        ImVec2 textPos(tabPos.x + style.FramePadding.x, tabPos.y + (m_tabHeight - textSize.y) * 0.5f);
        drawList->AddText(textPos, IM_COL32(220, 220, 220, 255), tab.name.c_str());

        // クローズボタン
        float closeBtnX = tabEnd.x - m_closeButtonSize - style.FramePadding.x;
        float closeBtnY = tabPos.y + (m_tabHeight - m_closeButtonSize) * 0.5f;
        ImVec2 closePos(closeBtnX, closeBtnY);
        ImVec2 closeEnd(closeBtnX + m_closeButtonSize, closeBtnY + m_closeButtonSize);

        bool closeHovered = ImGui::IsMouseHoveringRect(closePos, closeEnd);

        // クローズボタン背景（ホバー時）
        if (closeHovered) {
            drawList->AddRectFilled(
                closePos, closeEnd,
                IM_COL32(255, 255, 255, 40),
                2.0f
            );
        }

        // ×マーク（常に表示）
        ImU32 closeColor = closeHovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(180, 180, 180, 255);
        float cx = closeBtnX + m_closeButtonSize * 0.5f;
        float cy = closeBtnY + m_closeButtonSize * 0.5f;
        float cr = m_closeButtonSize * 0.25f;
        drawList->AddLine(ImVec2(cx - cr, cy - cr), ImVec2(cx + cr, cy + cr), closeColor, 1.5f);
        drawList->AddLine(ImVec2(cx + cr, cy - cr), ImVec2(cx - cr, cy + cr), closeColor, 1.5f);

        // クリック処理
        if (ImGui::IsMouseClicked(0)) {
            if (closeHovered) {
                tabToClose = i;
            } else if (isHovered && !isActive) {
                // タブ切り替え
                m_activeTab = i;
                selectTmuxWindow(tab.tmuxWindowIndex);
            }
        }

        xOffset += tabWidth + 2.0f; // タブ間のマージン
    }

    // +ボタン
    float addBtnX = tabBarPos.x + xOffset + 4.0f;
    float addBtnY = tabBarPos.y + (m_tabHeight - m_addButtonSize) * 0.5f;
    ImVec2 addPos(addBtnX, addBtnY);
    ImVec2 addEnd(addBtnX + m_addButtonSize, addBtnY + m_addButtonSize);

    bool addHovered = ImGui::IsMouseHoveringRect(addPos, addEnd);

    // +ボタン背景（ホバー時）
    if (addHovered) {
        drawList->AddRectFilled(
            addPos, addEnd,
            IM_COL32(255, 255, 255, 30),
            2.0f
        );
    }

    // +マーク
    ImU32 addColor = addHovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(150, 150, 150, 255);
    float ax = addBtnX + m_addButtonSize * 0.5f;
    float ay = addBtnY + m_addButtonSize * 0.5f;
    float ar = m_addButtonSize * 0.3f;
    drawList->AddLine(ImVec2(ax - ar, ay), ImVec2(ax + ar, ay), addColor, 2.0f);
    drawList->AddLine(ImVec2(ax, ay - ar), ImVec2(ax, ay + ar), addColor, 2.0f);

    // +ボタンクリック
    if (addHovered && ImGui::IsMouseClicked(0)) {
        addTab();
    }

    ImGui::EndGroup();

    // タブバー分の高さを進める
    ImGui::Dummy(ImVec2(tabBarWidth, m_tabHeight));

    // タブを閉じる
    if (tabToClose >= 0) {
        closeTab(tabToClose);
    }
}

void TerminalDock::renderTerminal(ImFont* font) {
    if (!m_terminal) {
        return;
    }

    // ターミナルサイズ調整
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    ImVec2 charSize = font ? ImGui::CalcTextSize("A") : ImVec2(8, 16);

    int newCols = static_cast<int>(contentRegion.x / charSize.x);
    int newRows = static_cast<int>(contentRegion.y / charSize.y);

    if (newCols > 0 && newRows > 0) {
        if (newCols != m_terminal->cols() || newRows != m_terminal->rows()) {
            m_terminal->resize(newCols, newRows);
        }
    }

    m_terminal->render(font);
}

int TerminalDock::addTab(const std::string& name) {
    if (!m_connection || !m_connected) {
        std::cerr << "タブ追加失敗: 未接続" << std::endl;
        return -1;
    }

    // tmuxコントローラでウィンドウを作成
    if (m_tmuxController && m_tmuxController->isAttached()) {
        std::string windowName = name.empty() ? "" : name;
        if (!m_tmuxController->createWindow(windowName)) {
            std::cerr << "タブ追加失敗: tmuxウィンドウ作成失敗" << std::endl;
            return -1;
        }

        // 新しいウィンドウ一覧を取得
        auto windows = m_tmuxController->listWindows();
        if (windows.empty()) {
            std::cerr << "タブ追加失敗: ウィンドウ一覧取得失敗" << std::endl;
            return -1;
        }

        // 最新のウィンドウ（最後に作成されたもの）を取得
        const auto& newWindow = windows.back();

        // タブ情報を追加
        TerminalTabInfo tab;
        tab.id = m_nextTabId++;
        tab.tmuxWindowIndex = newWindow.index;
        tab.name = newWindow.name.empty() ? ("Window " + std::to_string(newWindow.index)) : newWindow.name;
        tab.currentPath = newWindow.currentPath;
        m_tabs.push_back(tab);

        // 新しいタブをアクティブに
        m_activeTab = static_cast<int>(m_tabs.size()) - 1;

        // tmuxウィンドウを選択
        selectTmuxWindow(newWindow.index);

        std::cout << "タブ追加成功: " << tab.name << " (tmux window " << newWindow.index << ")" << std::endl;
        std::cout << "m_terminal=" << (m_terminal ? "OK" : "NULL") << " m_channel=" << (m_channel ? "OK" : "NULL") << std::endl;
        return m_activeTab;
    }

    // tmuxなしの場合（フォールバック）
    TerminalTabInfo tab;
    tab.id = m_nextTabId++;
    tab.tmuxWindowIndex = -1;
    tab.name = name.empty() ? generateTabName() : name;
    m_tabs.push_back(tab);

    m_activeTab = static_cast<int>(m_tabs.size()) - 1;
    std::cout << "タブ追加成功: " << tab.name << std::endl;
    return m_activeTab;
}

void TerminalDock::closeTab(int index) {
    if (index < 0 || index >= static_cast<int>(m_tabs.size())) {
        return;
    }

    // tmuxウィンドウも閉じる
    if (m_tmuxController && m_tabs[index].tmuxWindowIndex >= 0) {
        m_tmuxController->closeWindow(m_tabs[index].tmuxWindowIndex);
    }

    m_tabs.erase(m_tabs.begin() + index);

    // アクティブタブ調整
    if (m_tabs.empty()) {
        m_activeTab = -1;
    } else if (m_activeTab >= static_cast<int>(m_tabs.size())) {
        m_activeTab = static_cast<int>(m_tabs.size()) - 1;
        // 新しいアクティブタブのウィンドウを選択
        if (m_activeTab >= 0) {
            selectTmuxWindow(m_tabs[m_activeTab].tmuxWindowIndex);
        }
    } else if (m_activeTab >= index) {
        if (m_activeTab > 0) {
            m_activeTab--;
        }
        if (m_activeTab >= 0 && m_activeTab < static_cast<int>(m_tabs.size())) {
            selectTmuxWindow(m_tabs[m_activeTab].tmuxWindowIndex);
        }
    }
}

void TerminalDock::setActiveTab(int index) {
    if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
        m_activeTab = index;
        selectTmuxWindow(m_tabs[index].tmuxWindowIndex);
    }
}

Terminal* TerminalDock::activeTerminal() {
    return m_terminal.get();
}

std::string TerminalDock::activePath() const {
    if (m_activeTab >= 0 && m_activeTab < static_cast<int>(m_tabs.size())) {
        return m_tabs[m_activeTab].currentPath;
    }
    return std::string();
}

void TerminalDock::sendText(const std::string& text) {
    if (!m_connected || !m_channel || text.empty()) {
        return;
    }

    // テキストをSSHチャンネルに送信
    m_channel->write(text.c_str(), text.length());
}

void TerminalDock::onTmuxWindowListChanged(const std::vector<TmuxWindow>& windows) {
    // 既存タブと新しいウィンドウリストを同期

    // 現在のタブのtmuxWindowIndexを集める
    std::vector<int> currentIndices;
    for (const auto& tab : m_tabs) {
        if (tab.tmuxWindowIndex >= 0) {
            currentIndices.push_back(tab.tmuxWindowIndex);
        }
    }

    // 新しいウィンドウインデックスを集める
    std::vector<int> newIndices;
    for (const auto& win : windows) {
        newIndices.push_back(win.index);
    }

    // 削除されたウィンドウのタブを削除
    m_tabs.erase(
        std::remove_if(m_tabs.begin(), m_tabs.end(), [&newIndices](const TerminalTabInfo& tab) {
            if (tab.tmuxWindowIndex < 0) return false;
            return std::find(newIndices.begin(), newIndices.end(), tab.tmuxWindowIndex) == newIndices.end();
        }),
        m_tabs.end()
    );

    // 新しいウィンドウのタブを追加、既存タブの名前を更新
    for (const auto& win : windows) {
        bool found = false;
        for (auto& tab : m_tabs) {
            if (tab.tmuxWindowIndex == win.index) {
                // 名前更新
                tab.name = win.name.empty() ? ("Window " + std::to_string(win.index)) : win.name;
                tab.currentPath = win.currentPath;
                found = true;
                break;
            }
        }
        if (!found) {
            TerminalTabInfo tab;
            tab.id = m_nextTabId++;
            tab.tmuxWindowIndex = win.index;
            tab.name = win.name.empty() ? ("Window " + std::to_string(win.index)) : win.name;
            tab.currentPath = win.currentPath;
            m_tabs.push_back(tab);
        }
    }

    // アクティブタブの調整
    if (m_tabs.empty()) {
        m_activeTab = -1;
    } else if (m_activeTab >= static_cast<int>(m_tabs.size())) {
        m_activeTab = static_cast<int>(m_tabs.size()) - 1;
    }
}

std::string TerminalDock::generateTabName() {
    return "Terminal " + std::to_string(m_nextTabId);
}

} // namespace pbterm
