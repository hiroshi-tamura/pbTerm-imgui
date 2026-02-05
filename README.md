# pbTerm

Dear ImGui ベースの SSH ターミナルクライアント for macOS

## 概要

pbTerm は、軽量で高速な SSH ターミナルクライアントです。Dear ImGui による即時モード GUI を採用し、スムーズな操作感を実現しています。

## 主な機能

### SSH 接続
- **公開鍵認証 / パスワード認証** に対応
- **接続プロファイル** の保存・管理
- **自動接続** オプション

### ターミナル
- **VT100/VT220 互換** の完全なターミナルエミュレーション（libvterm）
- **日本語（全角文字）** 対応
- **スクロールバック** 履歴
- **マウス選択 & コピー**

### tmux 統合
- SSH 接続時に **tmux セッション** を自動作成/アタッチ
- **マルチウィンドウ** をタブで管理
- セッションの永続化（接続が切れても復帰可能）

### コマンドショートカット (CommandDock)
- よく使うコマンドを **ワンクリック実行**
- **グループ分け** で整理
- **ドラッグ＆ドロップ** で並び替え
- JSON ファイルで永続化

### リモートフォルダツリー (FolderTreeDock)
- リモートのファイルシステムを **ツリー表示**
- **Unix / Windows** 両対応
- フォルダの **新規作成・削除**
- クリックでそのディレクトリに **cd**

### 設定
- **日本語 / 英語** UI 切り替え
- **フォント** 選択・サイズ変更
- **ウィンドウサイズ・位置** の保存

## 技術スタック

| 分類 | 技術 |
|------|------|
| GUI | Dear ImGui + GLFW + OpenGL 3.3 |
| SSH | libssh |
| ターミナルエミュレーション | libvterm |
| ビルドシステム | CMake 3.20+ |
| 言語 | C++17 |
| ターゲット | macOS 12.0+ |

## ビルド方法

### 必要な依存関係

```bash
brew install glfw libssh libvterm
```

### ビルド

```bash
git clone --recursive https://github.com/hiroshi-tamura/pbTerm-imgui.git
cd pbTerm-imgui
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### 実行

```bash
open pbTerm.app
```

または

```bash
./pbTerm.app/Contents/MacOS/pbTerm
```

## 設定ファイルの場所

設定ファイルは `~/.config/pbterm/` に保存されます：

| ファイル | 内容 |
|----------|------|
| `settings.json` | アプリ設定（言語、フォント、ウィンドウサイズ） |
| `profiles.json` | SSH 接続プロファイル |
| `commands.json` | コマンドショートカット |
| `imgui.ini` | ImGui レイアウト |

## プロジェクト構成

```
pbTerm-imgui/
├── include/              # ヘッダファイル
│   ├── App.h             # メインアプリケーション
│   ├── Terminal.h        # ターミナルエミュレータ
│   ├── SshConnection.h   # SSH 接続管理
│   ├── TerminalDock.h    # ターミナル UI
│   ├── TmuxController.h  # tmux セッション管理
│   ├── CommandDock.h     # コマンドショートカット UI
│   ├── FolderTreeDock.h  # フォルダツリー UI
│   ├── SettingsDialog.h  # 設定ダイアログ
│   ├── ProfileManager.h  # プロファイル管理
│   └── ConnectionDialog.h # 接続ダイアログ
├── src/                  # ソースファイル
├── external/imgui/       # Dear ImGui (サブモジュール)
├── resources/
│   ├── fonts/            # HackGen, Material Icons
│   └── icon/             # アプリアイコン
└── CMakeLists.txt
```

## ライセンス

MIT License

## 作者

hiroshi-tamura
