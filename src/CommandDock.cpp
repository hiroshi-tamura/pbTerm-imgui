#include "CommandDock.h"
#include "SettingsDialog.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <algorithm>

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
    renderAddGroupDialog();
    renderEditGroupDialog();
    renderDeleteGroupDialog();
}

void CommandDock::renderCommandList() {
    const auto& loc = getLoc(m_language);

    // ツールバー
    if (ImGui::Button("Command+", ImVec2(0, 0))) {
        m_showAddDialog = true;
        std::memset(m_nameBuf, 0, sizeof(m_nameBuf));
        std::memset(m_commandBuf, 0, sizeof(m_commandBuf));
        m_selectedGroupIndex = 0;
        std::memset(m_groupBuf, 0, sizeof(m_groupBuf));
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", loc.addCommand);
    }

    ImGui::SameLine();
    if (ImGui::Button("Group+", ImVec2(0, 0))) {
        m_showAddGroupDialog = true;
        std::memset(m_groupBuf, 0, sizeof(m_groupBuf));
    }

    ImGui::Separator();

    // コマンドリスト
    if (m_commands.empty()) {
        ImGui::TextWrapped("%s", loc.noCommands);
        return;
    }

    // スクロール可能なリスト
    ImGui::BeginChild("##commandList", ImVec2(0, 0), ImGuiChildFlags_None);

    // グループごとのインデックス収集
    std::vector<int> ungrouped;
    std::vector<std::vector<int>> grouped(m_groups.size());

    for (int i = 0; i < static_cast<int>(m_commands.size()); ++i) {
        const auto& cmd = m_commands[i];
        if (cmd.group.empty()) {
            ungrouped.push_back(i);
        } else {
            auto it = std::find(m_groups.begin(), m_groups.end(), cmd.group);
            if (it != m_groups.end()) {
                grouped[static_cast<size_t>(std::distance(m_groups.begin(), it))].push_back(i);
            } else {
                ungrouped.push_back(i);
            }
        }
    }

    // グループ表示
    for (size_t gi = 0; gi < m_groups.size(); ++gi) {
        renderGroupSection(m_groups[gi], grouped[gi]);
    }

    // 未分類
    renderGroupSection("", ungrouped);

    ImGui::EndChild();
}

void CommandDock::renderGroupSection(const std::string& groupName, const std::vector<int>& indices) {
    const auto& loc = getLoc(m_language);

    std::string header = groupName.empty() ? "Ungrouped" : groupName;

    if (!groupName.empty()) {
        ImGui::PushID(groupName.c_str());
        ImGui::Selectable(header.c_str(), false, ImGuiSelectableFlags_SpanAvailWidth | ImGuiSelectableFlags_AllowOverlap);
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PBTERM_GROUP")) {
                if (payload->DataSize == sizeof(int)) {
                    int from = *static_cast<const int*>(payload->Data);
                    int to = static_cast<int>(std::find(m_groups.begin(), m_groups.end(), groupName) - m_groups.begin());
                    moveGroup(from, to);
                }
            }
            ImGui::EndDragDropTarget();
        }
        if (ImGui::BeginDragDropSource()) {
            int groupIndex = static_cast<int>(std::find(m_groups.begin(), m_groups.end(), groupName) - m_groups.begin());
            ImGui::SetDragDropPayload("PBTERM_GROUP", &groupIndex, sizeof(int));
            ImGui::TextUnformatted(groupName.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Edit")) {
            m_showEditGroupDialog = true;
            m_editGroupOldName = groupName;
            std::strncpy(m_groupBuf, groupName.c_str(), sizeof(m_groupBuf) - 1);
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Del")) {
            m_showDeleteGroupDialog = true;
            m_editGroupOldName = groupName;
        }
        ImGui::PopID();
    } else {
        ImGui::TextUnformatted(header.c_str());
    }

    float lineH = ImGui::GetFrameHeightWithSpacing();
    float height = lineH * static_cast<float>(std::max<size_t>(1, indices.size())) + ImGui::GetStyle().FramePadding.y * 2.0f + 2.0f;
    float width = ImGui::GetContentRegionAvail().x;

    // ドロップ先は枠内のみ（ヘッダー除外）
    ImVec2 childPos = ImGui::GetCursorScreenPos();
    ImRect dropRect(childPos, ImVec2(childPos.x + width, childPos.y + height));
    ImGuiID dropId = ImGui::GetID((std::string("##drop_") + header).c_str());
    if (ImGui::BeginDragDropTargetCustom(dropRect, dropId)) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PBTERM_CMD_IDX")) {
            if (payload->DataSize == sizeof(int)) {
                int idx = *static_cast<const int*>(payload->Data);
                moveCommandToGroup(idx, groupName);
            }
        }
        ImGui::EndDragDropTarget();
    }
    ImGuiWindowFlags childFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));
    ImGui::BeginChild((std::string("##group_") + header).c_str(), ImVec2(0, height),
                      ImGuiChildFlags_Borders, childFlags);

    for (int i : indices) {
        const auto& cmd = m_commands[i];

        ImGui::PushID(i);

        // コマンドボタン（幅いっぱい）
        float buttonWidth = ImGui::GetContentRegionAvail().x;
        if (ImGui::Button(cmd.name.c_str(), ImVec2(buttonWidth, 0))) {
            if (m_sendCallback) {
                m_sendCallback(cmd.command);
            }
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PBTERM_CMD_IDX")) {
                if (payload->DataSize == sizeof(int)) {
                    int from = *static_cast<const int*>(payload->Data);
                    if (m_commands[from].group == cmd.group) {
                        moveCommandBefore(from, i);
                    } else {
                        moveCommandToGroup(from, cmd.group);
                        moveCommandBefore(from, i);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("PBTERM_CMD_IDX", &i, sizeof(int));
            ImGui::TextUnformatted(cmd.name.c_str());
            ImGui::EndDragDropSource();
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
                std::memset(m_groupBuf, 0, sizeof(m_groupBuf));

                // グループ選択
                m_selectedGroupIndex = 0;
                if (!cmd.group.empty()) {
                    for (size_t gi = 0; gi < m_groups.size(); ++gi) {
                        if (m_groups[gi] == cmd.group) {
                            m_selectedGroupIndex = static_cast<int>(gi + 1);
                            break;
                        }
                    }
                }
            }
            if (ImGui::MenuItem(loc.deleteCommand)) {
                removeCommand(i);
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    if (indices.empty()) {
        ImGui::TextDisabled("(empty)");
    }
    // 最下部ドロップゾーン
    {
        ImVec2 dzPos = ImGui::GetCursorScreenPos();
        float dzH = 4.0f;
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, dzH));
        ImRect dzRect(dzPos, ImVec2(dzPos.x + ImGui::GetContentRegionAvail().x, dzPos.y + dzH));
        ImGuiID dzId = ImGui::GetID((std::string("##cmd_drop_end_") + header).c_str());
        if (ImGui::BeginDragDropTargetCustom(dzRect, dzId)) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PBTERM_CMD_IDX")) {
                if (payload->DataSize == sizeof(int)) {
                    int from = *static_cast<const int*>(payload->Data);
                    moveCommandToGroup(from, groupName);
                    int insertIndex = from;
                    for (int idx : indices) {
                        insertIndex = idx;
                    }
                    if (!indices.empty()) {
                        moveCommandBefore(from, insertIndex + 1);
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    // 下側の余白
    ImGui::Dummy(ImVec2(0.0f, 9.0f));

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::Spacing();
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

        // グループ選択
        ImGui::Text("Group:");
        std::vector<const char*> groupItems;
        groupItems.push_back("(None)");
        for (const auto& g : m_groups) groupItems.push_back(g.c_str());
        groupItems.push_back("(New)");

        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##group", &m_selectedGroupIndex, groupItems.data(), static_cast<int>(groupItems.size()));

        if (m_selectedGroupIndex == static_cast<int>(groupItems.size()) - 1) {
            ImGui::InputText("##newGroupName", m_groupBuf, sizeof(m_groupBuf));
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

        // グループ選択
        ImGui::Text("Group:");
        std::vector<const char*> groupItems;
        groupItems.push_back("(None)");
        for (const auto& g : m_groups) groupItems.push_back(g.c_str());
        groupItems.push_back("(New)");

        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##editGroup", &m_selectedGroupIndex, groupItems.data(), static_cast<int>(groupItems.size()));

        if (m_selectedGroupIndex == static_cast<int>(groupItems.size()) - 1) {
            ImGui::InputText("##editNewGroupName", m_groupBuf, sizeof(m_groupBuf));
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

                std::string groupName;
                if (m_selectedGroupIndex > 0 &&
                    m_selectedGroupIndex < static_cast<int>(groupItems.size()) - 1) {
                    groupName = m_groups[static_cast<size_t>(m_selectedGroupIndex - 1)];
                } else if (m_selectedGroupIndex == static_cast<int>(groupItems.size()) - 1 &&
                           std::strlen(m_groupBuf) > 0) {
                    groupName = m_groupBuf;
                    if (std::find(m_groups.begin(), m_groups.end(), groupName) == m_groups.end()) {
                        m_groups.push_back(groupName);
                    }
                }

                m_commands[m_editIndex].group = groupName;
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
    std::string groupName;
    if (m_selectedGroupIndex > 0 &&
        m_selectedGroupIndex < static_cast<int>(m_groups.size()) + 1) {
        groupName = m_groups[static_cast<size_t>(m_selectedGroupIndex - 1)];
    } else if (m_selectedGroupIndex == static_cast<int>(m_groups.size()) + 1 &&
               std::strlen(m_groupBuf) > 0) {
        groupName = m_groupBuf;
        if (std::find(m_groups.begin(), m_groups.end(), groupName) == m_groups.end()) {
            m_groups.push_back(groupName);
        }
    }
    cmd.group = groupName;
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

    file << "group_count=" << m_groups.size() << "\n";
    for (const auto& g : m_groups) {
        file << "[group]\n";
        file << "name=" << g << "\n";
    }

    file << "command_count=" << m_commands.size() << "\n";

    for (const auto& cmd : m_commands) {
        file << "[command]\n";
        file << "name=" << cmd.name << "\n";
        file << "command=" << cmd.command << "\n";
        file << "group=" << cmd.group << "\n";
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
    m_groups.clear();

    std::string line;
    CommandShortcut currentCmd;
    bool inCommand = false;
    bool inGroup = false;
    std::string currentGroup;

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

        if (line == "[group]") {
            if (inGroup && !currentGroup.empty()) {
                m_groups.push_back(currentGroup);
            }
            currentGroup.clear();
            inGroup = true;
            inCommand = false;
            continue;
        }

        if (line == "[command]") {
            if (inCommand && !currentCmd.name.empty()) {
                m_commands.push_back(currentCmd);
            }
            currentCmd = CommandShortcut();
            inCommand = true;
            inGroup = false;
            continue;
        }

        if (inGroup) {
            std::string val;
            val = getValue(line, "name");
            if (!val.empty()) {
                currentGroup = val;
                continue;
            }
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

            val = getValue(line, "group");
            if (!val.empty()) {
                currentCmd.group = val;
                continue;
            }
        }
    }

    // 最後のコマンド
    if (inCommand && !currentCmd.name.empty()) {
        m_commands.push_back(currentCmd);
    }

    if (inGroup && !currentGroup.empty()) {
        m_groups.push_back(currentGroup);
    }

    // コマンド側のグループを反映（旧フォーマット対応）
    for (const auto& cmd : m_commands) {
        if (!cmd.group.empty() &&
            std::find(m_groups.begin(), m_groups.end(), cmd.group) == m_groups.end()) {
            m_groups.push_back(cmd.group);
        }
    }

    std::cout << "コマンド設定読み込み: " << m_commands.size() << " コマンド" << std::endl;
}

std::string CommandDock::configPath() const {
    return AppSettings::configDir() + "/commands.conf";
}

void CommandDock::renderAddGroupDialog() {
    if (!m_showAddGroupDialog) return;

    ImGui::OpenPopup("Add Group");
    if (ImGui::BeginPopupModal("Add Group", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Group Name");
        ImGui::SetNextItemWidth(240);
        ImGui::InputText("##groupName", m_groupBuf, sizeof(m_groupBuf));
        ImGui::Spacing();

        if (ImGui::Button("Add", ImVec2(100, 0))) {
            if (std::strlen(m_groupBuf) > 0) {
                std::string g = m_groupBuf;
                if (std::find(m_groups.begin(), m_groups.end(), g) == m_groups.end()) {
                    m_groups.push_back(g);
                    save();
                }
            }
            m_showAddGroupDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            m_showAddGroupDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void CommandDock::moveCommandToGroup(int index, const std::string& groupName) {
    if (index < 0 || index >= static_cast<int>(m_commands.size())) return;
    m_commands[index].group = groupName;
    save();
}

void CommandDock::moveCommandBefore(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= static_cast<int>(m_commands.size())) return;
    if (toIndex < 0 || toIndex >= static_cast<int>(m_commands.size())) return;
    if (fromIndex == toIndex) return;

    CommandShortcut cmd = m_commands[fromIndex];
    m_commands.erase(m_commands.begin() + fromIndex);
    if (fromIndex < toIndex) {
        toIndex -= 1;
    }
    m_commands.insert(m_commands.begin() + toIndex, cmd);
    save();
}

void CommandDock::renderEditGroupDialog() {
    if (!m_showEditGroupDialog) return;

    ImGui::OpenPopup("Edit Group");
    if (ImGui::BeginPopupModal("Edit Group", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Group Name");
        ImGui::SetNextItemWidth(240);
        ImGui::InputText("##editGroupName", m_groupBuf, sizeof(m_groupBuf));
        ImGui::Spacing();

        if (ImGui::Button("Save", ImVec2(100, 0))) {
            if (std::strlen(m_groupBuf) > 0) {
                renameGroup(m_editGroupOldName, m_groupBuf);
            }
            m_showEditGroupDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            m_showEditGroupDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void CommandDock::renderDeleteGroupDialog() {
    if (!m_showDeleteGroupDialog) return;

    ImGui::OpenPopup("Delete Group");
    if (ImGui::BeginPopupModal("Delete Group", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Delete group '%s'?", m_editGroupOldName.c_str());
        ImGui::Spacing();
        if (ImGui::Button("Delete", ImVec2(100, 0))) {
            deleteGroup(m_editGroupOldName);
            m_showDeleteGroupDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            m_showDeleteGroupDialog = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void CommandDock::renameGroup(const std::string& oldName, const std::string& newName) {
    if (oldName.empty() || newName.empty()) return;
    if (oldName == newName) return;

    auto it = std::find(m_groups.begin(), m_groups.end(), oldName);
    if (it != m_groups.end()) {
        *it = newName;
    }

    for (auto& cmd : m_commands) {
        if (cmd.group == oldName) {
            cmd.group = newName;
        }
    }
    save();
}

void CommandDock::deleteGroup(const std::string& name) {
    if (name.empty()) return;
    m_groups.erase(std::remove(m_groups.begin(), m_groups.end(), name), m_groups.end());
    for (auto& cmd : m_commands) {
        if (cmd.group == name) {
            cmd.group.clear();
        }
    }
    save();
}

void CommandDock::moveGroup(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= static_cast<int>(m_groups.size())) return;
    if (toIndex < 0 || toIndex > static_cast<int>(m_groups.size())) return;
    if (fromIndex == toIndex) return;

    std::string g = m_groups[static_cast<size_t>(fromIndex)];
    m_groups.erase(m_groups.begin() + fromIndex);
    if (fromIndex < toIndex) {
        toIndex -= 1;
    }
    m_groups.insert(m_groups.begin() + toIndex, g);
    save();
}
} // namespace pbterm
