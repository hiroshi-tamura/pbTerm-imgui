cmake_minimum_required(VERSION 3.20)
project(pbTerm VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# macOS設定
if(APPLE)
    # Homebrewライブラリと同じデプロイメントターゲットを使用
    set(CMAKE_OSX_DEPLOYMENT_TARGET "26.0")
endif()

# Homebrew パス
set(HOMEBREW_PREFIX "/opt/homebrew")

# 依存関係を探す
find_package(OpenGL REQUIRED)

# GLFW
find_library(GLFW_LIBRARY glfw PATHS ${HOMEBREW_PREFIX}/lib REQUIRED)
set(GLFW_INCLUDE_DIR ${HOMEBREW_PREFIX}/include)

# libssh
find_library(LIBSSH_LIBRARY ssh PATHS ${HOMEBREW_PREFIX}/lib REQUIRED)
set(LIBSSH_INCLUDE_DIR ${HOMEBREW_PREFIX}/include)

# libvterm
find_library(LIBVTERM_LIBRARY vterm PATHS ${HOMEBREW_PREFIX}/lib REQUIRED)
set(LIBVTERM_INCLUDE_DIR ${HOMEBREW_PREFIX}/include)

# Dear ImGui ソース
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# アプリケーションソース
set(APP_SOURCES
    src/main.cpp
    src/App.cpp
    src/SshConnection.cpp
    src/Terminal.cpp
    src/TerminalDock.cpp
    src/ConnectionDialog.cpp
    src/ProfileManager.cpp
    src/SettingsDialog.cpp
    src/TmuxController.cpp
    src/CommandDock.cpp
    src/FolderTreeDock.cpp
)

# macOS .appバンドル
if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_NAME "pbTerm")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "com.pbterm.imgui")
    set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
    set(MACOSX_BUNDLE_ICON_FILE "terminal.icns")

    add_executable(pbTerm MACOSX_BUNDLE ${APP_SOURCES} ${IMGUI_SOURCES})

    # リソースをバンドル内にコピー
    set_target_properties(pbTerm PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/Info.plist.in
    )
else()
    add_executable(pbTerm ${APP_SOURCES} ${IMGUI_SOURCES})
endif()

target_include_directories(pbTerm PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${GLFW_INCLUDE_DIR}
    ${LIBSSH_INCLUDE_DIR}
    ${LIBVTERM_INCLUDE_DIR}
    ${OPENGL_INCLUDE_DIR}
)

target_link_libraries(pbTerm PRIVATE
    ${GLFW_LIBRARY}
    ${LIBSSH_LIBRARY}
    ${LIBVTERM_LIBRARY}
    ${OPENGL_LIBRARIES}
)

# macOS フレームワーク
if(APPLE)
    target_link_libraries(pbTerm PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

# リソースをコピー
if(APPLE)
    # .appバンドル内のResourcesにコピー
    add_custom_command(TARGET pbTerm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_BUNDLE_DIR:pbTerm>/Contents/Resources/resources
        # アイコンをResourcesルートにもコピー
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/resources/icon/pbTerm.icns
        $<TARGET_BUNDLE_DIR:pbTerm>/Contents/Resources/pbTerm.icns
    )
else()
    add_custom_command(TARGET pbTerm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/resources
        $<TARGET_FILE_DIR:pbTerm>/resources
    )
endif()

# コンパイラ警告設定
target_compile_options(pbTerm PRIVATE
    -Wall -Wextra -Wpedantic
    -Wno-unused-parameter
    -Wno-missing-field-initializers
)

# macOS OpenGL非推奨警告を抑制
if(APPLE)
    target_compile_definitions(pbTerm PRIVATE GL_SILENCE_DEPRECATION)
endif()
