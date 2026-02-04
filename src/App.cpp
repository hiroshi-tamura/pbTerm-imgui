#include "App.h"
#include "TerminalDock.h"
#include "ConnectionDialog.h"
#include "ProfileManager.h"
#include "SshConnection.h"
#include "TmuxController.h"
#include "SettingsDialog.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <libgen.h>
#endif

namespace pbterm {

App::App() = default;

App::~App() = default;

bool App::init() {
    // アプリケーション設定読み込み（ウィンドウ作成前に必要）
    m_appSettings.load();

    // GLFW初期化
    if (!glfwInit()) {
        std::cerr << "GLFW init failed" << std::endl;
        return false;
    }

    // OpenGL設定
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    // 保存されたウィンドウサイズを使用
    m_windowWidth = m_appSettings.windowWidth;
    m_windowHeight = m_appSettings.windowHeight;

    // ウィンドウ作成
    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "pbTerm", nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Window creation failed" << std::endl;
        glfwTerminate();
        return false;
    }

    // 保存されたウィンドウ位置を復元（-1の場合は中央配置）
    if (m_appSettings.windowX >= 0 && m_appSettings.windowY >= 0) {
        glfwSetWindowPos(m_window, m_appSettings.windowX, m_appSettings.windowY);
    }

    // 最大化状態を復元
    if (m_appSettings.windowMaximized) {
        glfwMaximizeWindow(m_window);
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1);

    // フレームバッファサイズ変更コールバックを設定
    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferSizeCallback);

    // Dear ImGui初期化
    setupImGui();

    // コンポーネント初期化
    m_profileManager = std::make_unique<ProfileManager>();
    m_profileManager->load();

    m_sshConnection = std::make_unique<SshConnection>();
    m_tmuxController = std::make_unique<TmuxController>();
    m_tmuxController->setConnection(m_sshConnection.get());

    m_terminalDock = std::make_unique<TerminalDock>();
    m_terminalDock->setConnection(m_sshConnection.get());
    m_terminalDock->setTmuxController(m_tmuxController.get());

    m_connectionDialog = std::make_unique<ConnectionDialog>(m_profileManager.get());
    m_connectionDialog->setConnectCallback([this](const SshConfig& config) {
        if (m_sshConnection->connect(config)) {
            // tmuxセッションにアタッチ
            m_tmuxController->setConnection(m_sshConnection.get());
            if (m_tmuxController->startOrAttachSession()) {
                std::cout << "tmuxセッションにアタッチしました" << std::endl;
            }
            m_connected = true;
            m_terminalDock->onConnected();
            m_showConnectionDialog = false;
        }
    });

    // 設定ダイアログ初期化
    m_settingsDialog = std::make_unique<SettingsDialog>();
    m_settingsDialog->setSettings(m_appSettings);
    m_settingsDialog->setApplyCallback([this](const AppSettings& settings) {
        onSettingsApplied(settings);
    });

    // オート接続
    const Profile* autoProfile = m_profileManager->autoConnectProfile();
    if (autoProfile) {
        if (m_sshConnection->connect(autoProfile->config)) {
            // tmuxセッションにアタッチ
            m_tmuxController->setConnection(m_sshConnection.get());
            if (m_tmuxController->startOrAttachSession()) {
                std::cout << "tmuxセッションにアタッチしました（オート接続）" << std::endl;
            }
            m_connected = true;
            m_terminalDock->onConnected();
        }
    }

    return true;
}

void App::setupImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // ImGuiのiniファイルを設定ディレクトリに保存
    m_imguiIniPath = AppSettings::configDir() + "/imgui.ini";

    // 設定ディレクトリが存在しなければ作成
    std::filesystem::path configDir = AppSettings::configDir();
    if (!std::filesystem::exists(configDir)) {
        std::filesystem::create_directories(configDir);
    }

    // IniFilenameは文字列のポインタを保持する必要があるため、メンバ変数を使用
    io.IniFilename = m_imguiIniPath.c_str();

    // ダークテーマ
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.TabRounding = 4.0f;

    // バックエンド初期化
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // フォント読み込み
    loadFont();
}

void App::loadFont() {
    ImGuiIO& io = ImGui::GetIO();

    // 設定からフォントパスを取得
    std::string fontPath = m_appSettings.fontPath;
    std::string fontFile = "HackGenConsole-Regular.ttf";

    // フォントが見つからない場合のフォールバック
    if (!std::filesystem::exists(fontPath)) {
        fontPath = "../" + m_appSettings.fontPath;
    }

    if (!std::filesystem::exists(fontPath)) {
        fontPath = "resources/fonts/" + fontFile;
    }

    if (!std::filesystem::exists(fontPath)) {
        fontPath = "../resources/fonts/" + fontFile;
    }

#ifdef __APPLE__
    // macOS: .appバンドル内のリソースを探す
    if (!std::filesystem::exists(fontPath)) {
        char execPath[PATH_MAX];
        uint32_t size = sizeof(execPath);
        if (_NSGetExecutablePath(execPath, &size) == 0) {
            std::string bundlePath = dirname(dirname(execPath));
            fontPath = bundlePath + "/Resources/resources/fonts/" + fontFile;
        }
    }
#endif

    std::cout << "Trying font path: " << fontPath << " exists: " << std::filesystem::exists(fontPath) << std::endl;

    if (std::filesystem::exists(fontPath)) {
        ImFontConfig config;
        config.OversampleH = 1;
        config.OversampleV = 1;
        config.PixelSnapH = true;

        // フォントアトラス設定を最適化
        io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;

        std::cout << "Loading font with GetGlyphRangesJapanese()" << std::endl;

        // ImGuiの日本語グリフ範囲を使用
        m_font = io.Fonts->AddFontFromFileTTF(
            fontPath.c_str(),
            m_appSettings.fontSize,
            &config,
            io.Fonts->GetGlyphRangesJapanese()
        );

        if (m_font) {
            std::cout << "Font loaded: " << fontPath << " (size: " << m_appSettings.fontSize << ")" << std::endl;
        } else {
            std::cerr << "Font load FAILED: " << fontPath << std::endl;
        }
    } else {
        std::cerr << "Font file not found: " << fontPath << std::endl;
    }

    if (!m_font) {
        std::cerr << "Font load failed, using default font" << std::endl;
        m_font = io.Fonts->AddFontDefault();
    }

    // フォントアトラス構築
    io.Fonts->Build();
}

void App::reloadFont() {
    ImGuiIO& io = ImGui::GetIO();

    // フォントアトラスをクリア
    io.Fonts->Clear();
    m_font = nullptr;

    // フォントを再読み込み
    loadFont();

    // OpenGLデバイスオブジェクトを再構築
    ImGui_ImplOpenGL3_DestroyDeviceObjects();
    ImGui_ImplOpenGL3_CreateDeviceObjects();
}

void App::onSettingsApplied(const AppSettings& settings) {
    bool needReload = (m_appSettings.fontPath != settings.fontPath ||
                       m_appSettings.fontSize != settings.fontSize);

    m_appSettings = settings;

    if (needReload) {
        m_needFontReload = true;
    }
}

void App::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        // フォント再読み込み（フレーム間で行う）
        if (m_needFontReload) {
            m_needFontReload = false;
            reloadFont();
        }

        // 新しいフレーム開始
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Docking設定
        setupDocking();

        // UI描画
        renderUI();

        // レンダリング
        ImGui::Render();
        int displayW, displayH;
        glfwGetFramebufferSize(m_window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
}

void App::setupDocking() {
    // フルスクリーンドッキングスペース
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    // メニューバー
    renderMenuBar();

    // ドッキングスペース
    ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");

    // imgui.iniが存在しない場合のみ初回レイアウトを構築
    static bool firstTime = true;
    if (firstTime) {
        firstTime = false;

        // 既存のドッキングノードがない場合のみ初期化
        if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

            // Terminalウィンドウをドッキングスペースにドッキング
            ImGui::DockBuilderDockWindow("Terminal", dockspaceId);

            ImGui::DockBuilderFinish(dockspaceId);
        }
    }

    // ドッキングスペースを作成（サイズ0,0で自動フィット）
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    ImGui::End();
}

void App::renderMenuBar() {
    const Localization& loc = getLocalization(m_appSettings.language);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu(loc.menuConnect)) {
            if (ImGui::MenuItem(loc.menuStart, nullptr, false, !m_connected)) {
                connect();
            }
            if (ImGui::MenuItem(loc.menuStop, nullptr, false, m_connected)) {
                disconnect();
            }
            ImGui::Separator();
            if (ImGui::MenuItem(loc.menuSetting)) {
                m_connectionDialog->initForShow();
                m_showConnectionDialog = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(loc.menuView)) {
            ImGui::MenuItem(loc.menuTerminal, nullptr, &m_showTerminal);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu(loc.menuOption)) {
            if (ImGui::MenuItem(loc.menuSetting)) {
                m_showSettings = true;
            }
            ImGui::EndMenu();
        }

        // 接続状態表示
        ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 120);
        if (m_connected) {
            ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", loc.statusConnected);
        } else {
            ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1.0f), "%s", loc.statusDisconnected);
        }

        ImGui::EndMenuBar();
    }
}

void App::renderUI() {
    const Localization& loc = getLocalization(m_appSettings.language);

    // ターミナルドック（###でIDを固定、表示名は言語に応じて変更）
    if (m_showTerminal) {
        char title[128];
        snprintf(title, sizeof(title), "%s###Terminal", loc.menuTerminal);
        ImGui::Begin(title, &m_showTerminal);
        m_terminalDock->setLanguage(m_appSettings.language);
        m_terminalDock->render(m_font);
        ImGui::End();
    }

    // 接続設定ダイアログ
    if (m_showConnectionDialog) {
        m_connectionDialog->setLanguage(m_appSettings.language);
        m_connectionDialog->render(&m_showConnectionDialog);
    }

    // オプション設定ダイアログ
    if (m_showSettings) {
        m_settingsDialog->render(&m_showSettings);
    }
}

void App::connect() {
    // 最後に使用したプロファイルで接続、なければダイアログ表示
    if (m_profileManager->profileCount() > 0) {
        const Profile* profile = m_profileManager->getProfile(0);
        if (profile && m_sshConnection->connect(profile->config)) {
            // tmuxセッションにアタッチ
            m_tmuxController->setConnection(m_sshConnection.get());
            if (m_tmuxController->startOrAttachSession()) {
                std::cout << "tmuxセッションにアタッチしました" << std::endl;
            }
            m_connected = true;
            m_terminalDock->onConnected();
            return;
        }
    }
    m_connectionDialog->initForShow();
    m_showConnectionDialog = true;
}

void App::disconnect() {
    // tmuxからデタッチ（セッションは維持）
    m_tmuxController->detach();
    m_tmuxController->closeControlChannel();
    m_sshConnection->disconnect();
    m_connected = false;
    m_terminalDock->onDisconnected();
}

void App::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    App* app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app) {
        app->onFramebufferResize(width, height);
    }
}

void App::onFramebufferResize(int width, int height) {
    // フレームバッファサイズが変更された時に即座にビューポートを更新
    glViewport(0, 0, width, height);
}

void App::shutdown() {
    // ウィンドウの位置・サイズを保存
    if (m_window) {
        // 最大化状態を確認
        m_appSettings.windowMaximized = (glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED) != 0);

        // 最大化されていない場合のみ位置・サイズを保存
        if (!m_appSettings.windowMaximized) {
            glfwGetWindowPos(m_window, &m_appSettings.windowX, &m_appSettings.windowY);
            glfwGetWindowSize(m_window, &m_appSettings.windowWidth, &m_appSettings.windowHeight);
        }

        m_appSettings.save();
    }

    m_terminalDock.reset();
    m_connectionDialog.reset();
    m_settingsDialog.reset();
    m_tmuxController.reset();
    m_sshConnection.reset();
    m_profileManager->save();
    m_profileManager.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

} // namespace pbterm
