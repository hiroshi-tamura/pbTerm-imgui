#include "FolderTreeDock.h"
#include "SshConnection.h"
#include "TerminalDock.h"
#include "SettingsDialog.h"
#include "imgui.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <filesystem>
#include <thread>
#include <iostream>
#include <cstdlib>

namespace pbterm {

namespace {
constexpr const char* kIconFolder = u8"\ue2c7"; // folder
constexpr const char* kIconFile = u8"\ue24d";   // insert_drive_file
constexpr const char* kIconDrive = u8"\ue1db";  // storage
} // namespace

FolderTreeDock::FolderTreeDock() = default;

void FolderTreeDock::setConnection(SshConnection* connection) {
    m_connection = connection;
}

void FolderTreeDock::setTerminalDock(TerminalDock* terminalDock) {
    m_terminalDock = terminalDock;
}

void FolderTreeDock::onConnected() {
    refreshRoots();
}

void FolderTreeDock::onDisconnected() {
    m_roots.clear();
    m_platform = RemotePlatform::Unknown;
}

void FolderTreeDock::render() {
    const Localization& loc = getLocalization(m_language);

    if (!m_connection || !m_connection->isConnected()) {
        ImGui::TextUnformatted(loc.termPleaseConnect);
        return;
    }

    if (m_roots.empty()) {
        refreshRoots();
    }

    std::string activePath;
    if (m_terminalDock) {
        activePath = m_terminalDock->activePath();
    }

    // toolbar (icon buttons)
    const ImVec4 toggleOnColor = ImVec4(0.2f, 0.6f, 0.2f, 1.0f);
    const ImVec4 toggleOnHover = ImVec4(0.25f, 0.7f, 0.25f, 1.0f);
    const ImVec4 toggleOnActive = ImVec4(0.18f, 0.5f, 0.18f, 1.0f);

    const char* eyeOn = u8"\ue8f4";   // visibility
    const char* eyeOff = u8"\ue8f5";  // visibility_off

    ImGui::PushID("hidden_dirs");
    bool wasDirsOn = m_showHiddenDirs;
    if (wasDirsOn) {
        ImGui::PushStyleColor(ImGuiCol_Button, toggleOnColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, toggleOnHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, toggleOnActive);
    }
    std::string dirLabel = std::string(kIconFolder) + " " + (m_showHiddenDirs ? eyeOn : eyeOff);
    if (ImGui::Button(dirLabel.c_str(), ImVec2(0, 0))) {
        m_showHiddenDirs = !m_showHiddenDirs;
        for (auto& root : m_roots) root->loaded = false;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(m_showHiddenDirs ? "Hidden folders: ON" : "Hidden folders: OFF");
    }
    if (wasDirsOn) {
        ImGui::PopStyleColor(3);
    }
    ImGui::PopID();

    ImGui::SameLine();

    ImGui::PushID("hidden_files");
    bool wasFilesOn = m_showHiddenFiles;
    if (wasFilesOn) {
        ImGui::PushStyleColor(ImGuiCol_Button, toggleOnColor);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, toggleOnHover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, toggleOnActive);
    }
    std::string fileLabel = std::string(kIconFile) + " " + (m_showHiddenFiles ? eyeOn : eyeOff);
    if (ImGui::Button(fileLabel.c_str(), ImVec2(0, 0))) {
        m_showHiddenFiles = !m_showHiddenFiles;
        for (auto& root : m_roots) root->loaded = false;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(m_showHiddenFiles ? "Hidden files: ON" : "Hidden files: OFF");
    }
    if (wasFilesOn) {
        ImGui::PopStyleColor(3);
    }
    ImGui::PopID();

    ImGui::SameLine();

    const char* iconLocate = u8"\ue55c"; // my_location
    ImGui::PushID("focus_current");
    if (ImGui::Button(iconLocate)) {
        if (!activePath.empty()) {
            m_focusPath = activePath;
            m_focusPending = true;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Focus current folder");
    }
    ImGui::PopID();

    ImGui::Separator();

    // アップロード進行状況表示
    if (m_uploading) {
        ImGui::Separator();
        ImGui::TextUnformatted(u8"\ue2c6");  // upload アイコン
        ImGui::SameLine();
        if (!m_uploadCurrentFile.empty()) {
            ImGui::Text("%s", m_uploadCurrentFile.c_str());
        }
        ImGui::ProgressBar(m_uploadProgress, ImVec2(-1, 0));
        ImGui::Separator();
    }

    // ダウンロード進行状況表示
    if (m_downloading) {
        ImGui::Separator();
        ImGui::TextUnformatted(u8"\ue2c4");  // download アイコン
        ImGui::SameLine();
        if (!m_downloadCurrentFile.empty()) {
            ImGui::Text("%s", m_downloadCurrentFile.c_str());
        }
        ImGui::ProgressBar(m_downloadProgress, ImVec2(-1, 0));
        ImGui::Separator();
    }

    // ドロップターゲットをリセット（次のフレームで更新される）
    m_dropTargetPath.clear();

    ImGui::BeginChild("##folderTree", ImVec2(0, 0), ImGuiChildFlags_None);

    const std::string focusPath = m_focusPending ? m_focusPath : std::string();

    for (auto& root : m_roots) {
        renderNode(*root, activePath, focusPath);
    }

    // ツリー領域全体がドロップターゲット（ルートディレクトリにアップロード）
    if (ImGui::BeginDragDropTarget()) {
        if (ImGui::AcceptDragDropPayload("PBTERM_EXTERNAL_FILES")) {
            // ホームディレクトリにアップロード
            m_dropTargetPath.clear();
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndChild();

    renderNewFolderDialog();
    renderDeleteDialog();
    renderDownloadDialog();
}

void FolderTreeDock::refreshRoots() {
    m_roots.clear();

    std::string uname = trim(exec("uname -s"));
    if (!uname.empty()) {
        std::string lower = uname;
        std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return std::tolower(c); });
        if (lower.find("linux") != std::string::npos ||
            lower.find("darwin") != std::string::npos ||
            lower.find("bsd") != std::string::npos) {
            m_platform = RemotePlatform::Unix;
        }
    }

    if (m_platform == RemotePlatform::Unknown) {
        m_platform = RemotePlatform::Windows;
    }

    if (m_platform == RemotePlatform::Unix) {
        auto root = std::make_unique<Node>();
        root->name = "/";
        root->path = "/";
        root->isDir = true;
        m_roots.push_back(std::move(root));
    } else {
        std::string drives = exec("powershell -NoProfile -Command \"Get-PSDrive -PSProvider FileSystem | ForEach-Object { $_.Root }\"");
        std::istringstream iss(drives);
        std::string line;
        while (std::getline(iss, line)) {
            line = trim(line);
            if (line.empty()) continue;
            auto root = std::make_unique<Node>();
            root->name = line;
            root->path = line;
            root->isDir = true;
            m_roots.push_back(std::move(root));
        }

        if (m_roots.empty()) {
            std::string systemDrive = trim(exec("cmd /c echo %SystemDrive%"));
            if (!systemDrive.empty()) {
                if (systemDrive.back() != '\\') {
                    systemDrive.push_back('\\');
                }
                auto root = std::make_unique<Node>();
                root->name = systemDrive;
                root->path = systemDrive;
                root->isDir = true;
                m_roots.push_back(std::move(root));
            }
        }
    }
}

void FolderTreeDock::loadChildren(Node& node) {
    if (!node.isDir || node.loaded) return;
    node.children = listDirectory(node.path);
    node.loaded = true;
}

std::vector<std::unique_ptr<FolderTreeDock::Node>> FolderTreeDock::listDirectory(const std::string& path) {
    std::vector<std::unique_ptr<Node>> out;

    if (m_platform == RemotePlatform::Unix) {
        std::string cmd = "cd " + shellQuote(path) + " && ls -a1p";
        std::string result = exec(cmd);
        std::istringstream iss(result);
        std::string line;
        while (std::getline(iss, line)) {
            line = trim(line);
            if (line.empty() || line == "." || line == "..") continue;

            bool isDir = false;
            if (!line.empty() && line.back() == '/') {
                isDir = true;
                line.pop_back();
            }

            bool isHidden = (!line.empty() && line[0] == '.');
            if (isHidden && ((isDir && !m_showHiddenDirs) || (!isDir && !m_showHiddenFiles))) {
                continue;
            }

            auto node = std::make_unique<Node>();
            node->name = line;
            node->path = joinPathUnix(path, line);
            node->isDir = isDir;
            out.push_back(std::move(node));
        }
    } else {
        std::string cmd =
            "powershell -NoProfile -Command \"Get-ChildItem -Force -LiteralPath " +
            psQuote(path) +
            " | ForEach-Object { $t = if ($_.PSIsContainer) { 'D' } else { 'F' }; $h = if ($_.Attributes -band 'Hidden') { 'H' } else { 'N' }; $t + ' ' + $h + ' ' + $_.Name }\"";
        std::string result = exec(cmd);
        std::istringstream iss(result);
        std::string line;
        while (std::getline(iss, line)) {
            line = trim(line);
            if (line.size() < 5) continue;
            char type = line[0];
            char hidden = line[2];
            std::string name = line.substr(4);
            if (name == "." || name == "..") continue;

            bool isDir = (type == 'D');
            bool isHidden = (hidden == 'H');
            if (isHidden && ((isDir && !m_showHiddenDirs) || (!isDir && !m_showHiddenFiles))) {
                continue;
            }

            auto node = std::make_unique<Node>();
            node->name = name;
            node->path = joinPathWindows(path, name);
            node->isDir = isDir;
            out.push_back(std::move(node));
        }
    }

    std::sort(out.begin(), out.end(), [](const std::unique_ptr<Node>& a, const std::unique_ptr<Node>& b) {
        if (a->isDir != b->isDir) {
            return a->isDir && !b->isDir;
        }
        std::string an = a->name;
        std::string bn = b->name;
        std::transform(an.begin(), an.end(), an.begin(), [](unsigned char c){ return std::tolower(c); });
        std::transform(bn.begin(), bn.end(), bn.begin(), [](unsigned char c){ return std::tolower(c); });
        return an < bn;
    });

    return out;
}

void FolderTreeDock::renderNode(Node& node, const std::string& activePath, const std::string& focusPath) {
    const Localization& loc = getLocalization(m_language);

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanFullWidth;
    if (!node.isDir) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    } else {
        flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
    }

    if (!activePath.empty() && isPathMatch(node.path, activePath)) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    if (!focusPath.empty() && node.isDir && isAncestorPath(node.path, focusPath)) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }

    const char* icon = node.isDir ? kIconFolder : kIconFile;
    ImVec4 iconColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);

    if (node.path.size() <= 3 && node.isDir) {
        icon = kIconDrive;
        iconColor = ImVec4(0.35f, 0.75f, 0.95f, 1.0f);
    } else if (node.isDir) {
        iconColor = ImVec4(0.95f, 0.75f, 0.25f, 1.0f);
    }

    ImGui::PushID(node.path.c_str());
    bool isRoot = false;
    if (m_platform == RemotePlatform::Unix) {
        isRoot = (node.path == "/");
    } else if (m_platform == RemotePlatform::Windows) {
        isRoot = (node.path.size() <= 3 && node.path.find(':') != std::string::npos);
    }

    bool open = ImGui::TreeNodeEx("##node",
                                  flags | ImGuiTreeNodeFlags_AllowOverlap,
                                  "");

    if (ImGui::BeginPopupContextItem("##context")) {
        if (node.isDir && ImGui::MenuItem(loc.folderNew)) {
            openNewFolderDialog(node.path);
        }
        if (!isRoot && ImGui::MenuItem(loc.folderDelete)) {
            openDeleteDialog(node.path, node.isDir);
        }
        if (!isRoot && ImGui::MenuItem(loc.folderDownload)) {
            downloadToLocal(node.path, node.isDir);
        }
        ImGui::EndPopup();
    }

    if (node.isDir) {
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("PBTERM_PATH", node.path.c_str(), node.path.size() + 1);
            ImGui::TextUnformatted(node.path.c_str());
            ImGui::EndDragDropSource();
        }

        // 外部ドロップターゲット: フォルダはドロップを受け入れられる
        if (ImGui::BeginDragDropTarget()) {
            if (ImGui::AcceptDragDropPayload("PBTERM_EXTERNAL_FILES")) {
                // 外部ファイルドロップを処理
                m_uploadDestination = node.path;
            }
            ImGui::EndDragDropTarget();
        }

        // ホバー時にドロップターゲットとしてマーク＆ハイライト
        if (ImGui::IsItemHovered()) {
            m_dropTargetPath = node.path;

            // ドロップターゲットの背景をハイライト
            ImVec2 itemMin = ImGui::GetItemRectMin();
            ImVec2 itemMax = ImGui::GetItemRectMax();
            // 幅を広げてフォルダ名全体をカバー
            itemMax.x = itemMin.x + ImGui::GetContentRegionAvail().x + 50.0f;

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec4 highlightColor = ImVec4(0.3f, 0.6f, 0.9f, 0.35f);
            drawList->AddRectFilled(
                itemMin,
                itemMax,
                ImGui::ColorConvertFloat4ToU32(highlightColor),
                3.0f
            );
            // 枠線
            drawList->AddRect(
                itemMin,
                itemMax,
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.4f, 0.7f, 1.0f, 0.9f)),
                3.0f,
                0,
                2.0f
            );
        }
    }

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
    ImGui::TextUnformatted(icon);
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextUnformatted(node.name.c_str());

    if (!focusPath.empty() && isPathMatch(node.path, focusPath)) {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        float centerY = (min.y + max.y) * 0.5f;
        ImGui::SetScrollFromPosY(centerY, 0.5f);
        m_focusPending = false;
    }

    if (node.isDir && open) {
        if (!node.loaded) {
            loadChildren(node);
        }
        for (auto& child : node.children) {
            renderNode(*child, activePath, focusPath);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void FolderTreeDock::openNewFolderDialog(const std::string& parentPath) {
    m_showNewFolder = true;
    m_newFolderParent = parentPath;
    std::memset(m_newFolderName, 0, sizeof(m_newFolderName));
}

void FolderTreeDock::openDeleteDialog(const std::string& targetPath, bool isDir) {
    m_showDelete = true;
    m_deleteTarget = targetPath;
    m_deleteIsDir = isDir;
}

void FolderTreeDock::renderNewFolderDialog() {
    if (!m_showNewFolder) return;

    const Localization& loc = getLocalization(m_language);
    ImGui::OpenPopup(loc.folderNewTitle);

    if (ImGui::BeginPopupModal(loc.folderNewTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", loc.folderNewName);
        ImGui::SetNextItemWidth(240);
        ImGui::InputText("##newFolderName", m_newFolderName, sizeof(m_newFolderName));

        ImGui::Spacing();

        if (ImGui::Button(loc.folderCreate, ImVec2(100, 0))) {
            if (m_newFolderName[0] != '\0') {
                std::string newPath;
                if (m_platform == RemotePlatform::Unix) {
                    newPath = joinPathUnix(m_newFolderParent, m_newFolderName);
                    exec("mkdir -p " + shellQuote(newPath));
                } else {
                    newPath = joinPathWindows(m_newFolderParent, m_newFolderName);
                    exec("powershell -NoProfile -Command \"New-Item -ItemType Directory -Force -LiteralPath " + psQuote(newPath) + "\"");
                }
                refreshNodeByPath(m_newFolderParent);
            }
            m_showNewFolder = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.folderCancel, ImVec2(100, 0))) {
            m_showNewFolder = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void FolderTreeDock::renderDeleteDialog() {
    if (!m_showDelete) return;

    const Localization& loc = getLocalization(m_language);
    ImGui::OpenPopup(loc.folderDeleteTitle);

    if (ImGui::BeginPopupModal(loc.folderDeleteTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        char msg[256];
        std::snprintf(msg, sizeof(msg), loc.folderDeleteMessage, m_deleteTarget.c_str());
        ImGui::TextUnformatted(msg);
        ImGui::Spacing();

        if (ImGui::Button(loc.folderDeleteYes, ImVec2(100, 0))) {
            if (m_platform == RemotePlatform::Unix) {
                if (m_deleteIsDir) {
                    exec("rm -rf " + shellQuote(m_deleteTarget));
                } else {
                    exec("rm -f " + shellQuote(m_deleteTarget));
                }
            } else {
                exec("powershell -NoProfile -Command \"Remove-Item -Recurse -Force -LiteralPath " + psQuote(m_deleteTarget) + "\"");
            }

            std::string parent = m_deleteTarget;
            size_t pos = parent.find_last_of(m_platform == RemotePlatform::Unix ? '/' : '\\');
            if (pos != std::string::npos) {
                parent = parent.substr(0, pos);
                if (parent.empty() && m_platform == RemotePlatform::Unix) parent = "/";
                if (!parent.empty()) {
                    refreshNodeByPath(parent);
                }
            }

            m_showDelete = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.folderDeleteNo, ImVec2(100, 0))) {
            m_showDelete = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void FolderTreeDock::refreshNodeByPath(const std::string& path) {
    for (auto& root : m_roots) {
        if (path == root->path || path.find(root->path) == 0) {
            root->loaded = false;
        }
    }
}

bool FolderTreeDock::isPathMatch(const std::string& path, const std::string& activePath) const {
    if (path.empty() || activePath.empty()) return false;
    if (m_platform == RemotePlatform::Windows) {
        if (path.size() != activePath.size()) return false;
        for (size_t i = 0; i < path.size(); ++i) {
            if (std::tolower(path[i]) != std::tolower(activePath[i])) return false;
        }
        return true;
    }
    return path == activePath;
}

bool FolderTreeDock::isAncestorPath(const std::string& path, const std::string& targetPath) const {
    if (path.empty() || targetPath.empty()) return false;
    if (m_platform == RemotePlatform::Windows) {
        std::string p = path;
        std::string t = targetPath;
        std::transform(p.begin(), p.end(), p.begin(), [](unsigned char c){ return std::tolower(c); });
        std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c){ return std::tolower(c); });
        if (t.size() < p.size()) return false;
        if (t.rfind(p, 0) != 0) return false;
        if (t.size() == p.size()) return true;
        char sep = '\\';
        if (p.back() != sep) return t[p.size()] == sep;
        return true;
    }

    if (targetPath.size() < path.size()) return false;
    if (targetPath.rfind(path, 0) != 0) return false;
    if (targetPath.size() == path.size()) return true;
    if (path.back() == '/') return true;
    return targetPath[path.size()] == '/';
}

std::string FolderTreeDock::exec(const std::string& cmd, int timeoutMs) const {
    if (!m_connection) return "";
    return m_connection->exec(cmd, timeoutMs);
}

std::string FolderTreeDock::trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) start++;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) end--;
    return s.substr(start, end - start);
}

std::string FolderTreeDock::shellQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out += c;
        }
    }
    out += "'";
    return out;
}

std::string FolderTreeDock::psQuote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') {
            out += "''";
        } else {
            out += c;
        }
    }
    out += "'";
    return out;
}

std::string FolderTreeDock::joinPathUnix(const std::string& base, const std::string& name) {
    if (base == "/") return "/" + name;
    if (!base.empty() && base.back() == '/') return base + name;
    return base + "/" + name;
}

std::string FolderTreeDock::joinPathWindows(const std::string& base, const std::string& name) {
    if (base.empty()) return name;
    if (base.back() == '\\') return base + name;
    return base + "\\" + name;
}

void FolderTreeDock::renderDownloadDialog() {
    if (!m_showDownloadComplete) return;

    const Localization& loc = getLocalization(m_language);
    ImGui::OpenPopup(loc.folderDownloadComplete);

    if (ImGui::BeginPopupModal(loc.folderDownloadComplete, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", m_downloadedPath.c_str());
        ImGui::Spacing();

        if (ImGui::Button(loc.folderOpenInFinder, ImVec2(150, 0))) {
            // Finderで開く（macOS）
            std::string cmd;
#ifdef __APPLE__
            cmd = "open -R \"" + m_downloadedPath + "\"";
#elif _WIN32
            cmd = "explorer /select,\"" + m_downloadedPath + "\"";
#else
            cmd = "xdg-open \"" + std::filesystem::path(m_downloadedPath).parent_path().string() + "\"";
#endif
            std::system(cmd.c_str());
            m_showDownloadComplete = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.dlgOK, ImVec2(80, 0))) {
            m_showDownloadComplete = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void FolderTreeDock::downloadToLocal(const std::string& remotePath, bool isDir) {
    if (!m_connection || !m_connection->isConnected()) {
        return;
    }

    namespace fs = std::filesystem;

    // ダウンロード先を決定（ユーザーのダウンロードフォルダ）
    std::string downloadDir;
#ifdef __APPLE__
    const char* home = std::getenv("HOME");
    if (home) {
        downloadDir = std::string(home) + "/Downloads";
    }
#elif _WIN32
    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile) {
        downloadDir = std::string(userProfile) + "\\Downloads";
    }
#else
    const char* home = std::getenv("HOME");
    if (home) {
        downloadDir = std::string(home) + "/Downloads";
    }
#endif

    if (downloadDir.empty() || !fs::exists(downloadDir)) {
        downloadDir = fs::temp_directory_path().string();
    }

    // ファイル名を取得
    std::string filename;
    if (m_platform == RemotePlatform::Unix) {
        size_t pos = remotePath.rfind('/');
        filename = (pos != std::string::npos) ? remotePath.substr(pos + 1) : remotePath;
    } else {
        size_t pos = remotePath.rfind('\\');
        filename = (pos != std::string::npos) ? remotePath.substr(pos + 1) : remotePath;
    }

    std::string localPath = downloadDir + "/" + filename;

    std::cout << "ダウンロード開始: " << remotePath << " -> " << localPath << std::endl;

    m_downloading = true;
    m_downloadProgress = 0.0f;
    m_downloadCurrentFile = filename;

    // 別スレッドでダウンロードを実行
    std::thread downloadThread([this, remotePath, localPath, isDir]() {
        bool success = false;

        if (isDir) {
            success = m_connection->downloadDirectory(remotePath, localPath);
        } else {
            success = m_connection->downloadFile(remotePath, localPath);
        }

        m_downloading = false;
        m_downloadProgress = 1.0f;
        m_downloadCurrentFile.clear();

        if (success) {
            std::cout << "ダウンロード完了: " << localPath << std::endl;
            m_downloadedPath = localPath;
            m_showDownloadComplete = true;
        } else {
            std::cerr << "ダウンロード失敗: " << m_connection->lastError() << std::endl;
        }
    });

    downloadThread.detach();
}

void FolderTreeDock::handleExternalFileDrop(const std::vector<std::string>& paths) {
    if (!m_connection || !m_connection->isConnected()) {
        return;
    }

    if (paths.empty()) {
        return;
    }

    // ドロップターゲットが設定されていない場合は、ホームディレクトリを使用
    std::string destination = m_dropTargetPath;
    if (destination.empty()) {
        if (m_platform == RemotePlatform::Unix) {
            destination = exec("echo $HOME");
            destination = trim(destination);
            if (destination.empty()) {
                destination = "/tmp";
            }
        } else {
            destination = exec("echo %USERPROFILE%");
            destination = trim(destination);
        }
    }

    if (destination.empty()) {
        std::cerr << "アップロード先が不明です" << std::endl;
        return;
    }

    std::cout << "ファイルドロップ: " << paths.size() << " 件を " << destination << " にアップロード" << std::endl;

    namespace fs = std::filesystem;

    m_uploading = true;
    m_uploadProgress = 0.0f;

    // 別スレッドでアップロードを実行
    std::thread uploadThread([this, paths, destination]() {
        int completed = 0;
        int total = static_cast<int>(paths.size());

        for (const auto& localPath : paths) {
            fs::path p(localPath);
            std::string filename = p.filename().string();
            m_uploadCurrentFile = filename;

            std::string remotePath;
            if (m_platform == RemotePlatform::Unix) {
                remotePath = joinPathUnix(destination, filename);
            } else {
                remotePath = joinPathWindows(destination, filename);
            }

            std::cout << "アップロード中: " << localPath << " -> " << remotePath << std::endl;

            bool success = false;
            if (fs::is_directory(localPath)) {
                success = m_connection->uploadDirectory(localPath, remotePath);
            } else {
                success = m_connection->uploadFile(localPath, remotePath);
            }

            if (success) {
                std::cout << "完了: " << filename << std::endl;
            } else {
                std::cerr << "失敗: " << filename << " - " << m_connection->lastError() << std::endl;
            }

            completed++;
            m_uploadProgress = static_cast<float>(completed) / static_cast<float>(total);
        }

        m_uploading = false;
        m_uploadCurrentFile.clear();
        m_uploadProgress = 1.0f;

        // ツリーを更新
        for (auto& root : m_roots) {
            root->loaded = false;
        }
    });

    uploadThread.detach();
}

} // namespace pbterm
