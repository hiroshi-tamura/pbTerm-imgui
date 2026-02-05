#include "SettingsDialog.h"
#include "imgui.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstdlib>

namespace pbterm {

// 英語ローカライゼーション
static const Localization englishLocalization = {
    // メニュー
    "Connect",
    "Start",
    "Stop",
    "Setting...",
    "View",
    "Terminal",
    "Commands",
    "Folders",
    "Option",

    // 接続状態
    "Connected",
    "Disconnected",

    // 接続ダイアログ
    "Connection Settings",
    "Profile:",
    "(New)",
    "Save",
    "Rename",
    "Delete",
    "Host:",
    "Port:",
    "Username:",
    "Authentication:",
    "Public Key Auth",
    "Private Key:",
    "Password:",
    "Auto Connect on Startup",
    "Connect",
    "Cancel",
    "Save Profile",
    "Enter profile name:",
    "Rename Profile",
    "Enter new profile name:",

    // 自動接続確認ダイアログ
    "Auto Connect Confirmation",
    "Profile '%s' already has auto connect enabled.\nDisable it and enable for this profile?",
    "Yes",
    "No",

    // 設定ダイアログ
    "Settings",
    "Language",
    "Font",
    "Font Size",
    "OK",
    "Apply",

    // ターミナル
    "Please connect to a server",

    // フォルダツリー
    "Folder Tree",
    "New Folder",
    "Delete",
    "Create Folder",
    "Folder Name",
    "Create",
    "Cancel",
    "Delete",
    "Delete this path?\n%s",
    "Yes",
    "No",

    // tmux未インストール
    "tmux not found",
    "tmux is not installed on the remote host.",
    "OK",
};

// 日本語ローカライゼーション
static const Localization japaneseLocalization = {
    // メニュー
    "接続",
    "開始",
    "停止",
    "設定...",
    "表示",
    "ターミナル",
    "コマンド",
    "フォルダ",
    "オプション",

    // 接続状態
    "接続中",
    "未接続",

    // 接続ダイアログ
    "接続設定",
    "プロファイル:",
    "(新規)",
    "保存",
    "名前変更",
    "削除",
    "ホスト:",
    "ポート:",
    "ユーザー名:",
    "認証方式:",
    "公開鍵認証",
    "秘密鍵:",
    "パスワード:",
    "起動時に自動接続",
    "接続",
    "キャンセル",
    "プロファイル保存",
    "プロファイル名を入力:",
    "プロファイル名変更",
    "新しいプロファイル名を入力:",

    // 自動接続確認ダイアログ
    "自動接続の確認",
    "プロファイル「%s」が現在自動接続の設定をしています。\n解除してこのプロファイルで起動時に自動接続しますか？",
    "はい",
    "いいえ",

    // 設定ダイアログ
    "設定",
    "言語",
    "フォント",
    "フォントサイズ",
    "OK",
    "適用",

    // ターミナル
    "接続してください",

    // フォルダツリー
    "フォルダツリー",
    "新規フォルダ",
    "削除",
    "フォルダ作成",
    "フォルダ名",
    "作成",
    "キャンセル",
    "削除",
    "このパスを削除しますか？\n%s",
    "はい",
    "いいえ",

    // tmux未インストール
    "tmuxが見つかりません",
    "接続先PCにtmuxがインストールされていません。",
    "OK",
};

const Localization& getLocalization(int language) {
    if (language == 1) {
        return japaneseLocalization;
    }
    return englishLocalization;
}

// AppSettings実装
void AppSettings::save() {
    std::string path = configPath();
    std::filesystem::path dir = std::filesystem::path(path).parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "Cannot open settings file: " << path << std::endl;
        return;
    }

    file << "language=" << language << "\n";
    file << "font_path=" << fontPath << "\n";
    file << "font_size=" << fontSize << "\n";
    file << "window_x=" << windowX << "\n";
    file << "window_y=" << windowY << "\n";
    file << "window_width=" << windowWidth << "\n";
    file << "window_height=" << windowHeight << "\n";
    file << "window_maximized=" << (windowMaximized ? "1" : "0") << "\n";

    std::cout << "Settings saved: " << path << std::endl;
}

void AppSettings::load() {
    std::string path = configPath();
    std::ifstream file(path);

    if (!file.is_open()) {
        std::cout << "No settings file: " << path << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        // 改行を削除
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }

        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        if (key == "language") {
            language = std::stoi(value);
        } else if (key == "font_path") {
            fontPath = value;
        } else if (key == "font_size") {
            fontSize = std::stof(value);
        } else if (key == "window_x") {
            windowX = std::stoi(value);
        } else if (key == "window_y") {
            windowY = std::stoi(value);
        } else if (key == "window_width") {
            windowWidth = std::stoi(value);
        } else if (key == "window_height") {
            windowHeight = std::stoi(value);
        } else if (key == "window_maximized") {
            windowMaximized = (value == "1");
        }
    }

    std::cout << "Settings loaded" << std::endl;
}

std::string AppSettings::configPath() const {
    return configDir() + "/settings.conf";
}

std::string AppSettings::configDir() {
    const char* home = std::getenv("HOME");
    if (!home) {
        home = ".";
    }
    return std::string(home) + "/.config/pbterm-imgui";
}

// SettingsDialog実装
SettingsDialog::SettingsDialog() {
    // 利用可能なフォントを設定
    m_availableFonts = {
        "HackGenConsole-Regular.ttf",
        "HackGenConsole-Bold.ttf",
        "HackGen-Regular.ttf",
        "HackGen-Bold.ttf",
        "HackGen35Console-Regular.ttf",
        "HackGen35Console-Bold.ttf",
    };
}

void SettingsDialog::setSettings(const AppSettings& settings) {
    m_settings = settings;
    m_selectedLanguage = settings.language;
    m_fontSize = settings.fontSize;

    // フォントインデックスを検索
    std::string fontName = std::filesystem::path(settings.fontPath).filename().string();
    m_selectedFont = 0;
    for (size_t i = 0; i < m_availableFonts.size(); ++i) {
        if (m_availableFonts[i] == fontName) {
            m_selectedFont = static_cast<int>(i);
            break;
        }
    }
}

void SettingsDialog::render(bool* open) {
    if (!*open) return;

    const Localization& loc = getLocalization(m_settings.language);

    ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);

    // ドッキング不可
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

    if (ImGui::Begin(loc.dlgSettingsTitle, open, flags)) {
        renderLanguageSettings();
        ImGui::Separator();
        renderFontSettings();
        ImGui::Separator();
        renderButtons(open);
    }
    ImGui::End();
}

void SettingsDialog::renderLanguageSettings() {
    const Localization& loc = getLocalization(m_settings.language);

    ImGui::Text("%s", loc.dlgLanguage);
    ImGui::SameLine(120);

    const char* languages[] = { "English", "Japanese" };
    ImGui::SetNextItemWidth(200);
    ImGui::Combo("##language", &m_selectedLanguage, languages, IM_ARRAYSIZE(languages));
}

void SettingsDialog::renderFontSettings() {
    const Localization& loc = getLocalization(m_settings.language);

    // フォント選択
    ImGui::Text("%s", loc.dlgFont);
    ImGui::SameLine(120);

    std::vector<const char*> fontNames;
    for (const auto& f : m_availableFonts) {
        fontNames.push_back(f.c_str());
    }

    ImGui::SetNextItemWidth(200);
    ImGui::Combo("##font", &m_selectedFont, fontNames.data(), static_cast<int>(fontNames.size()));

    // フォントサイズ
    ImGui::Text("%s", loc.dlgFontSize);
    ImGui::SameLine(120);
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("##fontsize", &m_fontSize, 12.0f, 32.0f, "%.0f");
}

void SettingsDialog::renderButtons(bool* open) {
    const Localization& loc = getLocalization(m_settings.language);

    ImGui::Spacing();

    float buttonWidth = 80.0f;
    float totalWidth = buttonWidth * 3 + 20; // 3つのボタン + マージン
    float windowWidth = ImGui::GetWindowWidth();
    ImGui::SetCursorPosX((windowWidth - totalWidth) * 0.5f);

    // OKボタン: 保存してダイアログを閉じる
    if (ImGui::Button(loc.dlgOK, ImVec2(buttonWidth, 0))) {
        // 設定を更新
        m_settings.language = m_selectedLanguage;
        m_settings.fontSize = m_fontSize;

        if (m_selectedFont >= 0 && m_selectedFont < static_cast<int>(m_availableFonts.size())) {
            m_settings.fontPath = "resources/fonts/" + m_availableFonts[m_selectedFont];
        }

        m_settings.save();

        if (m_applyCallback) {
            m_applyCallback(m_settings);
        }

        *open = false;
    }

    ImGui::SameLine();

    // 適用ボタン: 保存するが閉じない
    if (ImGui::Button(loc.dlgApply, ImVec2(buttonWidth, 0))) {
        // 設定を更新
        m_settings.language = m_selectedLanguage;
        m_settings.fontSize = m_fontSize;

        if (m_selectedFont >= 0 && m_selectedFont < static_cast<int>(m_availableFonts.size())) {
            m_settings.fontPath = "resources/fonts/" + m_availableFonts[m_selectedFont];
        }

        m_settings.save();

        if (m_applyCallback) {
            m_applyCallback(m_settings);
        }
    }

    ImGui::SameLine();

    // キャンセルボタン: 保存せずに閉じる
    if (ImGui::Button(loc.dlgCancel, ImVec2(buttonWidth, 0))) {
        *open = false;
    }
}

} // namespace pbterm
