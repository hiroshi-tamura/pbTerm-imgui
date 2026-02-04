#include "CommandDock.h"
#include "SettingsDialog.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <cstdlib>

namespace pbterm {

// ローカライズ文字列
struct CommandDockLocalization {
    const char* addCommand;
    const char* editCommand;
    const char* deleteCommand;
    const char* commandName;
    const char* commandText;
    const char* add;
    const char* save;
    const char* cancel;
    const char* noCommands;
    const char* confirmDelete;
    const char* yes;
    const char* no;
};

static const CommandDockLocalization englishLoc = {
    "Add Command",
    "Edit Command",
    "Delete",
    "Name:",
    "Command:",
    "Add",
    "Save",
    "Cancel",
    "No commands registered.\nClick + to add a command.",
    "Delete this command?",
    "Yes",
    "No"
};

static const CommandDockLocalization japaneseLoc = {
    "コマンド追加",
    "コマンド編集",
    "削除",
    "名前:",
    "コマンド:",
    "追加",
    "保存",
    "キャンセル",
    "コマンドが登録されていません。\n+ をクリックして追加してください。",
    "このコマンドを削除しますか？",
    "はい",
    "いいえ"
};

static const CommandDockLocalization& getLoc(int language) {
    return (language == 1) ? japaneseLoc : englishLoc;
}

CommandDock::CommandDock() {
    load();
}

void CommandDock::render() {
    renderCommandList();
    renderAddDialog();
    renderEditDialog();
}

void CommandDock::renderCommandList() {
    const auto& loc = getLoc(m_language);

    // ツールバー
    if (ImGui::Button("+", ImVec2(30, 0))) {
        m_showAddDialog = true;
        std::memset(m_nameBuf, 0, sizeof(m_nameBuf));
        std::memset(m_commandBuf, 0, sizeof(m_commandBuf));
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", loc.addCommand);
    }

    ImGui::Separator();

    // コマンドリスト
    if (m_commands.empty()) {
        ImGui::TextWrapped("%s", loc.noCommands);
        return;
    }

    // スクロール可能なリスト
    ImGui::BeginChild("##commandList", ImVec2(0, 0), ImGuiChildFlags_None);

    int deleteIndex = -1;

    for (int i = 0; i < static_cast<int>(m_commands.size()); ++i) {
        const auto& cmd = m_commands[i];

        ImGui::PushID(i);

        // コマンドボタン（幅いっぱい）
        float buttonWidth = ImGui::GetContentRegionAvail().x;
        if (ImGui::Button(cmd.name.c_str(), ImVec2(buttonWidth, 0))) {
            // コマンドを送信（エンターなし）
            if (m_sendCallback) {
                m_sendCallback(cmd.command);
            }
        }

        // ツールチップでコマンド内容を表示
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", cmd.command.c_str());
        }

        // 右クリックでコンテキストメニュー
        if (ImGui::BeginPopupContextItem("##contextMenu")) {
            if (ImGui::MenuItem(loc.editCommand)) {
                m_editIndex = i;
                m_showEditDialog = true;
                std::strncpy(m_nameBuf, cmd.name.c_str(), sizeof(m_nameBuf) - 1);
                std::strncpy(m_commandBuf, cmd.command.c_str(), sizeof(m_commandBuf) - 1);
            }
            if (ImGui::MenuItem(loc.deleteCommand)) {
                deleteIndex = i;
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    // 削除処理
    if (deleteIndex >= 0) {
        removeCommand(deleteIndex);
    }

    ImGui::EndChild();
}

void CommandDock::renderAddDialog() {
    if (!m_showAddDialog) return;

    const auto& loc = getLoc(m_language);
    static bool focusName = false;
    static bool firstOpen = true;

    // ウィンドウを中央に配置（初回のみ）
    if (firstOpen) {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        firstOpen = false;
        focusName = true;
    }

    ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_Always);

    // 通常のウィンドウとして表示
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin(loc.addCommand, &m_showAddDialog, flags)) {
        ImGui::Text("%s", loc.commandName);
        ImGui::SetNextItemWidth(-1);
        if (focusName) {
            ImGui::SetKeyboardFocusHere();
            focusName = false;
        }
        ImGui::InputText("##name", m_nameBuf, sizeof(m_nameBuf));
        // IME確定時の全選択を解除（Enterキー検出）
        if (ImGui::IsItemActive() &&
            (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))) {
            if (ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetItemID())) {
                state->ClearSelection();
            }
        }

        ImGui::Spacing();

        ImGui::Text("%s", loc.commandText);
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##command", m_commandBuf, sizeof(m_commandBuf));
        // IME確定時の全選択を解除（Enterキー検出）
        if (ImGui::IsItemActive() &&
            (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))) {
            if (ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetItemID())) {
                state->ClearSelection();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 80.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalWidth = buttonWidth * 2 + spacing;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);

        if (ImGui::Button(loc.add, ImVec2(buttonWidth, 0))) {
            if (strlen(m_nameBuf) > 0 && strlen(m_commandBuf) > 0) {
                addCommand(m_nameBuf, m_commandBuf);
                m_showAddDialog = false;
                firstOpen = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.cancel, ImVec2(buttonWidth, 0))) {
            m_showAddDialog = false;
            firstOpen = true;
        }
    }
    ImGui::End();
}

void CommandDock::renderEditDialog() {
    if (!m_showEditDialog || m_editIndex < 0) return;

    const auto& loc = getLoc(m_language);
    static bool focusEditName = false;
    static bool firstOpen = true;

    // ウィンドウを中央に配置（初回のみ）
    if (firstOpen) {
        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        firstOpen = false;
        focusEditName = true;
    }

    ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_Always);

    // 通常のウィンドウとして表示
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin(loc.editCommand, &m_showEditDialog, flags)) {
        ImGui::Text("%s", loc.commandName);
        ImGui::SetNextItemWidth(-1);
        if (focusEditName) {
            ImGui::SetKeyboardFocusHere();
            focusEditName = false;
        }
        ImGui::InputText("##editName", m_nameBuf, sizeof(m_nameBuf));
        // IME確定時の全選択を解除（Enterキー検出）
        if (ImGui::IsItemActive() &&
            (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))) {
            if (ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetItemID())) {
                state->ClearSelection();
            }
        }

        ImGui::Spacing();

        ImGui::Text("%s", loc.commandText);
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##editCommand", m_commandBuf, sizeof(m_commandBuf));
        // IME確定時の全選択を解除（Enterキー検出）
        if (ImGui::IsItemActive() &&
            (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))) {
            if (ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetItemID())) {
                state->ClearSelection();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        float buttonWidth = 80.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalWidth = buttonWidth * 2 + spacing;
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalWidth) * 0.5f);

        if (ImGui::Button(loc.save, ImVec2(buttonWidth, 0))) {
            if (strlen(m_nameBuf) > 0 && strlen(m_commandBuf) > 0 &&
                m_editIndex >= 0 && m_editIndex < static_cast<int>(m_commands.size())) {
                m_commands[m_editIndex].name = m_nameBuf;
                m_commands[m_editIndex].command = m_commandBuf;
                save();
                m_showEditDialog = false;
                m_editIndex = -1;
                firstOpen = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.cancel, ImVec2(buttonWidth, 0))) {
            m_showEditDialog = false;
            m_editIndex = -1;
            firstOpen = true;
        }
    }
    ImGui::End();
}

void CommandDock::addCommand(const std::string& name, const std::string& command) {
    CommandShortcut cmd;
    cmd.name = name;
    cmd.command = command;
    m_commands.push_back(cmd);
    save();
}

void CommandDock::removeCommand(int index) {
    if (index >= 0 && index < static_cast<int>(m_commands.size())) {
        m_commands.erase(m_commands.begin() + index);
        save();
    }
}

void CommandDock::moveCommand(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= static_cast<int>(m_commands.size()) ||
        toIndex < 0 || toIndex >= static_cast<int>(m_commands.size())) {
        return;
    }

    CommandShortcut cmd = m_commands[fromIndex];
    m_commands.erase(m_commands.begin() + fromIndex);
    m_commands.insert(m_commands.begin() + toIndex, cmd);
    save();
}

void CommandDock::save() {
    std::string path = configPath();

    // ディレクトリ作成
    std::filesystem::path dir = std::filesystem::path(path).parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "コマンド設定ファイルを開けません: " << path << std::endl;
        return;
    }

    file << "command_count=" << m_commands.size() << "\n";

    for (const auto& cmd : m_commands) {
        file << "[command]\n";
        file << "name=" << cmd.name << "\n";
        file << "command=" << cmd.command << "\n";
    }

    std::cout << "コマンド設定保存: " << path << std::endl;
}

void CommandDock::load() {
    std::string path = configPath();
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cout << "コマンド設定ファイルなし: " << path << std::endl;
        return;
    }

    m_commands.clear();

    std::string line;
    CommandShortcut currentCmd;
    bool inCommand = false;

    auto getValue = [](const std::string& line, const std::string& key) -> std::string {
        if (line.find(key + "=") == 0) {
            return line.substr(key.length() + 1);
        }
        return "";
    };

    while (std::getline(file, line)) {
        // 改行を削除
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }

        if (line == "[command]") {
            if (inCommand && !currentCmd.name.empty()) {
                m_commands.push_back(currentCmd);
            }
            currentCmd = CommandShortcut();
            inCommand = true;
            continue;
        }

        if (inCommand) {
            std::string val;

            val = getValue(line, "name");
            if (!val.empty()) {
                currentCmd.name = val;
                continue;
            }

            val = getValue(line, "command");
            if (!val.empty()) {
                currentCmd.command = val;
                continue;
            }
        }
    }

    // 最後のコマンド
    if (inCommand && !currentCmd.name.empty()) {
        m_commands.push_back(currentCmd);
    }

    std::cout << "コマンド設定読み込み: " << m_commands.size() << " コマンド" << std::endl;
}

std::string CommandDock::configPath() const {
    return AppSettings::configDir() + "/commands.conf";
}

} // namespace pbterm
