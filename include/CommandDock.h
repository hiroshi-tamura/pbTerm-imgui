#pragma once

#include <string>
#include <vector>
#include <functional>

namespace pbterm {

// コマンドショートカット
struct CommandShortcut {
    std::string name;       // 表示名
    std::string command;    // 実行するコマンド
};

// コマンドショートカットドック
class CommandDock {
public:
    using SendCommandCallback = std::function<void(const std::string&)>;

    CommandDock();
    ~CommandDock() = default;

    // 言語設定
    void setLanguage(int language) { m_language = language; }

    // コマンド送信コールバック
    void setSendCommandCallback(SendCommandCallback callback) {
        m_sendCallback = std::move(callback);
    }

    // 描画
    void render();

    // コマンド操作
    void addCommand(const std::string& name, const std::string& command);
    void removeCommand(int index);
    void moveCommand(int fromIndex, int toIndex);

    // 保存/読み込み
    void save();
    void load();

private:
    void renderCommandList();
    void renderAddDialog();
    void renderEditDialog();
    void renderContextMenu();

    std::string configPath() const;

    std::vector<CommandShortcut> m_commands;
    SendCommandCallback m_sendCallback;

    int m_language = 0;

    // UI状態
    bool m_showAddDialog = false;
    bool m_showEditDialog = false;
    int m_editIndex = -1;
    int m_contextMenuIndex = -1;

    // 入力バッファ
    char m_nameBuf[128] = {};
    char m_commandBuf[512] = {};
};

} // namespace pbterm
