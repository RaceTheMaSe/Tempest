#pragma once

#ifdef __ANDROID__
#include "system/systemapi.h"
#include <android_native_app_glue.h>
#include <android/input.h>
#include <android/log.h>
#include <android/looper.h>
#include <android/window.h>
#include <thread>
#include <ui/event.h>

extern android_app* globalAppPtr;

namespace Tempest {

class AndroidApi : public SystemApi {
  public:
    AndroidApi();
    ~AndroidApi();

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
    bool     implIsPaused()  override;
    bool     implIsResumeRequested() override;
    void     implRegisterApplication  (Application *p) override;
    void     implUnregisterApplication(Application *p) override;
    void     implShutdown() override;

  private:
    void onAppCmd(struct android_app* app, int32_t cmd);
    int  onInputEvent(struct android_app* app, AInputEvent* event);
    void inputHandling();
    void saveAppState();
    void loadSavedState();
    void render();
    void processEvents();
    android_app* m_App;
    static ANativeWindow* m_Window;
    static std::atomic_bool m_HasFocus;
    static std::atomic_bool isExit;
    static std::thread m_renderThread;
    static std::thread m_eventThread;
    static std::atomic_bool shouldProcess;
    static std::mutex m_mutex;
    static int32_t width;
    static int32_t height;
    static std::vector<Application*> m_Apps;

    bool mouseInputActive = false;
    int  mouseButtonStatePrevious;
    void dispatchMouseButton(int x, int y, int dir, const Tempest::Event::MouseButton& button);

    float axis_values[Event::GamepadAxisType::AxisLast] = { 0 };
    float deadzone = 0.22f;
    float deadzone_look = 0.15f;
    Vec2 RightStickPrevious;
  };
}
#endif
