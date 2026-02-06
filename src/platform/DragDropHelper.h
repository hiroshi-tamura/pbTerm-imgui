#pragma once

namespace pbterm {

// 外部からのドラッグ操作が進行中かどうかを取得
bool isExternalDragInProgress();

// ドラッグ状態を設定（内部使用）
void setExternalDragInProgress(bool inProgress);

} // namespace pbterm

// GLFWウィンドウのドラッグイベントをフックする（App初期化時に呼ぶ）
#ifdef __cplusplus
extern "C" {
#endif
void pbterm_setupDragDropHooks(void* nsWindow);
#ifdef __cplusplus
}
#endif
