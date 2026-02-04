#pragma once

#include <memory>
#include <string>
#include <vector>

namespace pbterm {

class SshConnection;
class TerminalDock;

class FolderTreeDock {
public:
    FolderTreeDock();

    void setConnection(SshConnection* connection);
    void setTerminalDock(TerminalDock* terminalDock);
    void setLanguage(int language) { m_language = language; }

    void onConnected();
    void onDisconnected();

    void render();

private:
    struct Node {
        std::string name;
        std::string path;
        bool isDir = false;
        bool loaded = false;
        std::vector<std::unique_ptr<Node>> children;
    };

    enum class RemotePlatform {
        Unknown,
        Unix,
        Windows
    };

    void refreshRoots();
    void loadChildren(Node& node);
    std::vector<std::unique_ptr<Node>> listDirectory(const std::string& path);

    std::string exec(const std::string& cmd, int timeoutMs = 2000) const;
    static std::string trim(const std::string& s);
    static std::string shellQuote(const std::string& s);
    static std::string psQuote(const std::string& s);
    static std::string joinPathUnix(const std::string& base, const std::string& name);
    static std::string joinPathWindows(const std::string& base, const std::string& name);

    void renderNode(Node& node, const std::string& activePath, const std::string& focusPath);
    void openNewFolderDialog(const std::string& parentPath);
    void openDeleteDialog(const std::string& targetPath, bool isDir);
    void renderNewFolderDialog();
    void renderDeleteDialog();
    void refreshNodeByPath(const std::string& path);

    bool isPathMatch(const std::string& path, const std::string& activePath) const;
    bool isAncestorPath(const std::string& path, const std::string& targetPath) const;

    SshConnection* m_connection = nullptr;
    TerminalDock* m_terminalDock = nullptr;
    int m_language = 0;

    RemotePlatform m_platform = RemotePlatform::Unknown;
    std::vector<std::unique_ptr<Node>> m_roots;

    bool m_showHiddenDirs = false;
    bool m_showHiddenFiles = false;
    bool m_focusPending = false;
    std::string m_focusPath;

    // dialogs
    bool m_showNewFolder = false;
    bool m_showDelete = false;
    std::string m_newFolderParent;
    std::string m_deleteTarget;
    bool m_deleteIsDir = false;
    char m_newFolderName[256] = {0};
};

} // namespace pbterm
