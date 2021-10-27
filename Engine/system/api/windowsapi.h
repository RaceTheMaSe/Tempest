#pragma once

#include "system/systemapi.h"

namespace Tempest {

class WindowsApi final : SystemApi {
  public:

  private:
    WindowsApi();

    Window*  implCreateWindow(Tempest::Window* owner, uint32_t width, uint32_t height, ShowMode sm, const char* title);
    Window*  implCreateWindow(Tempest::Window *owner, uint32_t width, uint32_t height, const char* title) override;
    Window*  implCreateWindow(Tempest::Window *owner, ShowMode sm, const char* title) override;
    void     implDestroyWindow(Window* w) override;
    void     implExit() override;

    Rect     implWindowClientRect(SystemApi::Window *w) override;
    bool     implSetAsFullscreen(SystemApi::Window *w, bool fullScreen) override;
    bool     implIsFullscreen(SystemApi::Window *w) override;

    void     implSetCursorPosition(SystemApi::Window *w, int x, int y) override;
    void     implShowCursor(SystemApi::Window *w, CursorShape show) override;

    bool     implIsRunning() override;
    int      implExec(AppCallBack& cb) override;
    void     implProcessEvents(AppCallBack& cb) override;
    void     implShutdown() override {};

    static long long windowProc(void* hWnd, uint32_t msg, const unsigned long long wParam, const long long lParam);
    static void handleKeyEvent(Tempest::Window* cb, uint32_t msg, const unsigned long long wParam, const long long lParam);

  friend class SystemApi;
  };

}
