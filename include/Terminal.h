#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <vterm.h>
#include "imgui.h"

namespace pbterm {

class SshConnection;
class SshChannel;

// ANSIカラー定義
struct TerminalColor {
    uint8_t r, g, b, a;

    TerminalColor() : r(255), g(255), b(255), a(255) {}
    TerminalColor(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}

    ImU32 toImU32() const {
        return IM_COL32(r, g, b, a);
    }
};

// ターミナルセル
struct TerminalCell {
    std::string text;
    TerminalColor fg;
    TerminalColor bg;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool reverse = false;
    int width = 1;  // セル幅（全角=2, 半角=1）
};

// ターミナルエミュレータ（libvterm使用）
class Terminal {
public:
    Terminal(int cols = 80, int rows = 24);
    ~Terminal();

    // SSHチャンネルを設定
    void setChannel(std::shared_ptr<SshChannel> channel);

    // 後方互換性（旧API）
    void setConnection(SshConnection* connection);

    // データ受信（SSH経由）
    void onData(const char* data, size_t len);

    // キー入力
    void onKeyInput(ImGuiKey key, bool ctrl, bool shift, bool alt);
    void onCharInput(unsigned int c);

    // サイズ変更
    void resize(int cols, int rows);

    // 描画
    void render(ImFont* font);

    // サイズ取得
    int cols() const { return m_cols; }
    int rows() const { return m_rows; }

    // カレントディレクトリ（OSC経由で取得）
    std::string currentDirectory() const { return m_currentDirectory; }

    // libvtermコールバック（publicにする必要がある）
    static int onDamage(VTermRect rect, void* user);
    static int onMoveCursor(VTermPos pos, VTermPos oldpos, int visible, void* user);
    static int onSetTermProp(VTermProp prop, VTermValue* val, void* user);
    static int onBell(void* user);
    static int onResize(int rows, int cols, void* user);
    static int onSbPushline(int cols, const VTermScreenCell* cells, void* user);
    static int onSbPopline(int cols, VTermScreenCell* cells, void* user);
    static int onOsc(int command, VTermStringFragment frag, void* user);

    // 出力コールバック用（vterm経由でSSHに送信）
    void sendToConnection(const char* data, size_t len);

private:
    // 内部ヘルパー
    void updateScreen();
    TerminalColor vtermColorToTerminalColor(VTermColor color);

    VTerm* m_vterm = nullptr;
    VTermScreen* m_screen = nullptr;
    std::shared_ptr<SshChannel> m_channel;
    SshConnection* m_connection = nullptr;  // 後方互換性用

    int m_cols;
    int m_rows;
    int m_cursorCol = 0;
    int m_cursorRow = 0;
    bool m_cursorVisible = true;

    std::vector<std::vector<TerminalCell>> m_cells;
    std::mutex m_mutex;

    std::string m_currentDirectory;

    // スクロールバック
    std::vector<std::vector<TerminalCell>> m_scrollback;
    static constexpr int MAX_SCROLLBACK = 10000;
    bool m_autoScroll = false;

    // リサイズ中フラグ（リサイズ中はスクロールバックへのプッシュを抑制）
    bool m_resizing = false;

    // マウス選択
    bool m_selecting = false;
    bool m_hasSelection = false;
    int m_selStartCol = 0;
    int m_selStartRow = 0;
    int m_selEndCol = 0;
    int m_selEndRow = 0;

    // 選択範囲のテキスト取得
    std::string getSelectedText() const;
    void clearSelection();
};

} // namespace pbterm
