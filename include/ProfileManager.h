#pragma once

#include <string>
#include <vector>
#include "SshConnection.h"

namespace pbterm {

// 接続プロファイル
struct Profile {
    std::string name;
    SshConfig config;
};

// プロファイル管理
class ProfileManager {
public:
    ProfileManager();
    ~ProfileManager() = default;

    // プロファイル操作
    void addProfile(const std::string& name, const SshConfig& config);
    void removeProfile(int index);
    void updateProfile(int index, const SshConfig& config);
    bool renameProfile(const std::string& oldName, const std::string& newName);

    // プロファイル取得
    const std::vector<Profile>& profiles() const { return m_profiles; }
    const Profile* getProfile(int index) const;
    const Profile* getProfile(const std::string& name) const;
    int profileCount() const { return static_cast<int>(m_profiles.size()); }

    // オート接続プロファイル
    const Profile* autoConnectProfile() const;
    const std::string& autoConnectProfileName() const { return m_autoConnectName; }
    void setAutoConnectProfile(const std::string& name);
    void clearAutoConnectFromProfile(const std::string& name);  // 特定プロファイルの自動接続をクリア

    // 保存/読み込み
    void save();
    void load();

private:
    std::string configPath() const;

    std::vector<Profile> m_profiles;
    std::string m_autoConnectName;
};

} // namespace pbterm
