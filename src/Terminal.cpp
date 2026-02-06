#include "Terminal.h"
#include "SshConnection.h"
#include <iostream>
#include <cstring>
#include <memory>

namespace pbterm {

// カラーテーマ定義（10個）
static const std::vector<TerminalColorTheme> s_colorThemes = {
    // 1. Default - 現行の黒背景
    {
        "Default", "default",
        {0, 0, 0},           // 背景
        {229, 229, 229},     // 前景
        {200, 200, 200},     // カーソル
        {100, 150, 255, 100}, // 選択
        {
            {0, 0, 0}, {205, 0, 0}, {0, 205, 0}, {205, 205, 0},
            {0, 0, 238}, {205, 0, 205}, {0, 205, 205}, {229, 229, 229},
            {127, 127, 127}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0},
            {92, 92, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}
        }
    },
    // 2. Solarized Dark
    {
        "Solarized Dark", "solarized-dark",
        {0, 43, 54},         // 背景
        {131, 148, 150},     // 前景
        {147, 161, 161},     // カーソル
        {7, 54, 66, 100},    // 選択
        {
            {7, 54, 66}, {220, 50, 47}, {133, 153, 0}, {181, 137, 0},
            {38, 139, 210}, {211, 54, 130}, {42, 161, 152}, {238, 232, 213},
            {0, 43, 54}, {203, 75, 22}, {88, 110, 117}, {101, 123, 131},
            {131, 148, 150}, {108, 113, 196}, {147, 161, 161}, {253, 246, 227}
        }
    },
    // 3. Solarized Light
    {
        "Solarized Light", "solarized-light",
        {253, 246, 227},     // 背景
        {101, 123, 131},     // 前景
        {88, 110, 117},      // カーソル
        {238, 232, 213, 100}, // 選択
        {
            {7, 54, 66}, {220, 50, 47}, {133, 153, 0}, {181, 137, 0},
            {38, 139, 210}, {211, 54, 130}, {42, 161, 152}, {238, 232, 213},
            {0, 43, 54}, {203, 75, 22}, {88, 110, 117}, {101, 123, 131},
            {131, 148, 150}, {108, 113, 196}, {147, 161, 161}, {253, 246, 227}
        }
    },
    // 4. Dracula
    {
        "Dracula", "dracula",
        {40, 42, 54},        // 背景
        {248, 248, 242},     // 前景
        {248, 248, 242},     // カーソル
        {68, 71, 90, 100},   // 選択
        {
            {33, 34, 44}, {255, 85, 85}, {80, 250, 123}, {241, 250, 140},
            {189, 147, 249}, {255, 121, 198}, {139, 233, 253}, {248, 248, 242},
            {98, 114, 164}, {255, 110, 110}, {105, 255, 148}, {255, 255, 165},
            {214, 172, 255}, {255, 146, 223}, {164, 255, 255}, {255, 255, 255}
        }
    },
    // 5. One Dark
    {
        "One Dark", "one-dark",
        {40, 44, 52},        // 背景
        {171, 178, 191},     // 前景
        {171, 178, 191},     // カーソル
        {62, 68, 81, 100},   // 選択
        {
            {40, 44, 52}, {224, 108, 117}, {152, 195, 121}, {229, 192, 123},
            {97, 175, 239}, {198, 120, 221}, {86, 182, 194}, {171, 178, 191},
            {92, 99, 112}, {224, 108, 117}, {152, 195, 121}, {229, 192, 123},
            {97, 175, 239}, {198, 120, 221}, {86, 182, 194}, {255, 255, 255}
        }
    },
    // 6. Nord
    {
        "Nord", "nord",
        {46, 52, 64},        // 背景
        {216, 222, 233},     // 前景
        {216, 222, 233},     // カーソル
        {67, 76, 94, 100},   // 選択
        {
            {59, 66, 82}, {191, 97, 106}, {163, 190, 140}, {235, 203, 139},
            {129, 161, 193}, {180, 142, 173}, {136, 192, 208}, {229, 233, 240},
            {76, 86, 106}, {191, 97, 106}, {163, 190, 140}, {235, 203, 139},
            {129, 161, 193}, {180, 142, 173}, {143, 188, 187}, {236, 239, 244}
        }
    },
    // 7. Monokai
    {
        "Monokai", "monokai",
        {39, 40, 34},        // 背景
        {248, 248, 242},     // 前景
        {248, 248, 242},     // カーソル
        {73, 72, 62, 100},   // 選択
        {
            {39, 40, 34}, {249, 38, 114}, {166, 226, 46}, {244, 191, 117},
            {102, 217, 239}, {174, 129, 255}, {161, 239, 228}, {248, 248, 242},
            {117, 113, 94}, {249, 38, 114}, {166, 226, 46}, {244, 191, 117},
            {102, 217, 239}, {174, 129, 255}, {161, 239, 228}, {249, 248, 245}
        }
    },
    // 8. Gruvbox Dark
    {
        "Gruvbox Dark", "gruvbox-dark",
        {40, 40, 40},        // 背景
        {235, 219, 178},     // 前景
        {235, 219, 178},     // カーソル
        {80, 73, 69, 100},   // 選択
        {
            {40, 40, 40}, {204, 36, 29}, {152, 151, 26}, {215, 153, 33},
            {69, 133, 136}, {177, 98, 134}, {104, 157, 106}, {168, 153, 132},
            {146, 131, 116}, {251, 73, 52}, {184, 187, 38}, {250, 189, 47},
            {131, 165, 152}, {211, 134, 155}, {142, 192, 124}, {235, 219, 178}
        }
    },
    // 9. Tokyo Night
    {
        "Tokyo Night", "tokyo-night",
        {26, 27, 38},        // 背景
        {169, 177, 214},     // 前景
        {192, 202, 245},     // カーソル
        {52, 59, 88, 100},   // 選択
        {
            {21, 22, 30}, {247, 118, 142}, {158, 206, 106}, {224, 175, 104},
            {122, 162, 247}, {187, 154, 247}, {125, 207, 255}, {192, 202, 245},
            {65, 72, 104}, {247, 118, 142}, {158, 206, 106}, {224, 175, 104},
            {122, 162, 247}, {187, 154, 247}, {125, 207, 255}, {255, 255, 255}
        }
    },
    // 10. Catppuccin Mocha
    {
        "Catppuccin Mocha", "catppuccin-mocha",
        {30, 30, 46},        // 背景
        {205, 214, 244},     // 前景
        {245, 224, 220},     // カーソル
        {69, 71, 90, 100},   // 選択
        {
            {69, 71, 90}, {243, 139, 168}, {166, 227, 161}, {249, 226, 175},
            {137, 180, 250}, {245, 194, 231}, {148, 226, 213}, {186, 194, 222},
            {88, 91, 112}, {243, 139, 168}, {166, 227, 161}, {249, 226, 175},
            {137, 180, 250}, {245, 194, 231}, {148, 226, 213}, {205, 214, 244}
        }
    }
};

const std::vector<TerminalColorTheme>& getAvailableThemes() {
    return s_colorThemes;
}

const TerminalColorTheme* getThemeById(const std::string& id) {
    for (const auto& theme : s_colorThemes) {
        if (theme.id == id) {
            return &theme;
        }
    }
    return nullptr;
}

// Unicode文字が全角かどうかを判定
static bool isWideChar(uint32_t codepoint) {
    // CJK記号、ひらがな、カタカナ、漢字、全角英数など
    if (codepoint >= 0x1100 && codepoint <= 0x115F) return true;  // Hangul Jamo
    if (codepoint >= 0x231A && codepoint <= 0x231B) return true;  // Watch, Hourglass
    if (codepoint >= 0x2600 && codepoint <= 0x26FF) return true;  // Miscellaneous Symbols
    if (codepoint >= 0x2700 && codepoint <= 0x27BF) return true;  // Dingbats
    if (codepoint >= 0x2E80 && codepoint <= 0x2EFF) return true;  // CJK Radicals
    if (codepoint >= 0x2F00 && codepoint <= 0x2FDF) return true;  // Kangxi Radicals
    if (codepoint >= 0x3000 && codepoint <= 0x303F) return true;  // CJK Symbols
    if (codepoint >= 0x3040 && codepoint <= 0x309F) return true;  // Hiragana
    if (codepoint >= 0x30A0 && codepoint <= 0x30FF) return true;  // Katakana
    if (codepoint >= 0x3100 && codepoint <= 0x312F) return true;  // Bopomofo
    if (codepoint >= 0x3130 && codepoint <= 0x318F) return true;  // Hangul Compatibility
    if (codepoint >= 0x31F0 && codepoint <= 0x31FF) return true;  // Katakana Phonetic
    if (codepoint >= 0x3200 && codepoint <= 0x32FF) return true;  // Enclosed CJK
    if (codepoint >= 0x3300 && codepoint <= 0x33FF) return true;  // CJK Compatibility
    if (codepoint >= 0x3400 && codepoint <= 0x4DBF) return true;  // CJK Extension A
    if (codepoint >= 0x4E00 && codepoint <= 0x9FFF) return true;  // CJK Unified Ideographs
    if (codepoint >= 0xA000 && codepoint <= 0xA4CF) return true;  // Yi
    if (codepoint >= 0xAC00 && codepoint <= 0xD7AF) return true;  // Hangul Syllables
    if (codepoint >= 0xF900 && codepoint <= 0xFAFF) return true;  // CJK Compatibility Ideographs
    if (codepoint >= 0xFE10 && codepoint <= 0xFE1F) return true;  // Vertical Forms
    if (codepoint >= 0xFE30 && codepoint <= 0xFE4F) return true;  // CJK Compatibility Forms
    if (codepoint >= 0xFF00 && codepoint <= 0xFF60) return true;  // Fullwidth Forms
    if (codepoint >= 0xFFE0 && codepoint <= 0xFFE6) return true;  // Fullwidth Forms
    if (codepoint >= 0x1F300 && codepoint <= 0x1F9FF) return true; // 絵文字（Emoji）
    if (codepoint >= 0x1FA00 && codepoint <= 0x1FAFF) return true; // 追加絵文字
    if (codepoint >= 0x20000 && codepoint <= 0x2FFFF) return true; // CJK Extension B-F
    return false;
}

// libvtermコールバック構造体
static VTermScreenCallbacks screenCallbacks = {
    Terminal::onDamage,
    nullptr, // moverect
    Terminal::onMoveCursor,
    Terminal::onSetTermProp,
    Terminal::onBell,
    Terminal::onResize,
    Terminal::onSbPushline,
    Terminal::onSbPopline,
};

// vterm出力コールバック
static void vtermOutputCallback(const char* s, size_t len, void* user) {
    Terminal* term = static_cast<Terminal*>(user);
    term->sendToConnection(s, len);
}

Terminal::Terminal(int cols, int rows)
    : m_cols(cols), m_rows(rows)
{
    // デフォルトテーマを設定
    m_colorTheme = s_colorThemes[0];

    // libvterm初期化
    m_vterm = vterm_new(rows, cols);
    vterm_set_utf8(m_vterm, 1);

    // 出力コールバック設定（キー入力の結果をSSHに送信）
    vterm_output_set_callback(m_vterm, vtermOutputCallback, this);

    // スクリーン取得
    m_screen = vterm_obtain_screen(m_vterm);
    vterm_screen_set_callbacks(m_screen, &screenCallbacks, this);
    vterm_screen_reset(m_screen, 1);

    // OSCコールバック設定（スレッドセーフティの問題があるため一時的に無効化）
    // VTermState* state = vterm_obtain_state(m_vterm);
    // VTermStateFallbacks fallbacks = {};
    // fallbacks.osc = Terminal::onOsc;
    // vterm_state_set_unrecognised_fallbacks(state, &fallbacks, this);

    // セル配列初期化
    m_cells.resize(m_rows);
    for (auto& row : m_cells) {
        row.resize(m_cols);
    }
}

Terminal::~Terminal() {
    if (m_vterm) {
        vterm_free(m_vterm);
    }
}

void Terminal::setChannel(std::shared_ptr<SshChannel> channel) {
    m_channel = channel;

    if (m_channel) {
        m_channel->setDataCallback([this](const char* data, size_t len) {
            onData(data, len);
        });
    }
}

void Terminal::setConnection(SshConnection* connection) {
    m_connection = connection;
    // 後方互換性: SshConnectionのデフォルトチャンネルを使用
    if (m_connection) {
        m_connection->setDataCallback([this](const char* data, size_t len) {
            onData(data, len);
        });
    }
}

void Terminal::sendToConnection(const char* data, size_t len) {
    if (len > 0) {
        if (m_channel) {
            m_channel->write(data, len);
        } else if (m_connection) {
            m_connection->write(data, len);
        } else {
            std::cerr << "sendToConnection: 接続なし" << std::endl;
        }
    }
}

void Terminal::onData(const char* data, size_t len) {
    std::lock_guard<std::mutex> lock(m_mutex);
    vterm_input_write(m_vterm, data, len);
    updateScreen();
}

void Terminal::onKeyInput(ImGuiKey key, bool ctrl, bool shift, bool alt) {
    if (!m_channel && !m_connection) return;

    // 直接エスケープシーケンスを送信
    const char* seq = nullptr;
    char buf[16];
    int len = 0;

    switch (key) {
        case ImGuiKey_Enter:
            if (shift) {
                // Shift+Enter: 改行（LF）を送信
                buf[0] = '\n';
                len = 1;
            } else {
                // 通常のEnter: CR を送信
                buf[0] = '\r';
                len = 1;
            }
            break;
        case ImGuiKey_Tab:
            buf[0] = '\t';
            len = 1;
            break;
        case ImGuiKey_Backspace:
            buf[0] = 0x7f;  // DEL
            len = 1;
            break;
        case ImGuiKey_Escape:
            buf[0] = 0x1b;
            len = 1;
            break;
        case ImGuiKey_UpArrow:
            seq = "\x1b[A";
            break;
        case ImGuiKey_DownArrow:
            seq = "\x1b[B";
            break;
        case ImGuiKey_RightArrow:
            seq = "\x1b[C";
            break;
        case ImGuiKey_LeftArrow:
            seq = "\x1b[D";
            break;
        case ImGuiKey_Home:
            seq = "\x1b[H";
            break;
        case ImGuiKey_End:
            seq = "\x1b[F";
            break;
        case ImGuiKey_Insert:
            seq = "\x1b[2~";
            break;
        case ImGuiKey_Delete:
            seq = "\x1b[3~";
            break;
        case ImGuiKey_PageUp:
            seq = "\x1b[5~";
            break;
        case ImGuiKey_PageDown:
            seq = "\x1b[6~";
            break;
        case ImGuiKey_F1:
            seq = "\x1bOP";
            break;
        case ImGuiKey_F2:
            seq = "\x1bOQ";
            break;
        case ImGuiKey_F3:
            seq = "\x1bOR";
            break;
        case ImGuiKey_F4:
            seq = "\x1bOS";
            break;
        case ImGuiKey_F5:
            seq = "\x1b[15~";
            break;
        case ImGuiKey_F6:
            seq = "\x1b[17~";
            break;
        case ImGuiKey_F7:
            seq = "\x1b[18~";
            break;
        case ImGuiKey_F8:
            seq = "\x1b[19~";
            break;
        case ImGuiKey_F9:
            seq = "\x1b[20~";
            break;
        case ImGuiKey_F10:
            seq = "\x1b[21~";
            break;
        case ImGuiKey_F11:
            seq = "\x1b[23~";
            break;
        case ImGuiKey_F12:
            seq = "\x1b[24~";
            break;
        default:
            return;
    }

    if (seq) {
        sendToConnection(seq, strlen(seq));
    } else if (len > 0) {
        sendToConnection(buf, len);
    }
}

void Terminal::onCharInput(unsigned int c) {
    if (!m_channel && !m_connection) return;

    // Ctrl+キー処理
    ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && c >= 'a' && c <= 'z') {
        char ctrl = static_cast<char>(c - 'a' + 1);
        sendToConnection(&ctrl, 1);
        return;
    }

    // UTF-8エンコード
    char utf8[4];
    int len = 0;

    if (c < 0x80) {
        utf8[0] = static_cast<char>(c);
        len = 1;
    } else if (c < 0x800) {
        utf8[0] = static_cast<char>(0xC0 | (c >> 6));
        utf8[1] = static_cast<char>(0x80 | (c & 0x3F));
        len = 2;
    } else if (c < 0x10000) {
        utf8[0] = static_cast<char>(0xE0 | (c >> 12));
        utf8[1] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        utf8[2] = static_cast<char>(0x80 | (c & 0x3F));
        len = 3;
    } else {
        utf8[0] = static_cast<char>(0xF0 | (c >> 18));
        utf8[1] = static_cast<char>(0x80 | ((c >> 12) & 0x3F));
        utf8[2] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
        utf8[3] = static_cast<char>(0x80 | (c & 0x3F));
        len = 4;
    }

    sendToConnection(utf8, len);
}

void Terminal::resize(int cols, int rows) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // サイズが変わらない場合は何もしない
    if (cols == m_cols && rows == m_rows) {
        return;
    }

    // リサイズ中フラグを設定（スクロールバックへのプッシュを抑制）
    m_resizing = true;

    // スクロールバックをクリア（リサイズ時の表示崩れを防ぐ）
    m_scrollback.clear();

    m_cols = cols;
    m_rows = rows;

    vterm_set_size(m_vterm, rows, cols);

    // セル配列をリサイズしてクリア
    m_cells.resize(m_rows);
    for (auto& row : m_cells) {
        row.clear();
        row.resize(m_cols);
    }

    if (m_channel) {
        m_channel->resize(cols, rows);
    } else if (m_connection) {
        m_connection->resize(cols, rows);
    }

    // リサイズ中フラグを解除
    m_resizing = false;

    // リサイズ時間を記録（リサイズ後一定期間スクロールバックへのプッシュを抑制）
    m_lastResizeTime = std::chrono::steady_clock::now();

    updateScreen();
}

void Terminal::clearScreen() {
    std::lock_guard<std::mutex> lock(m_mutex);

    // スクロールバックをクリア
    m_scrollback.clear();

    // セルを空に
    for (auto& row : m_cells) {
        for (auto& cell : row) {
            cell.text.clear();
            cell.width = 1;
        }
    }

    // vterm側もリセット
    if (m_screen) {
        vterm_screen_reset(m_screen, 1);
    }

    m_autoScroll = true;
}

void Terminal::render(ImFont* font) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!font) return;

    ImGui::PushFont(font);

    ImVec2 charSize = ImGui::CalcTextSize("A");

    // スクロール可能な子ウィンドウを作成
    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    ImGuiWindowFlags scrollFlags = ImGuiWindowFlags_HorizontalScrollbar;
    ImGui::BeginChild("##terminalScroll", contentSize, ImGuiChildFlags_None, scrollFlags);

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    int scrollbackRows = static_cast<int>(m_scrollback.size());
    float yOffset = scrollbackRows * charSize.y;

    // マウス選択処理
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isHovered = ImGui::IsWindowHovered();

    // マウス位置をセル座標に変換
    auto screenToCell = [&](ImVec2 screenPos, int& outCol, int& outRow) {
        float relX = screenPos.x - pos.x;
        float relY = screenPos.y - pos.y;
        outCol = static_cast<int>(relX / charSize.x);
        outRow = static_cast<int>(relY / charSize.y) - scrollbackRows;
        outCol = std::max(0, std::min(outCol, m_cols - 1));
        outRow = std::max(-scrollbackRows, std::min(outRow, m_rows - 1));
    };

    // 左クリックで選択開始
    if (isHovered && ImGui::IsMouseClicked(0)) {
        screenToCell(mousePos, m_selStartCol, m_selStartRow);
        m_selEndCol = m_selStartCol;
        m_selEndRow = m_selStartRow;
        m_selecting = true;
        m_hasSelection = false;
    }

    // ドラッグ中
    if (m_selecting && ImGui::IsMouseDown(0)) {
        screenToCell(mousePos, m_selEndCol, m_selEndRow);
        if (m_selStartCol != m_selEndCol || m_selStartRow != m_selEndRow) {
            m_hasSelection = true;
        }
    }

    // 左ボタン離したら選択終了
    if (m_selecting && ImGui::IsMouseReleased(0)) {
        m_selecting = false;
    }

    // 右クリック処理
    if (isHovered && ImGui::IsMouseClicked(1)) {
        if (m_hasSelection) {
            // 選択中なら、選択テキストをクリップボードにコピー
            std::string selectedText = getSelectedText();
            if (!selectedText.empty()) {
                ImGui::SetClipboardText(selectedText.c_str());
            }
            clearSelection();
        } else {
            // 選択なしなら、クリップボードからペースト
            const char* clipboard = ImGui::GetClipboardText();
            if (clipboard && clipboard[0]) {
                sendToConnection(clipboard, strlen(clipboard));
            }
        }
    }

    // 全体の背景（スクロールバック + 現在の画面）- テーマの背景色を使用
    ImU32 bgColor = m_colorTheme.background.toImU32();
    float totalHeight = (scrollbackRows + m_rows) * charSize.y;
    drawList->AddRectFilled(pos, ImVec2(pos.x + charSize.x * m_cols, pos.y + totalHeight), bgColor);

    // スクロールバック行の描画
    for (int row = 0; row < scrollbackRows; ++row) {
        const auto& sbRow = m_scrollback[row];
        for (int col = 0; col < static_cast<int>(sbRow.size()) && col < m_cols; ++col) {
            const TerminalCell& cell = sbRow[col];
            if (cell.width == 0) continue;  // 全角文字の後続セルはスキップ
            if (cell.text.empty()) continue;

            ImVec2 cellPos(pos.x + col * charSize.x, pos.y + row * charSize.y);
            ImU32 fgColor = cell.reverse ? cell.bg.toImU32() : cell.fg.toImU32();
            drawList->AddText(cellPos, fgColor, cell.text.c_str());
        }
    }

    // 現在の画面のセル描画
    for (int row = 0; row < m_rows; ++row) {
        int skipNext = 0;
        for (int col = 0; col < m_cols; ++col) {
            // 前の全角文字の継続セルはスキップ
            if (skipNext > 0) {
                skipNext--;
                continue;
            }

            const TerminalCell& cell = m_cells[row][col];

            // width==0は継続セルなのでスキップ
            if (cell.width == 0) continue;
            if (cell.text.empty()) continue;

            // 実際の幅を決定（全角文字は2セル分）
            int actualWidth = cell.width;
            if (actualWidth <= 0) actualWidth = 1;

            ImVec2 cellPos(pos.x + col * charSize.x, pos.y + yOffset + row * charSize.y);
            float cellWidth = charSize.x * actualWidth;

            // 全角文字なら次のセルをスキップ
            if (actualWidth >= 2) {
                skipNext = actualWidth - 1;
            }

            // 背景色と前景色（逆ビデオ対応）
            TerminalColor bgColor = cell.reverse ? cell.fg : cell.bg;
            TerminalColor fgColor = cell.reverse ? cell.bg : cell.fg;

            // 背景色を描画（逆ビデオ時は前景色が背景になる）
            if (cell.reverse || bgColor.r != 0 || bgColor.g != 0 || bgColor.b != 0) {
                drawList->AddRectFilled(
                    cellPos,
                    ImVec2(cellPos.x + cellWidth, cellPos.y + charSize.y),
                    bgColor.toImU32()
                );
            }

            // 文字
            drawList->AddText(cellPos, fgColor.toImU32(), cell.text.c_str());

            // 下線
            if (cell.underline) {
                drawList->AddLine(
                    ImVec2(cellPos.x, cellPos.y + charSize.y - 1),
                    ImVec2(cellPos.x + cellWidth, cellPos.y + charSize.y - 1),
                    fgColor.toImU32()
                );
            }
        }
    }

    // ウィンドウがフォーカスされているか判定
    bool windowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    // 選択範囲のハイライト描画
    if (m_hasSelection) {
        // 選択範囲を正規化（開始が終了より前になるように）
        int startRow = m_selStartRow, startCol = m_selStartCol;
        int endRow = m_selEndRow, endCol = m_selEndCol;
        if (startRow > endRow || (startRow == endRow && startCol > endCol)) {
            std::swap(startRow, endRow);
            std::swap(startCol, endCol);
        }

        // テーマの選択色を使用
        ImU32 selColor = m_colorTheme.selection.toImU32();

        for (int row = startRow; row <= endRow; ++row) {
            int colStart = (row == startRow) ? startCol : 0;
            int colEnd = (row == endRow) ? endCol : m_cols - 1;

            float drawY = pos.y + (row + scrollbackRows) * charSize.y;
            ImVec2 selStart(pos.x + colStart * charSize.x, drawY);
            ImVec2 selEnd(pos.x + (colEnd + 1) * charSize.x, drawY + charSize.y);
            drawList->AddRectFilled(selStart, selEnd, selColor);
        }
    }

    // カーソル（アプリがカーソルを可視に設定している場合のみ表示）
    // Claude Codeなどのリッチアプリはカーソルを非表示にして独自UIを描画する
    if (m_cursorVisible &&
        m_cursorRow >= 0 && m_cursorRow < m_rows &&
        m_cursorCol >= 0 && m_cursorCol < m_cols) {
        ImVec2 cursorPos(pos.x + m_cursorCol * charSize.x, pos.y + yOffset + m_cursorRow * charSize.y);

        bool showCursor = true;
        if (windowFocused) {
            // フォーカス時は点滅
            float time = static_cast<float>(ImGui::GetTime());
            showCursor = fmod(time, 1.0f) < 0.5f;
        }

        if (showCursor) {
            // フォーカス時は塗りつぶし、非フォーカス時は枠線 - テーマのカーソル色を使用
            if (windowFocused) {
                drawList->AddRectFilled(
                    cursorPos,
                    ImVec2(cursorPos.x + charSize.x, cursorPos.y + charSize.y),
                    IM_COL32(m_colorTheme.cursor.r, m_colorTheme.cursor.g, m_colorTheme.cursor.b, 180)
                );
            } else {
                drawList->AddRect(
                    cursorPos,
                    ImVec2(cursorPos.x + charSize.x, cursorPos.y + charSize.y),
                    IM_COL32(m_colorTheme.cursor.r, m_colorTheme.cursor.g, m_colorTheme.cursor.b, 200),
                    0.0f, 0, 1.5f
                );
            }
        }
    }

    // スクロール可能な領域のサイズを設定
    ImGui::Dummy(ImVec2(charSize.x * m_cols, totalHeight));

    // 新しい出力時は自動スクロール
    if (m_autoScroll) {
        ImGui::SetScrollHereY(1.0f);
        m_autoScroll = false;
    }

    // キー入力処理（ウィンドウフォーカス時）
    if (windowFocused) {
        ImGuiIO& io = ImGui::GetIO();

        // IME入力位置をカーソル位置に設定
        if (m_cursorRow >= 0 && m_cursorRow < m_rows &&
            m_cursorCol >= 0 && m_cursorCol < m_cols) {
            ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
            if (platform_io.Platform_SetImeDataFn) {
                ImGuiPlatformImeData ime_data;
                ime_data.WantVisible = true;
                ime_data.InputPos = ImVec2(pos.x + m_cursorCol * charSize.x,
                                           pos.y + yOffset + m_cursorRow * charSize.y + charSize.y);
                ime_data.InputLineHeight = charSize.y;
                platform_io.Platform_SetImeDataFn(ImGui::GetCurrentContext(),
                                                   ImGui::GetMainViewport(), &ime_data);
            }
        }

        // 文字入力（日本語を含むすべてのUnicode文字）
        // IME確定時の文字を先に処理
        bool hasImeInput = (io.InputQueueCharacters.Size > 0);
        for (int i = 0; i < io.InputQueueCharacters.Size; ++i) {
            unsigned int c = io.InputQueueCharacters[i];
            if (c > 0) {
                onCharInput(c);
            }
        }

        // 特殊キー
        static const ImGuiKey specialKeys[] = {
            ImGuiKey_Enter, ImGuiKey_Tab, ImGuiKey_Backspace, ImGuiKey_Escape,
            ImGuiKey_UpArrow, ImGuiKey_DownArrow, ImGuiKey_LeftArrow, ImGuiKey_RightArrow,
            ImGuiKey_Insert, ImGuiKey_Delete, ImGuiKey_Home, ImGuiKey_End,
            ImGuiKey_PageUp, ImGuiKey_PageDown,
            ImGuiKey_F1, ImGuiKey_F2, ImGuiKey_F3, ImGuiKey_F4,
            ImGuiKey_F5, ImGuiKey_F6, ImGuiKey_F7, ImGuiKey_F8,
            ImGuiKey_F9, ImGuiKey_F10, ImGuiKey_F11, ImGuiKey_F12,
        };

        for (ImGuiKey key : specialKeys) {
            if (ImGui::IsKeyPressed(key)) {
                // IME確定時のEnterキーは無視（文字入力として処理済み）
                if (key == ImGuiKey_Enter && hasImeInput) {
                    continue;
                }
                onKeyInput(key, io.KeyCtrl, io.KeyShift, io.KeyAlt);
            }
        }
    }

    // フォルダツリーからのドラッグ＆ドロップ
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PBTERM_PATH")) {
            if (payload->Data && payload->DataSize > 0) {
                const char* path = static_cast<const char*>(payload->Data);
                sendToConnection(path, std::strlen(path));
            }
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::EndChild();
    ImGui::PopFont();
}

void Terminal::updateScreen() {
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            VTermPos pos = {row, col};
            VTermScreenCell cell;
            vterm_screen_get_cell(m_screen, pos, &cell);

            TerminalCell& tcell = m_cells[row][col];

            uint32_t firstChar = cell.chars[0];

            if (firstChar == 0) {
                tcell.width = 1;
                tcell.text.clear();
                continue;  // 空セルはスキップ
            } else if (cell.width == 0) {
                // 幅0は継続セル（全角文字の2バイト目など）
                tcell.width = 0;
                tcell.text.clear();
                continue;  // 継続セルはスキップ
            } else {
                // libvtermの幅をそのまま使用（カーソル位置の整合性のため）
                tcell.width = cell.width;
            }

            // テキスト構築（cell.chars[0]のみ使用 - 合成文字は無視）
            tcell.text.clear();
            {
                char utf8[4];
                int len = 0;
                uint32_t c = firstChar;

                if (c < 0x80) {
                    utf8[0] = static_cast<char>(c);
                    len = 1;
                } else if (c < 0x800) {
                    utf8[0] = static_cast<char>(0xC0 | (c >> 6));
                    utf8[1] = static_cast<char>(0x80 | (c & 0x3F));
                    len = 2;
                } else if (c < 0x10000) {
                    utf8[0] = static_cast<char>(0xE0 | (c >> 12));
                    utf8[1] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                    utf8[2] = static_cast<char>(0x80 | (c & 0x3F));
                    len = 3;
                } else {
                    utf8[0] = static_cast<char>(0xF0 | (c >> 18));
                    utf8[1] = static_cast<char>(0x80 | ((c >> 12) & 0x3F));
                    utf8[2] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                    utf8[3] = static_cast<char>(0x80 | (c & 0x3F));
                    len = 4;
                }
                tcell.text.append(utf8, len);
            }

            // 属性
            tcell.fg = vtermColorToTerminalColor(cell.fg);
            tcell.bg = vtermColorToTerminalColor(cell.bg);
            tcell.bold = (cell.attrs.bold != 0);
            tcell.italic = (cell.attrs.italic != 0);
            tcell.underline = (cell.attrs.underline != 0);
            tcell.reverse = (cell.attrs.reverse != 0);
        }
    }
}

TerminalColor Terminal::vtermColorToTerminalColor(VTermColor color) {
    if (VTERM_COLOR_IS_RGB(&color)) {
        return TerminalColor(color.rgb.red, color.rgb.green, color.rgb.blue);
    } else if (VTERM_COLOR_IS_INDEXED(&color)) {
        int idx = color.indexed.idx;

        // ANSI 16色はテーマから取得
        if (idx < 16) {
            return m_colorTheme.colors[idx];
        } else if (idx < 232) {
            // 216色キューブ
            idx -= 16;
            int r = (idx / 36) * 51;
            int g = ((idx / 6) % 6) * 51;
            int b = (idx % 6) * 51;
            return TerminalColor(r, g, b);
        } else {
            // グレースケール
            int gray = (idx - 232) * 10 + 8;
            return TerminalColor(gray, gray, gray);
        }
    }

    // デフォルトはテーマの前景色
    return m_colorTheme.foreground;
}

void Terminal::setColorTheme(const std::string& themeId) {
    const TerminalColorTheme* theme = getThemeById(themeId);
    if (theme) {
        m_colorTheme = *theme;
        // 画面を更新
        std::lock_guard<std::mutex> lock(m_mutex);
        updateScreen();
    }
}

// libvtermコールバック実装
int Terminal::onDamage(VTermRect rect, void* user) {
    (void)rect;
    (void)user;
    return 0;
}

int Terminal::onMoveCursor(VTermPos pos, VTermPos oldpos, int visible, void* user) {
    (void)oldpos;
    (void)visible;  // カーソル可視状態はonSetTermPropで管理
    Terminal* term = static_cast<Terminal*>(user);
    term->m_cursorRow = pos.row;
    term->m_cursorCol = pos.col;
    return 0;
}

int Terminal::onSetTermProp(VTermProp prop, VTermValue* val, void* user) {
    Terminal* term = static_cast<Terminal*>(user);

    switch (prop) {
        case VTERM_PROP_CURSORVISIBLE:
            // カーソルの表示/非表示（DECTCEM: ?25h / ?25l）
            term->m_cursorVisible = val->boolean;
            break;
        default:
            break;
    }
    return 0;
}

int Terminal::onBell(void* user) {
    (void)user;
    return 0;
}

int Terminal::onResize(int rows, int cols, void* user) {
    (void)rows;
    (void)cols;
    (void)user;
    return 0;
}

int Terminal::onSbPushline(int cols, const VTermScreenCell* cells, void* user) {
    Terminal* term = static_cast<Terminal*>(user);

    // リサイズ中はスクロールバックへのプッシュをスキップ
    if (term->m_resizing) {
        return 1;  // 1を返してlibvtermに処理完了を通知
    }

    // リサイズ後の抑制期間中もスキップ
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - term->m_lastResizeTime).count();
    if (elapsed < RESIZE_SUPPRESS_MS) {
        return 1;
    }

    // スクロールバック行を保存
    std::vector<TerminalCell> row;
    row.resize(cols);

    int skipNext = 0;  // 全角文字の次のセルをスキップするためのカウンタ

    for (int col = 0; col < cols; ++col) {
        TerminalCell& tcell = row[col];

        // 前の全角文字の継続セル
        if (skipNext > 0) {
            tcell.width = 0;
            tcell.text.clear();
            skipNext--;
            continue;
        }

        const VTermScreenCell& cell = cells[col];
        uint32_t firstChar = cell.chars[0];

        // 空セル
        if (firstChar == 0) {
            tcell.width = 1;
            tcell.text.clear();
            continue;
        }

        // libvtermが継続セルとして返した場合
        if (cell.width == 0) {
            tcell.width = 0;
            tcell.text.clear();
            continue;
        }

        // libvtermの幅をそのまま使用
        tcell.width = cell.width;

        // 全角文字（幅2以上）なら次のセルをスキップ
        if (tcell.width >= 2 && col + 1 < cols) {
            skipNext = tcell.width - 1;
        }

        // テキスト構築（UTF-8エンコード）
        tcell.text.clear();
        {
            char utf8[4];
            int len = 0;
            uint32_t c = firstChar;

            if (c < 0x80) {
                utf8[0] = static_cast<char>(c);
                len = 1;
            } else if (c < 0x800) {
                utf8[0] = static_cast<char>(0xC0 | (c >> 6));
                utf8[1] = static_cast<char>(0x80 | (c & 0x3F));
                len = 2;
            } else if (c < 0x10000) {
                utf8[0] = static_cast<char>(0xE0 | (c >> 12));
                utf8[1] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                utf8[2] = static_cast<char>(0x80 | (c & 0x3F));
                len = 3;
            } else {
                utf8[0] = static_cast<char>(0xF0 | (c >> 18));
                utf8[1] = static_cast<char>(0x80 | ((c >> 12) & 0x3F));
                utf8[2] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                utf8[3] = static_cast<char>(0x80 | (c & 0x3F));
                len = 4;
            }
            tcell.text.append(utf8, len);
        }

        // 属性
        tcell.fg = term->vtermColorToTerminalColor(cell.fg);
        tcell.bg = term->vtermColorToTerminalColor(cell.bg);
        tcell.bold = (cell.attrs.bold != 0);
        tcell.underline = (cell.attrs.underline != 0);
        tcell.reverse = (cell.attrs.reverse != 0);
    }

    term->m_scrollback.push_back(std::move(row));

    // 最大行数を超えたら古い行を削除
    if (term->m_scrollback.size() > MAX_SCROLLBACK) {
        term->m_scrollback.erase(term->m_scrollback.begin());
    }

    term->m_autoScroll = true;
    return 0;
}

int Terminal::onSbPopline(int cols, VTermScreenCell* cells, void* user) {
    (void)cols;
    (void)cells;
    (void)user;
    return 0;
}

int Terminal::onOsc(int command, VTermStringFragment frag, void* user) {
    Terminal* term = static_cast<Terminal*>(user);

    // OSC 7: カレントディレクトリ通知
    if (command == 7 && frag.str && frag.len > 0) {
        std::string path(frag.str, frag.len);
        // file://hostname/path 形式からパスを抽出
        size_t pos = path.find("//");
        if (pos != std::string::npos) {
            pos = path.find('/', pos + 2);
            if (pos != std::string::npos) {
                term->m_currentDirectory = path.substr(pos);
            }
        }
    }

    return 0;
}

std::string Terminal::getSelectedText() const {
    if (!m_hasSelection) return "";

    // 選択範囲を正規化
    int startRow = m_selStartRow, startCol = m_selStartCol;
    int endRow = m_selEndRow, endCol = m_selEndCol;
    if (startRow > endRow || (startRow == endRow && startCol > endCol)) {
        std::swap(startRow, endRow);
        std::swap(startCol, endCol);
    }

    std::string result;
    int scrollbackRows = static_cast<int>(m_scrollback.size());

    for (int row = startRow; row <= endRow; ++row) {
        int colStart = (row == startRow) ? startCol : 0;
        int colEnd = (row == endRow) ? endCol : m_cols - 1;

        // スクロールバックか現在の画面かを判定
        int actualRow = row + scrollbackRows;
        if (actualRow < 0) {
            // スクロールバック範囲外
            continue;
        } else if (actualRow < scrollbackRows) {
            // スクロールバック行
            const auto& sbRow = m_scrollback[actualRow];
            for (int col = colStart; col <= colEnd && col < static_cast<int>(sbRow.size()); ++col) {
                if (sbRow[col].width > 0) {
                    result += sbRow[col].text;
                }
            }
        } else {
            // 現在の画面
            int screenRow = row;
            if (screenRow >= 0 && screenRow < m_rows) {
                for (int col = colStart; col <= colEnd && col < m_cols; ++col) {
                    if (m_cells[screenRow][col].width > 0) {
                        result += m_cells[screenRow][col].text;
                    }
                }
            }
        }

        // 行末に改行を追加（最終行以外）
        if (row < endRow) {
            result += '\n';
        }
    }

    return result;
}

void Terminal::clearSelection() {
    m_selecting = false;
    m_hasSelection = false;
    m_selStartCol = 0;
    m_selStartRow = 0;
    m_selEndCol = 0;
    m_selEndRow = 0;
}

} // namespace pbterm
