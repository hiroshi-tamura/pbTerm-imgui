#include "DragDropHelper.h"
#include <atomic>

#import <Cocoa/Cocoa.h>

namespace pbterm {

static std::atomic<bool> s_externalDragInProgress{false};

bool isExternalDragInProgress() {
    // macOSのドラッグ状態を確認
    // NSPasteboardのドラッグタイプをチェック
    NSPasteboard* dragPasteboard = [NSPasteboard pasteboardWithName:NSPasteboardNameDrag];
    if (dragPasteboard) {
        NSArray* types = [dragPasteboard types];
        if (types && [types count] > 0) {
            // ファイルURLがあればドラッグ中と判断
            if ([types containsObject:NSPasteboardTypeFileURL] ||
                [types containsObject:@"public.file-url"]) {
                return true;
            }
        }
    }
    return s_externalDragInProgress.load();
}

void setExternalDragInProgress(bool inProgress) {
    s_externalDragInProgress.store(inProgress);
}

} // namespace pbterm
