#include "DragDropHelper.h"
#include <atomic>
#include <objc/runtime.h>

#import <Cocoa/Cocoa.h>

namespace pbterm {

static std::atomic<bool> s_externalDragInProgress{false};
static bool s_swizzled = false;

bool isExternalDragInProgress() {
    return s_externalDragInProgress.load();
}

void setExternalDragInProgress(bool inProgress) {
    s_externalDragInProgress.store(inProgress);
}

} // namespace pbterm

// オリジナルのメソッド実装を保存
static IMP s_originalDraggingEntered = nil;
static IMP s_originalDraggingExited = nil;
static IMP s_originalDraggingEnded = nil;
static IMP s_originalPerformDragOperation = nil;

// フック関数
static NSDragOperation swizzled_draggingEntered(id self, SEL _cmd, id<NSDraggingInfo> sender) {
    pbterm::setExternalDragInProgress(true);
    if (s_originalDraggingEntered) {
        return ((NSDragOperation(*)(id, SEL, id<NSDraggingInfo>))s_originalDraggingEntered)(self, _cmd, sender);
    }
    return NSDragOperationCopy;
}

static void swizzled_draggingExited(id self, SEL _cmd, id<NSDraggingInfo> sender) {
    pbterm::setExternalDragInProgress(false);
    if (s_originalDraggingExited) {
        ((void(*)(id, SEL, id<NSDraggingInfo>))s_originalDraggingExited)(self, _cmd, sender);
    }
}

static void swizzled_draggingEnded(id self, SEL _cmd, id<NSDraggingInfo> sender) {
    pbterm::setExternalDragInProgress(false);
    if (s_originalDraggingEnded) {
        ((void(*)(id, SEL, id<NSDraggingInfo>))s_originalDraggingEnded)(self, _cmd, sender);
    }
}

static BOOL swizzled_performDragOperation(id self, SEL _cmd, id<NSDraggingInfo> sender) {
    pbterm::setExternalDragInProgress(false);
    if (s_originalPerformDragOperation) {
        return ((BOOL(*)(id, SEL, id<NSDraggingInfo>))s_originalPerformDragOperation)(self, _cmd, sender);
    }
    return YES;
}

// GLFWのNSViewをswizzleする関数（App初期化時に呼ぶ）
extern "C" void pbterm_setupDragDropHooks(void* nsWindow) {
    if (pbterm::s_swizzled) return;

    NSWindow* window = (__bridge NSWindow*)nsWindow;
    NSView* contentView = [window contentView];
    if (!contentView) return;

    Class viewClass = [contentView class];

    // draggingEntered:
    Method m1 = class_getInstanceMethod(viewClass, @selector(draggingEntered:));
    if (m1) {
        s_originalDraggingEntered = method_setImplementation(m1, (IMP)swizzled_draggingEntered);
    }

    // draggingExited:
    Method m2 = class_getInstanceMethod(viewClass, @selector(draggingExited:));
    if (m2) {
        s_originalDraggingExited = method_setImplementation(m2, (IMP)swizzled_draggingExited);
    }

    // draggingEnded:
    Method m3 = class_getInstanceMethod(viewClass, @selector(draggingEnded:));
    if (m3) {
        s_originalDraggingEnded = method_setImplementation(m3, (IMP)swizzled_draggingEnded);
    }

    // performDragOperation:
    Method m4 = class_getInstanceMethod(viewClass, @selector(performDragOperation:));
    if (m4) {
        s_originalPerformDragOperation = method_setImplementation(m4, (IMP)swizzled_performDragOperation);
    }

    pbterm::s_swizzled = true;
}
