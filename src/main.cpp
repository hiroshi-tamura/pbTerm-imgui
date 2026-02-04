#include "App.h"
#include <iostream>
#include <locale.h>

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    // ロケール設定（日本語全角文字の幅計算に必要）
    setlocale(LC_ALL, "");

    pbterm::App app;

    if (!app.init()) {
        std::cerr << "アプリケーションの初期化に失敗しました" << std::endl;
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}
