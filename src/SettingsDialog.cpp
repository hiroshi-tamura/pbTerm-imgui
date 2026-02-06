#include "SettingsDialog.h"
#include "Terminal.h"
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
    "Color Theme",
    "UI Theme",
    "Preview",
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

    // ダウンロード
    "Download",
    "Download Complete!",
    "Open in Finder",
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
    "カラーテーマ",
    "UIテーマ",
    "プレビュー",
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

    // ダウンロード
    "ダウンロード",
    "ダウンロード完了！",
    "Finderで開く",
};

const Localization& getLocalization(int language) {
    if (language == 1) {
        return japaneseLocalization;
    }
    return englishLocalization;
}

// UIテーマ定義
static const std::vector<UIThemeInfo> s_uiThemes = {
    {"dark", "Dark"},
    {"light", "Light"},
    {"classic", "Classic"},
    {"dracula", "Dracula"},
    {"nord", "Nord"},
    {"spectrum", "Spectrum"},
    {"cherry", "Cherry"},
    {"steam", "Steam"},
};

// UIテーマを適用
void applyUITheme(const std::string& themeId) {
    ImGuiStyle& style = ImGui::GetStyle();

    if (themeId == "light") {
        ImGui::StyleColorsLight();
        style.WindowRounding = 4.0f;
        style.FrameRounding = 2.0f;
        style.TabRounding = 4.0f;
    }
    else if (themeId == "classic") {
        ImGui::StyleColorsClassic();
        style.WindowRounding = 4.0f;
        style.FrameRounding = 2.0f;
        style.TabRounding = 4.0f;
    }
    else if (themeId == "dracula") {
        // Dracula テーマ
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                   = ImVec4(0.97f, 0.97f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.16f, 0.16f, 0.21f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.16f, 0.16f, 0.21f, 0.94f);
        colors[ImGuiCol_Border]                 = ImVec4(0.27f, 0.28f, 0.35f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.21f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.33f, 0.34f, 0.42f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.18f, 0.19f, 0.24f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.18f, 0.19f, 0.24f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.74f, 0.58f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.74f, 0.58f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.84f, 0.68f, 1.00f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.74f, 0.58f, 0.98f, 0.60f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.74f, 0.58f, 0.98f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.74f, 0.58f, 0.98f, 0.60f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.74f, 0.58f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.74f, 0.58f, 0.98f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.74f, 0.58f, 0.98f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.74f, 0.58f, 0.98f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.74f, 0.58f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.74f, 0.58f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.19f, 0.24f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.74f, 0.58f, 0.98f, 0.80f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.27f, 0.28f, 0.35f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.13f, 0.13f, 0.17f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.18f, 0.19f, 0.24f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.74f, 0.58f, 0.98f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.74f, 0.58f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.74f, 0.58f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
        style.WindowRounding = 6.0f;
        style.FrameRounding = 4.0f;
        style.TabRounding = 4.0f;
    }
    else if (themeId == "nord") {
        // Nord テーマ
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                   = ImVec4(0.85f, 0.87f, 0.91f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.44f, 0.50f, 0.56f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.18f, 0.20f, 0.25f, 0.94f);
        colors[ImGuiCol_Border]                 = ImVec4(0.23f, 0.26f, 0.32f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.23f, 0.26f, 0.32f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.30f, 0.34f, 0.41f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.35f, 0.39f, 0.47f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.53f, 0.75f, 0.82f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.53f, 0.75f, 0.82f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.63f, 0.85f, 0.92f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.30f, 0.34f, 0.41f, 1.00f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.53f, 0.75f, 0.82f, 0.60f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.53f, 0.75f, 0.82f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.30f, 0.34f, 0.41f, 1.00f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.53f, 0.75f, 0.82f, 0.60f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.53f, 0.75f, 0.82f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.30f, 0.34f, 0.41f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.53f, 0.75f, 0.82f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.53f, 0.75f, 0.82f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.53f, 0.75f, 0.82f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.53f, 0.75f, 0.82f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.53f, 0.75f, 0.82f, 0.95f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.53f, 0.75f, 0.82f, 0.80f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.30f, 0.34f, 0.41f, 1.00f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.15f, 0.17f, 0.21f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.18f, 0.20f, 0.25f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.53f, 0.75f, 0.82f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.53f, 0.75f, 0.82f, 0.35f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.53f, 0.75f, 0.82f, 1.00f);
        style.WindowRounding = 4.0f;
        style.FrameRounding = 2.0f;
        style.TabRounding = 4.0f;
    }
    else if (themeId == "spectrum") {
        // Adobe Spectrum 風テーマ
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.18f, 0.18f, 0.18f, 0.94f);
        colors[ImGuiCol_Border]                 = ImVec4(0.30f, 0.30f, 0.30f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.21f, 0.52f, 0.89f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.21f, 0.52f, 0.89f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.31f, 0.62f, 0.99f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.21f, 0.52f, 0.89f, 0.60f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.21f, 0.52f, 0.89f, 0.80f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.21f, 0.52f, 0.89f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.21f, 0.52f, 0.89f, 0.31f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.21f, 0.52f, 0.89f, 0.60f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.21f, 0.52f, 0.89f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.21f, 0.52f, 0.89f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.21f, 0.52f, 0.89f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.21f, 0.52f, 0.89f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.21f, 0.52f, 0.89f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.21f, 0.52f, 0.89f, 0.95f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.21f, 0.52f, 0.89f, 0.80f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.21f, 0.52f, 0.89f, 0.60f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.21f, 0.52f, 0.89f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.21f, 0.52f, 0.89f, 0.35f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.21f, 0.52f, 0.89f, 1.00f);
        style.WindowRounding = 0.0f;
        style.FrameRounding = 4.0f;
        style.TabRounding = 0.0f;
    }
    else if (themeId == "cherry") {
        // Cherry テーマ（赤系アクセント）
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                   = ImVec4(0.86f, 0.93f, 0.89f, 0.88f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.86f, 0.93f, 0.89f, 0.28f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.13f, 0.14f, 0.17f, 0.94f);
        colors[ImGuiCol_Border]                 = ImVec4(0.31f, 0.31f, 0.31f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.92f, 0.18f, 0.29f, 0.40f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.92f, 0.18f, 0.29f, 0.60f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.20f, 0.22f, 0.27f, 0.75f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.20f, 0.22f, 0.27f, 0.47f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.47f, 0.77f, 0.83f, 0.21f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.92f, 0.18f, 0.29f, 0.80f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.92f, 0.18f, 0.29f, 0.60f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.47f, 0.77f, 0.83f, 0.14f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.92f, 0.18f, 0.29f, 0.76f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.14f, 0.16f, 0.19f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.47f, 0.77f, 0.83f, 0.04f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.92f, 0.18f, 0.29f, 0.78f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.92f, 0.18f, 0.29f, 0.86f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.92f, 0.18f, 0.29f, 0.60f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.13f, 0.14f, 0.17f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.20f, 0.22f, 0.27f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.92f, 0.18f, 0.29f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.92f, 0.18f, 0.29f, 0.35f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.92f, 0.18f, 0.29f, 1.00f);
        style.WindowRounding = 4.0f;
        style.FrameRounding = 2.0f;
        style.TabRounding = 4.0f;
    }
    else if (themeId == "steam") {
        // Steam 風テーマ
        ImVec4* colors = style.Colors;
        colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg]               = ImVec4(0.12f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_ChildBg]                = ImVec4(0.12f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_PopupBg]                = ImVec4(0.12f, 0.15f, 0.17f, 0.94f);
        colors[ImGuiCol_Border]                 = ImVec4(0.25f, 0.28f, 0.30f, 0.50f);
        colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg]                = ImVec4(0.18f, 0.21f, 0.24f, 1.00f);
        colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.24f, 0.39f, 0.47f, 1.00f);
        colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.48f, 0.58f, 1.00f);
        colors[ImGuiCol_TitleBg]                = ImVec4(0.10f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBgActive]          = ImVec4(0.12f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.17f, 0.19f, 1.00f);
        colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark]              = ImVec4(0.40f, 0.73f, 0.42f, 1.00f);
        colors[ImGuiCol_SliderGrab]             = ImVec4(0.40f, 0.73f, 0.42f, 1.00f);
        colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.50f, 0.83f, 0.52f, 1.00f);
        colors[ImGuiCol_Button]                 = ImVec4(0.40f, 0.73f, 0.42f, 0.60f);
        colors[ImGuiCol_ButtonHovered]          = ImVec4(0.40f, 0.73f, 0.42f, 0.80f);
        colors[ImGuiCol_ButtonActive]           = ImVec4(0.40f, 0.73f, 0.42f, 1.00f);
        colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.73f, 0.42f, 0.31f);
        colors[ImGuiCol_HeaderHovered]          = ImVec4(0.40f, 0.73f, 0.42f, 0.60f);
        colors[ImGuiCol_HeaderActive]           = ImVec4(0.40f, 0.73f, 0.42f, 1.00f);
        colors[ImGuiCol_Separator]              = ImVec4(0.25f, 0.28f, 0.30f, 1.00f);
        colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.40f, 0.73f, 0.42f, 0.78f);
        colors[ImGuiCol_SeparatorActive]        = ImVec4(0.40f, 0.73f, 0.42f, 1.00f);
        colors[ImGuiCol_ResizeGrip]             = ImVec4(0.40f, 0.73f, 0.42f, 0.20f);
        colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.40f, 0.73f, 0.42f, 0.67f);
        colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.40f, 0.73f, 0.42f, 0.95f);
        colors[ImGuiCol_Tab]                    = ImVec4(0.12f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_TabHovered]             = ImVec4(0.40f, 0.73f, 0.42f, 0.80f);
        colors[ImGuiCol_TabActive]              = ImVec4(0.40f, 0.73f, 0.42f, 0.60f);
        colors[ImGuiCol_TabUnfocused]           = ImVec4(0.10f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.12f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_DockingPreview]         = ImVec4(0.40f, 0.73f, 0.42f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.40f, 0.73f, 0.42f, 0.35f);
        colors[ImGuiCol_NavHighlight]           = ImVec4(0.40f, 0.73f, 0.42f, 1.00f);
        style.WindowRounding = 0.0f;
        style.FrameRounding = 0.0f;
        style.TabRounding = 0.0f;
    }
    else {
        // Dark (デフォルト)
        ImGui::StyleColorsDark();
        style.WindowRounding = 4.0f;
        style.FrameRounding = 2.0f;
        style.TabRounding = 4.0f;
    }
}

const std::vector<UIThemeInfo>& getAvailableUIThemes() {
    return s_uiThemes;
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
    file << "color_theme=" << colorTheme << "\n";
    file << "ui_theme=" << uiTheme << "\n";

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
        } else if (key == "color_theme") {
            colorTheme = value;
        } else if (key == "ui_theme") {
            uiTheme = value;
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

    // ターミナルカラーテーマインデックスを検索
    const auto& themes = getAvailableThemes();
    m_selectedTheme = 0;
    for (size_t i = 0; i < themes.size(); ++i) {
        if (themes[i].id == settings.colorTheme) {
            m_selectedTheme = static_cast<int>(i);
            break;
        }
    }

    // UIテーマインデックスを検索
    const auto& uiThemes = getAvailableUIThemes();
    m_selectedUITheme = 0;
    for (size_t i = 0; i < uiThemes.size(); ++i) {
        if (uiThemes[i].id == settings.uiTheme) {
            m_selectedUITheme = static_cast<int>(i);
            break;
        }
    }
}

void SettingsDialog::render(bool* open) {
    if (!*open) return;

    const Localization& loc = getLocalization(m_settings.language);

    ImGui::SetNextWindowSize(ImVec2(450, 520), ImGuiCond_FirstUseEver);

    // ドッキング不可
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;

    if (ImGui::Begin(loc.dlgSettingsTitle, open, flags)) {
        renderLanguageSettings();
        ImGui::Separator();
        renderFontSettings();
        ImGui::Separator();
        renderUIThemeSettings();
        ImGui::Separator();
        renderColorThemeSettings();
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

void SettingsDialog::renderUIThemeSettings() {
    const Localization& loc = getLocalization(m_settings.language);
    const auto& uiThemes = getAvailableUIThemes();

    // UIテーマ選択
    ImGui::Text("%s", loc.dlgUITheme);
    ImGui::SameLine(120);

    std::vector<const char*> themeNames;
    for (const auto& t : uiThemes) {
        themeNames.push_back(t.name);
    }

    ImGui::SetNextItemWidth(200);
    if (ImGui::Combo("##uiTheme", &m_selectedUITheme, themeNames.data(), static_cast<int>(themeNames.size()))) {
        // 変更があったらプレビューとして即座に適用
        if (m_selectedUITheme >= 0 && m_selectedUITheme < static_cast<int>(uiThemes.size())) {
            applyUITheme(uiThemes[m_selectedUITheme].id);
        }
    }
}

void SettingsDialog::renderColorThemeSettings() {
    const Localization& loc = getLocalization(m_settings.language);
    const auto& themes = getAvailableThemes();

    // テーマ選択
    ImGui::Text("%s", loc.dlgColorTheme);
    ImGui::SameLine(120);

    std::vector<const char*> themeNames;
    for (const auto& t : themes) {
        themeNames.push_back(t.name.c_str());
    }

    ImGui::SetNextItemWidth(200);
    ImGui::Combo("##colorTheme", &m_selectedTheme, themeNames.data(), static_cast<int>(themeNames.size()));

    // プレビュー
    ImGui::Spacing();
    ImGui::Text("%s:", loc.dlgPreview);

    // プレビュー描画エリア
    float previewWidth = ImGui::GetContentRegionAvail().x - 10;
    float previewHeight = 120;
    renderThemePreview(previewWidth, previewHeight);
}

void SettingsDialog::renderThemePreview(float width, float height) {
    const auto& themes = getAvailableThemes();
    if (m_selectedTheme < 0 || m_selectedTheme >= static_cast<int>(themes.size())) {
        return;
    }

    const TerminalColorTheme& theme = themes[m_selectedTheme];

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    // 背景
    drawList->AddRectFilled(
        pos,
        ImVec2(pos.x + width, pos.y + height),
        theme.background.toImU32(),
        4.0f
    );

    // 枠線
    drawList->AddRect(
        pos,
        ImVec2(pos.x + width, pos.y + height),
        IM_COL32(100, 100, 100, 200),
        4.0f
    );

    // プロンプトのサンプルテキスト
    float lineHeight = 16.0f;
    float x = pos.x + 10;
    float y = pos.y + 10;

    // user@host:~$ の部分
    drawList->AddText(ImVec2(x, y), theme.colors[2].toImU32(), "user");  // 緑
    drawList->AddText(ImVec2(x + 28, y), theme.foreground.toImU32(), "@");
    drawList->AddText(ImVec2(x + 38, y), theme.colors[6].toImU32(), "host");  // シアン
    drawList->AddText(ImVec2(x + 66, y), theme.foreground.toImU32(), ":~$ ");
    drawList->AddText(ImVec2(x + 94, y), theme.foreground.toImU32(), "ls -la");

    y += lineHeight;

    // lsコマンドの出力例
    drawList->AddText(ImVec2(x, y), theme.colors[4].toImU32(), "drwxr-xr-x");  // 青
    drawList->AddText(ImVec2(x + 80, y), theme.foreground.toImU32(), "  Documents/");

    y += lineHeight;

    drawList->AddText(ImVec2(x, y), theme.colors[2].toImU32(), "-rwxr-xr-x");  // 緑
    drawList->AddText(ImVec2(x + 80, y), theme.foreground.toImU32(), "  script.sh");

    y += lineHeight + 8;

    // ANSI 16色パレット表示
    float boxSize = 14.0f;
    float spacing = 2.0f;

    // 通常色 (0-7)
    for (int i = 0; i < 8; ++i) {
        ImVec2 boxPos(x + i * (boxSize + spacing), y);
        drawList->AddRectFilled(
            boxPos,
            ImVec2(boxPos.x + boxSize, boxPos.y + boxSize),
            theme.colors[i].toImU32()
        );
    }

    y += boxSize + spacing;

    // 明るい色 (8-15)
    for (int i = 8; i < 16; ++i) {
        ImVec2 boxPos(x + (i - 8) * (boxSize + spacing), y);
        drawList->AddRectFilled(
            boxPos,
            ImVec2(boxPos.x + boxSize, boxPos.y + boxSize),
            theme.colors[i].toImU32()
        );
    }

    // カーソル表示
    float cursorX = pos.x + width - 40;
    float cursorY = pos.y + 10;
    drawList->AddRectFilled(
        ImVec2(cursorX, cursorY),
        ImVec2(cursorX + 8, cursorY + lineHeight),
        IM_COL32(theme.cursor.r, theme.cursor.g, theme.cursor.b, 180)
    );

    // プレビューエリア用のダミー
    ImGui::Dummy(ImVec2(width, height));
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

        // ターミナルカラーテーマを更新
        const auto& themes = getAvailableThemes();
        if (m_selectedTheme >= 0 && m_selectedTheme < static_cast<int>(themes.size())) {
            m_settings.colorTheme = themes[m_selectedTheme].id;
        }

        // UIテーマを更新
        const auto& uiThemes = getAvailableUIThemes();
        if (m_selectedUITheme >= 0 && m_selectedUITheme < static_cast<int>(uiThemes.size())) {
            m_settings.uiTheme = uiThemes[m_selectedUITheme].id;
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

        // ターミナルカラーテーマを更新
        const auto& themes = getAvailableThemes();
        if (m_selectedTheme >= 0 && m_selectedTheme < static_cast<int>(themes.size())) {
            m_settings.colorTheme = themes[m_selectedTheme].id;
        }

        // UIテーマを更新
        const auto& uiThemes = getAvailableUIThemes();
        if (m_selectedUITheme >= 0 && m_selectedUITheme < static_cast<int>(uiThemes.size())) {
            m_settings.uiTheme = uiThemes[m_selectedUITheme].id;
        }

        m_settings.save();

        if (m_applyCallback) {
            m_applyCallback(m_settings);
        }
    }

    ImGui::SameLine();

    // キャンセルボタン: 保存せずに閉じる（UIテーマを元に戻す）
    if (ImGui::Button(loc.dlgCancel, ImVec2(buttonWidth, 0))) {
        // UIテーマを元に戻す
        applyUITheme(m_settings.uiTheme);
        *open = false;
    }
}

} // namespace pbterm
