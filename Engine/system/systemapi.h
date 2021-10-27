#pragma once

#include <Tempest/Platform>
#include <Tempest/Rect>
#include <Tempest/Event>
#include <Tempest/WidgetState>

#include <memory>
#include <cstdint>

namespace Tempest {

class SizeEvent;
class MouseEvent;
class KeyEvent;
class GamepadKeyEvent;
class CloseEvent;
class PaintEvent;

class Widget;
class Window;
class UiOverlay;
class Application;

class SystemApi {
  public:
    struct Window;

    enum ShowMode : uint8_t {
      Minimized,
      Normal,
      Maximized,
      FullScreen,

      Hidden //internal
      };

    struct TranslateKeyPair final {
      uint16_t src;
      uint16_t result;
      };

    virtual ~SystemApi()=default;
    static Window*  createWindow(Tempest::Window* owner, uint32_t width, uint32_t height, const char* title);
    static Window*  createWindow(Tempest::Window* owner, ShowMode sm, const char* title);
    static void     destroyWindow(Window* w);
    static void     exit();
    static void     shutdown();

    static Rect     windowClientRect(SystemApi::Window *w);

    static bool     setAsFullscreen(SystemApi::Window *w, bool fullScreen);
    static bool     isFullscreen(SystemApi::Window *w);

    static uint16_t translateKey(uint64_t scancode);
    static void     setupKeyTranslate(const TranslateKeyPair k[], uint16_t funcCount);

    static void     addOverlay (UiOverlay* ui);
    static void     takeOverlay(UiOverlay* ui);

    static void     setKeyRepeatDelay(uint64_t delay);
    static uint64_t getKeyRepeatDelay();
    static void     setKeyFirstRepeatDelay(uint64_t delay);
    static uint64_t getKeyFirstRepeatDelay();
    static void     setTickCount(uint64_t t);
    static uint64_t getTickCount();
    static void     clearInput();

  protected:
    struct AppCallBack {
      virtual ~AppCallBack()=default;
      virtual uint32_t onTimer()=0;
      };

    SystemApi();
    virtual Window*  implCreateWindow (Tempest::Window *owner,uint32_t width,uint32_t height, const char* title) = 0;
    virtual Window*  implCreateWindow (Tempest::Window *owner,ShowMode sm, const char* title) = 0;
    virtual void     implDestroyWindow(Window* w) = 0;
    virtual void     implExit() = 0;

    virtual Rect     implWindowClientRect(SystemApi::Window *w) = 0;

    virtual bool     implSetAsFullscreen(SystemApi::Window *w, bool fullScreen) = 0;
    virtual bool     implIsFullscreen(SystemApi::Window *w) = 0;

    virtual void     implSetCursorPosition(SystemApi::Window *w, int x, int y) = 0;
    virtual void     implShowCursor(SystemApi::Window *w, CursorShape show) = 0;

    virtual bool     implIsRunning() = 0;
    virtual bool     implIsPaused() { return false; }
    virtual bool     implIsResumeRequested() { return false; }
    virtual void     implRegisterApplication  (Application*) {}
    virtual void     implUnregisterApplication(Application*) {}
    virtual int      implExec(AppCallBack& cb) = 0;
    virtual void     implProcessEvents(AppCallBack& cb) = 0;
    virtual void     implShutdown() = 0;

    static void      registerApplication  (Application *pApplication);
    static void      unregisterApplication(Application *pApplication);
    static void      setCursorPosition(SystemApi::Window *w, int x, int y);
    static void      showCursor(SystemApi::Window *w, CursorShape c);

    static void      dispatchOverlayRender(Tempest::Window &w, Tempest::PaintEvent& e);
    static void      dispatchRender    (Tempest::Window& cb);
    static void      dispatchMouseDown (Tempest::Window& cb, MouseEvent& e);
    static void      dispatchMouseUp   (Tempest::Window& cb, MouseEvent& e);
    static void      dispatchMouseMove (Tempest::Window& cb, MouseEvent& e);
    static void      dispatchMouseWheel(Tempest::Window& cb, MouseEvent& e);

    static void      dispatchKeyDown   (Tempest::Window& cb, KeyEvent& e, uint32_t scancode);
    static void      dispatchKeyUp     (Tempest::Window& cb, KeyEvent& e, uint32_t scancode);

    static void      dispatchKeyDown   (Tempest::Window& cb, GamepadKeyEvent& e, uint32_t scancode);
    static void      dispatchKeyUp     (Tempest::Window& cb, GamepadKeyEvent& e, uint32_t scancode);
    static void      dispatchGamepadMove(Tempest::Window& cb, AnalogEvent& e);

    static void      dispatchPointerDown(Tempest::Window& cb, PointerEvent& e);
    static void      dispatchPointerUp  (Tempest::Window& cb, PointerEvent& e);
    static void      dispatchPointerMove(Tempest::Window& cb, PointerEvent& e);

    static void      dispatchResize    (Tempest::Window& cb, SizeEvent& e);
    static void      dispatchClose     (Tempest::Window& cb, CloseEvent& e);
    static void      dispatchAppState  (Tempest::Window& cb, AppStateEvent& e);
    static void      dispatchFocus     (Tempest::Window& cb, FocusEvent& e);

  private:
    static bool       isRunning();
    static bool       isPaused();
    static bool       isResumeRequested();
    static int        exec(AppCallBack& cb);
    static void       processEvent(AppCallBack& cb);
    static SystemApi& inst();

    struct Data;
    static Data m;

  friend class Tempest::Window;
  friend class Tempest::Application;
  };

}
