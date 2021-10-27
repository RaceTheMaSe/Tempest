#pragma once

#include "system/systemapi.h"

namespace Tempest {

class X11Api : public SystemApi {
  public:
    X11Api();

    static void* display();

  protected:
    Window*  implCreateWindow(Tempest::Window *owner, uint32_t width, uint32_t height, const char* title) override;
    Window*  implCreateWindow(Tempest::Window *owner, ShowMode sm, const char* title) override;
    void     implDestroyWindow(Window* w) override;
    void     implExit() override;

    Rect     implWindowClientRect(SystemApi::Window *w) override;

    bool     implSetAsFullscreen(SystemApi::Window *w, bool fullScreen) override;
    bool     implIsFullscreen(SystemApi::Window *w) override;

    void     implSetCursorPosition(SystemApi::Window *w, int x, int y) override;
    void     implShowCursor(SystemApi::Window *w, CursorShape show) override;

    int      implExec(AppCallBack& cb) override;
    void     implProcessEvents(AppCallBack& cb) override;
    bool     implIsRunning() override;
    void     implShutdown() override {};

  private:
    void     alignGeometry(Window *w, Tempest::Window& owner);
  };
}
