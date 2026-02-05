#include "ProfileManager.h"
#include <fstream>
#include <filesystem>
#include <iostream>

namespace pbterm {

ProfileManager::ProfileManager() {
    // 設定ディレクトリ作成
    std::string path = configPath();
    std::filesystem::path dir = std::filesystem::path(path).parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
}

void ProfileManager::addProfile(const std::string& name, const SshConfig& config) {
    // 既存のプロファイルを更新または追加
    for (auto& p : m_profiles) {
        if (p.name == name) {
            p.config = config;
            return;
        }
    }

    Profile profile;
    profile.name = name;
    profile.config = config;
    m_profiles.push_back(profile);
}

void ProfileManager::removeProfile(int index) {
    if (index >= 0 && index < static_cast<int>(m_profiles.size())) {
        // オート接続プロファイルの場合はクリア
        if (m_profiles[index].name == m_autoConnectName) {
            m_autoConnectName.clear();
        }
        m_profiles.erase(m_profiles.begin() + index);
    }
}

void ProfileManager::updateProfile(int index, const SshConfig& config) {
    if (index >= 0 && index < static_cast<int>(m_profiles.size())) {
        m_profiles[index].config = config;
    }
}

bool ProfileManager::renameProfile(const std::string& oldName, const std::string& newName) {
    if (oldName.empty() || newName.empty() || oldName == newName) {
        return false;
    }
    if (getProfile(newName)) {
        return false;
    }
    for (auto& p : m_profiles) {
        if (p.name == oldName) {
            p.name = newName;
            if (m_autoConnectName == oldName) {
                m_autoConnectName = newName;
            }
            return true;
        }
    }
    return false;
}

const Profile* ProfileManager::getProfile(int index) const {
    if (index >= 0 && index < static_cast<int>(m_profiles.size())) {
        return &m_profiles[index];
    }
    return nullptr;
}

const Profile* ProfileManager::getProfile(const std::string& name) const {
    for (const auto& p : m_profiles) {
        if (p.name == name) {
            return &p;
        }
    }
    return nullptr;
}

const Profile* ProfileManager::autoConnectProfile() const {
    if (m_autoConnectName.empty()) {
        return nullptr;
    }
    return getProfile(m_autoConnectName);
}

void ProfileManager::setAutoConnectProfile(const std::string& name) {
    // 以前の自動接続プロファイルのフラグをクリア
    for (auto& p : m_profiles) {
        if (p.config.autoConnect && p.name != name) {
            p.config.autoConnect = false;
        }
    }
    // 新しいプロファイルの自動接続フラグを設定
    for (auto& p : m_profiles) {
        if (p.name == name) {
            p.config.autoConnect = true;
        }
    }
    m_autoConnectName = name;
}

void ProfileManager::clearAutoConnectFromProfile(const std::string& name) {
    for (auto& p : m_profiles) {
        if (p.name == name) {
            p.config.autoConnect = false;
        }
    }
    if (m_autoConnectName == name) {
        m_autoConnectName.clear();
    }
}

void ProfileManager::save() {
    std::string path = configPath();
    std::ofstream file(path);

    if (!file.is_open()) {
        std::cerr << "設定ファイルを開けません: " << path << std::endl;
        return;
    }

    // シンプルなテキスト形式で保存
    file << "auto_connect=" << m_autoConnectName << "\n";
    file << "profile_count=" << m_profiles.size() << "\n";

    for (const auto& p : m_profiles) {
        file << "[profile]\n";
        file << "name=" << p.name << "\n";
        file << "host=" << p.config.host << "\n";
        file << "port=" << p.config.port << "\n";
        file << "username=" << p.config.username << "\n";
        file << "password=" << p.config.password << "\n";
        file << "private_key=" << p.config.privateKeyPath << "\n";
        file << "use_key_auth=" << (p.config.useKeyAuth ? "1" : "0") << "\n";
        file << "auto_connect=" << (p.config.autoConnect ? "1" : "0") << "\n";
    }

    std::cout << "設定保存: " << path << std::endl;
}

void ProfileManager::load() {
    std::string path = configPath();
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cout << "設定ファイルなし: " << path << std::endl;
        return;
    }

    m_profiles.clear();
    m_autoConnectName.clear();

    std::string line;
    Profile currentProfile;
    bool inProfile = false;

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

        if (line == "[profile]") {
            if (inProfile && !currentProfile.name.empty()) {
                m_profiles.push_back(currentProfile);
            }
            currentProfile = Profile();
            inProfile = true;
            continue;
        }

        std::string val;

        val = getValue(line, "auto_connect");
        if (!val.empty() && !inProfile) {
            m_autoConnectName = val;
            continue;
        }

        if (inProfile) {
            val = getValue(line, "name");
            if (!val.empty()) { currentProfile.name = val; continue; }

            val = getValue(line, "host");
            if (!val.empty()) { currentProfile.config.host = val; continue; }

            val = getValue(line, "port");
            if (!val.empty()) { currentProfile.config.port = std::stoi(val); continue; }

            val = getValue(line, "username");
            if (!val.empty()) { currentProfile.config.username = val; continue; }

            val = getValue(line, "password");
            if (!val.empty()) { currentProfile.config.password = val; continue; }

            val = getValue(line, "private_key");
            if (!val.empty()) { currentProfile.config.privateKeyPath = val; continue; }

            val = getValue(line, "use_key_auth");
            if (!val.empty()) { currentProfile.config.useKeyAuth = (val == "1"); continue; }

            val = getValue(line, "auto_connect");
            if (!val.empty()) { currentProfile.config.autoConnect = (val == "1"); continue; }
        }
    }

    // 最後のプロファイル
    if (inProfile && !currentProfile.name.empty()) {
        m_profiles.push_back(currentProfile);
    }

    std::cout << "設定読み込み: " << m_profiles.size() << " プロファイル" << std::endl;
}

std::string ProfileManager::configPath() const {
    const char* home = std::getenv("HOME");
    if (!home) {
        home = ".";
    }
    return std::string(home) + "/.config/pbterm-imgui/profiles.conf";
}

} // namespace pbterm
