#pragma once

namespace pbterm {

// 外部からのドラッグ操作が進行中かどうかを取得
bool isExternalDragInProgress();

// ドラッグ状態を設定（内部使用）
void setExternalDragInProgress(bool inProgress);

} // namespace pbterm
