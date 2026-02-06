#include "ConnectionDialog.h"
#include "ProfileManager.h"
#include "SettingsDialog.h"
#include "imgui.h"
#include <cstring>

namespace pbterm {

ConnectionDialog::ConnectionDialog(ProfileManager* profileManager)
    : m_profileManager(profileManager)
{
    std::strcpy(m_hostBuf, "");
    std::strcpy(m_userBuf, "");
    std::strcpy(m_passBuf, "");
    std::strcpy(m_keyPathBuf, "~/.ssh/id_rsa");
}

void ConnectionDialog::initForShow() {
    // 自動接続が設定されているプロファイルを探して選択
    const auto& profiles = m_profileManager->profiles();
    const std::string& autoConnectName = m_profileManager->autoConnectProfileName();

    m_selectedProfile = 0;  // デフォルトは新規

    if (!autoConnectName.empty()) {
        for (size_t i = 0; i < profiles.size(); ++i) {
            if (profiles[i].name == autoConnectName) {
                m_selectedProfile = static_cast<int>(i) + 1;
                break;
            }
        }
    }

    // 選択されたプロファイルの情報を読み込み
    selectProfile(m_selectedProfile);
}

void ConnectionDialog::selectProfile(int index) {
    m_selectedProfile = index;
    const auto& profiles = m_profileManager->profiles();

    if (m_selectedProfile > 0 && m_selectedProfile <= static_cast<int>(profiles.size())) {
        const Profile& p = profiles[m_selectedProfile - 1];
        std::strncpy(m_hostBuf, p.config.host.c_str(), sizeof(m_hostBuf) - 1);
        m_hostBuf[sizeof(m_hostBuf) - 1] = '\0';
        m_port = p.config.port;
        std::strncpy(m_userBuf, p.config.username.c_str(), sizeof(m_userBuf) - 1);
        m_userBuf[sizeof(m_userBuf) - 1] = '\0';
        std::strncpy(m_passBuf, p.config.password.c_str(), sizeof(m_passBuf) - 1);
        m_passBuf[sizeof(m_passBuf) - 1] = '\0';
        std::strncpy(m_keyPathBuf, p.config.privateKeyPath.c_str(), sizeof(m_keyPathBuf) - 1);
        m_keyPathBuf[sizeof(m_keyPathBuf) - 1] = '\0';
        m_useKeyAuth = p.config.useKeyAuth;
        m_autoConnect = p.config.autoConnect;
    } else {
        // 新規プロファイル: デフォルト値を設定
        std::strcpy(m_hostBuf, "");
        m_port = 22;
        std::strcpy(m_userBuf, "");
        std::strcpy(m_passBuf, "");
        std::strcpy(m_keyPathBuf, "~/.ssh/id_rsa");
        m_useKeyAuth = true;
        m_autoConnect = false;
    }
}

void ConnectionDialog::render(bool* open) {
    if (!*open) return;

    const Localization& loc = getLocalization(m_language);

    ImGui::SetNextWindowSize(ImVec2(450, 350), ImGuiCond_FirstUseEver);

    // ドッキング不可
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

    if (ImGui::Begin(loc.dlgConnectionTitle, open, flags)) {
        renderProfileSelector();
        ImGui::Separator();
        renderConnectionSettings();
        ImGui::Separator();
        renderButtons(open);
    }
    ImGui::End();

    // Profile save dialog
    if (m_showSaveDialog) {
        ImGui::OpenPopup(loc.dlgSaveProfile);
        m_showSaveDialog = false;
    }

    if (ImGui::BeginPopupModal(loc.dlgSaveProfile, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", loc.dlgProfileName);
        ImGui::InputText("##profilename", m_newProfileName, sizeof(m_newProfileName));

        if (ImGui::Button(loc.dlgSave, ImVec2(100, 0))) {
            if (strlen(m_newProfileName) > 0) {
                m_config.host = m_hostBuf;
                m_config.port = m_port;
                m_config.username = m_userBuf;
                m_config.password = m_passBuf;
                m_config.privateKeyPath = m_keyPathBuf;
                m_config.useKeyAuth = m_useKeyAuth;
                m_config.autoConnect = m_autoConnect;

                m_profileManager->addProfile(m_newProfileName, m_config);

                // 自動接続が設定されていれば、このプロファイルを自動接続に設定
                if (m_autoConnect) {
                    m_profileManager->setAutoConnectProfile(m_newProfileName);
                }

                m_profileManager->save();

                // 新しく保存したプロファイルを選択状態にする
                const auto& profiles = m_profileManager->profiles();
                for (size_t i = 0; i < profiles.size(); ++i) {
                    if (profiles[i].name == m_newProfileName) {
                        m_selectedProfile = static_cast<int>(i) + 1;
                        break;
                    }
                }

                std::memset(m_newProfileName, 0, sizeof(m_newProfileName));
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.dlgCancel, ImVec2(100, 0))) {
            std::memset(m_newProfileName, 0, sizeof(m_newProfileName));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // Profile rename dialog
    if (m_showRenameDialog) {
        ImGui::OpenPopup(loc.dlgRenameProfile);
        m_showRenameDialog = false;
    }

    if (ImGui::BeginPopupModal(loc.dlgRenameProfile, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", loc.dlgRenameProfileName);
        ImGui::InputText("##renameprofilename", m_renameProfileName, sizeof(m_renameProfileName));

        if (ImGui::Button(loc.dlgSave, ImVec2(100, 0))) {
            if (strlen(m_renameProfileName) > 0) {
                const auto& profiles = m_profileManager->profiles();
                if (m_selectedProfile > 0 &&
                    m_selectedProfile <= static_cast<int>(profiles.size())) {
                    std::string oldName = profiles[m_selectedProfile - 1].name;
                    if (m_profileManager->renameProfile(oldName, m_renameProfileName)) {
                        m_profileManager->save();
                        // 再選択
                        const auto& newProfiles = m_profileManager->profiles();
                        for (size_t i = 0; i < newProfiles.size(); ++i) {
                            if (newProfiles[i].name == m_renameProfileName) {
                                m_selectedProfile = static_cast<int>(i) + 1;
                                break;
                            }
                        }
                    }
                }
                std::memset(m_renameProfileName, 0, sizeof(m_renameProfileName));
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.dlgCancel, ImVec2(100, 0))) {
            std::memset(m_renameProfileName, 0, sizeof(m_renameProfileName));
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    // 自動接続切り替え確認ダイアログ
    if (m_showAutoConnectConfirm) {
        ImGui::OpenPopup(loc.dlgAutoConnectConfirm);
        m_showAutoConnectConfirm = false;
    }

    if (ImGui::BeginPopupModal(loc.dlgAutoConnectConfirm, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        // メッセージをフォーマット
        char message[256];
        snprintf(message, sizeof(message), loc.dlgAutoConnectMessage, m_existingAutoConnectProfile.c_str());
        ImGui::Text("%s", message);

        ImGui::Spacing();

        float buttonWidth = 80.0f;
        float windowWidth = ImGui::GetWindowWidth();
        ImGui::SetCursorPosX((windowWidth - buttonWidth * 2 - 10) * 0.5f);

        if (ImGui::Button(loc.dlgYes, ImVec2(buttonWidth, 0))) {
            // 既存の自動接続をクリアして、現在のプロファイルを自動接続に設定
            m_profileManager->clearAutoConnectFromProfile(m_existingAutoConnectProfile);
            m_autoConnect = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.dlgNo, ImVec2(buttonWidth, 0))) {
            // キャンセル、何もしない
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ConnectionDialog::renderProfileSelector() {
    const Localization& loc = getLocalization(m_language);

    ImGui::Text("%s", loc.dlgProfile);

    const auto& profiles = m_profileManager->profiles();

    std::vector<const char*> items;
    items.push_back(loc.dlgNewProfile);
    for (const auto& p : profiles) {
        items.push_back(p.name.c_str());
    }

    int prevSelected = m_selectedProfile;
    if (ImGui::Combo("##profile", &m_selectedProfile, items.data(), static_cast<int>(items.size()))) {
        if (m_selectedProfile != prevSelected) {
            selectProfile(m_selectedProfile);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button(loc.dlgSave)) {
        if (m_selectedProfile > 0) {
            const auto& profiles = m_profileManager->profiles();
            if (m_selectedProfile <= static_cast<int>(profiles.size())) {
                m_config.host = m_hostBuf;
                m_config.port = m_port;
                m_config.username = m_userBuf;
                m_config.password = m_passBuf;
                m_config.privateKeyPath = m_keyPathBuf;
                m_config.useKeyAuth = m_useKeyAuth;
                m_config.autoConnect = m_autoConnect;

                m_profileManager->updateProfile(m_selectedProfile - 1, m_config);
                if (m_autoConnect) {
                    m_profileManager->setAutoConnectProfile(profiles[m_selectedProfile - 1].name);
                } else {
                    m_profileManager->clearAutoConnectFromProfile(profiles[m_selectedProfile - 1].name);
                }
                m_profileManager->save();
            }
        } else {
            m_showSaveDialog = true;
        }
    }

    if (m_selectedProfile > 0) {
        ImGui::SameLine();
        if (ImGui::Button(loc.dlgRename)) {
            const auto& profiles = m_profileManager->profiles();
            if (m_selectedProfile <= static_cast<int>(profiles.size())) {
                std::strncpy(m_renameProfileName, profiles[m_selectedProfile - 1].name.c_str(),
                             sizeof(m_renameProfileName) - 1);
                m_showRenameDialog = true;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.dlgDelete)) {
            m_profileManager->removeProfile(m_selectedProfile - 1);
            m_profileManager->save();
            m_selectedProfile = 0;
        }
    }
}

void ConnectionDialog::renderConnectionSettings() {
    const Localization& loc = getLocalization(m_language);

    ImGui::Text("SSH");
    ImGui::Spacing();

    // Host
    ImGui::Text("%s", loc.dlgHost);
    ImGui::SameLine(100);
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##host", m_hostBuf, sizeof(m_hostBuf));

    // Port
    ImGui::Text("%s", loc.dlgPort);
    ImGui::SameLine(100);
    ImGui::SetNextItemWidth(100);
    ImGui::InputInt("##port", &m_port);
    if (m_port < 1) m_port = 1;
    if (m_port > 65535) m_port = 65535;

    // Username
    ImGui::Text("%s", loc.dlgUsername);
    ImGui::SameLine(100);
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##user", m_userBuf, sizeof(m_userBuf));

    ImGui::Spacing();
    ImGui::Text("%s", loc.dlgAuthMethod);

    // Public key auth
    ImGui::Checkbox(loc.dlgPublicKey, &m_useKeyAuth);

    if (m_useKeyAuth) {
        ImGui::Text("%s", loc.dlgPrivateKey);
        ImGui::SameLine(100);
        ImGui::SetNextItemWidth(280);
        ImGui::InputText("##keypath", m_keyPathBuf, sizeof(m_keyPathBuf));
    }

    // Password
    ImGui::Text("%s", loc.dlgPassword);
    ImGui::SameLine(100);
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##password", m_passBuf, sizeof(m_passBuf), ImGuiInputTextFlags_Password);

    ImGui::Spacing();

    // Auto connect
    // 一時変数を使ってチェックボックスの状態を管理
    bool autoConnectPrev = m_autoConnect;
    if (ImGui::Checkbox(loc.dlgAutoConnect, &m_autoConnect)) {
        // チェックボックスがオンに変更された場合
        if (m_autoConnect && !autoConnectPrev) {
            // 他のプロファイルが既に自動接続を持っているか確認
            const std::string& existingAutoConnect = m_profileManager->autoConnectProfileName();

            // 現在選択中のプロファイル名を取得
            std::string currentProfileName;
            if (m_selectedProfile > 0 && m_selectedProfile <= static_cast<int>(m_profileManager->profiles().size())) {
                currentProfileName = m_profileManager->profiles()[m_selectedProfile - 1].name;
            }

            // 別のプロファイルが自動接続を持っている場合は確認ダイアログを表示
            if (!existingAutoConnect.empty() && existingAutoConnect != currentProfileName) {
                m_existingAutoConnectProfile = existingAutoConnect;
                m_showAutoConnectConfirm = true;
                m_autoConnect = false;  // 一旦オフに戻す（確認後に設定）
            }
        }
    }
}

void ConnectionDialog::renderButtons(bool* open) {
    const Localization& loc = getLocalization(m_language);

    ImGui::Spacing();

    float buttonWidth = 100.0f;
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((windowWidth - buttonWidth * 2 - 10) * 0.5f);

    if (ImGui::Button(loc.dlgConnect, ImVec2(buttonWidth, 0))) {
        m_config.host = m_hostBuf;
        m_config.port = m_port;
        m_config.username = m_userBuf;
        m_config.password = m_passBuf;
        m_config.privateKeyPath = m_keyPathBuf;
        m_config.useKeyAuth = m_useKeyAuth;
        m_config.autoConnect = m_autoConnect;

        if (m_config.isValid() && m_connectCallback) {
            if (m_autoConnect && m_selectedProfile == 0) {
                m_profileManager->addProfile("default", m_config);
            }

            if (m_autoConnect) {
                m_profileManager->setAutoConnectProfile(
                    m_selectedProfile > 0
                        ? m_profileManager->profiles()[m_selectedProfile - 1].name
                        : "default"
                );
            }

            // プロファイル名を取得（選択中のプロファイルまたはホスト名）
            std::string profileName;
            if (m_selectedProfile > 0 && m_selectedProfile <= static_cast<int>(m_profileManager->profiles().size())) {
                profileName = m_profileManager->profiles()[m_selectedProfile - 1].name;
            } else {
                profileName = m_config.host;  // 新規の場合はホスト名を使用
            }

            m_profileManager->save();
            m_connectCallback(m_config, profileName);
        }
    }

    ImGui::SameLine();

    if (ImGui::Button(loc.dlgCancel, ImVec2(buttonWidth, 0))) {
        *open = false;
    }
}

} // namespace pbterm
