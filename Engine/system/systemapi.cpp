#include "systemapi.h"

#include "api/windowsapi.h"
#include "api/x11api.h"
#include "api/androidapi.h"
#include "api/macosapi.h"
#include "eventdispatcher.h"

#include "exceptions/exception.h"

#include <Tempest/Event>
#include <Tempest/Window>

#include <thread>
#include <unordered_set>
#include <atomic>

using namespace Tempest;

static EventDispatcher dispatcher;
#if defined(__ANDROID__)
static AndroidApi* api = nullptr;
#endif

struct SystemApi::Data {
  std::vector<WindowsApi::TranslateKeyPair> keys;
  std::vector<WindowsApi::TranslateKeyPair> a, k0, f1;
  uint16_t                                  fkeysCount=1;
  };
SystemApi::Data SystemApi::m;

SystemApi::SystemApi() = default;

void SystemApi::setupKeyTranslate(const TranslateKeyPair k[], uint16_t funcCount ) {
  m.keys.clear();
  m.a. clear();
  m.k0.clear();
  m.f1.clear();
  m.fkeysCount = funcCount;

  for( size_t i=0; k[i].result!=Event::K_NoKey; ++i ){
    if( k[i].result==Event::K_A )
      m.a.push_back(k[i]); else
    if( k[i].result==Event::K_0 )
      m.k0.push_back(k[i]); else
    if( k[i].result==Event::K_F1 )
      m.f1.push_back(k[i]); else
      m.keys.push_back( k[i] );
    }

  m.keys.shrink_to_fit();
  m.a. shrink_to_fit();
  m.k0.shrink_to_fit();
  m.f1.shrink_to_fit();
  }

void SystemApi::addOverlay(UiOverlay* ui) {
  dispatcher.addOverlay(ui);
  }

void SystemApi::takeOverlay(UiOverlay* ui) {
  dispatcher.takeOverlay(ui);
  }

void SystemApi::setKeyRepeatDelay(uint64_t delay) {
  dispatcher.setKeyRepeatDelay(delay);
  }

uint64_t SystemApi::getKeyRepeatDelay() {
  return dispatcher.getKeyRepeatDelay();
  }

void SystemApi::setKeyFirstRepeatDelay(uint64_t delay) {
  dispatcher.setKeyFirstRepeatDelay(delay);
  }

uint64_t SystemApi::getKeyFirstRepeatDelay() {
  return dispatcher.getKeyFirstRepeatDelay();
  }

void SystemApi::setTickCount(uint64_t t) {
  dispatcher.setTickCount(t);
  }

uint64_t SystemApi::getTickCount() {
  return dispatcher.getTickCount();
  }

uint16_t SystemApi::translateKey(uint64_t scancode) {
  for(auto & key:m.keys)
    if( key.src==scancode )
      return key.result;

  for(auto & i:m.k0)
    if( i.src<=scancode &&
                     (int)scancode<=i.src+9 ){
      auto dx = (scancode-i.src);
      return Event::KeyType(i.result + dx);
      }

  uint16_t literalsCount = (Event::K_Z - Event::K_A);
  for(auto & i:m.a)
    if(i.src<=scancode &&
                   scancode<=i.src+literalsCount ){
      auto dx = (scancode-i.src);
      return Event::KeyType(i.result + dx);
      }

  for(size_t i=0; i<m.f1.size(); ++i)
    if(m.f1[i].src<=scancode &&
                    scancode<=m.f1[i].src+m.fkeysCount ){
      auto dx = (scancode-m.f1[i].src);
      return Event::KeyType(m.f1[i].result+dx);
      }

  return Event::K_NoKey;
  }

SystemApi& SystemApi::inst() {
#if defined(ANDROID)
  if (api) return *api;
  else {
    api = new AndroidApi;
    return *api;
  }
#else // Desktop
#ifdef __WINDOWS__
  static WindowsApi api;
#elif defined(__LINUX__)
  static X11Api api;
#elif defined(__OSX__)
  static MacOSApi api;
#endif
  return api;
#endif // ANDROID
  }

void SystemApi::dispatchOverlayRender(Tempest::Window &w, PaintEvent& e) {
  dispatcher.dispatchOverlayRender(w,e);
  }

void SystemApi::dispatchRender(Tempest::Window &w) {
  dispatcher.dispatchRender(w);
  }

void SystemApi::dispatchMouseDown(Tempest::Window &cb, MouseEvent &e) {
  dispatcher.dispatchMouseDown(cb,e);
  }

void SystemApi::dispatchMouseUp(Tempest::Window &cb, MouseEvent &e) {
  dispatcher.dispatchMouseUp(cb,e);
  }

void SystemApi::dispatchMouseMove(Tempest::Window &cb, MouseEvent &e) {
  dispatcher.dispatchMouseMove(cb,e);
  }

void SystemApi::dispatchMouseWheel(Tempest::Window &cb, MouseEvent &e) {
  dispatcher.dispatchMouseWheel(cb,e);
  }

void SystemApi::dispatchKeyDown(Tempest::Window &cb, KeyEvent &e, uint32_t scancode) {
  dispatcher.dispatchKeyDown(cb,e,scancode);
  }

void SystemApi::dispatchKeyUp(Tempest::Window &cb, KeyEvent &e, uint32_t scancode) {
  dispatcher.dispatchKeyUp(cb,e,scancode);
  }

void SystemApi::dispatchKeyDown(Tempest::Window &cb, GamepadKeyEvent &e, uint32_t scancode) {
  dispatcher.dispatchKeyDown(cb,e,scancode);
}

void SystemApi::dispatchKeyUp(Tempest::Window &cb, GamepadKeyEvent &e, uint32_t scancode) {
  dispatcher.dispatchKeyUp(cb,e,scancode);
}

void SystemApi::dispatchGamepadMove(Tempest::Window &cb, AnalogEvent &e) {
dispatcher.dispatchGamepadMove(cb,e);
}

void SystemApi::dispatchPointerDown(Tempest::Window &cb, PointerEvent &e) {
dispatcher.dispatchPointerDown(cb,e);
}

void SystemApi::dispatchPointerUp(Tempest::Window &cb, PointerEvent &e) {
dispatcher.dispatchPointerUp(cb,e);
}

void SystemApi::dispatchPointerMove(Tempest::Window &cb, PointerEvent &e) {
dispatcher.dispatchPointerMove(cb,e);
}

void SystemApi::dispatchResize(Tempest::Window& cb, SizeEvent& e) {
  dispatcher.dispatchResize(cb,e);
  }

void SystemApi::dispatchClose(Tempest::Window& cb, CloseEvent& e) {
  dispatcher.dispatchClose(cb,e);
  }

void SystemApi::dispatchAppState(Tempest::Window& cb, AppStateEvent& e) {
  dispatcher.dispatchAppState(cb,e);
  }

void SystemApi::dispatchFocus(Tempest::Window& cb, FocusEvent& e) {
  dispatcher.dispatchFocus(cb,e);
  }

bool SystemApi::isRunning() {
  return inst().implIsRunning();
  }

bool SystemApi::isPaused() {
  return inst().implIsPaused();
  }

bool SystemApi::isResumeRequested() {
  return inst().implIsResumeRequested();
  }

SystemApi::Window *SystemApi::createWindow(Tempest::Window *owner, uint32_t width, uint32_t height, const char* title) {
  return inst().implCreateWindow(owner,width,height,title);
  }

SystemApi::Window *SystemApi::createWindow(Tempest::Window *owner, ShowMode sm, const char* title) {
  return inst().implCreateWindow(owner,sm,title);
  }

void SystemApi::destroyWindow(SystemApi::Window *w) {
  dispatcher.dispatchDestroyWindow(w);
  return inst().implDestroyWindow(w);
  }

void SystemApi::exit() {
  return inst().implExit();
  }

int SystemApi::exec(AppCallBack& cb) {
  return inst().implExec(cb);
  }

void SystemApi::processEvent(AppCallBack& cb) {
  return inst().implProcessEvents(cb);
  }

Rect SystemApi::windowClientRect(SystemApi::Window* w) {
  return inst().implWindowClientRect(w);
  }

bool SystemApi::setAsFullscreen(SystemApi::Window *wx,bool fullScreen) {
  return inst().implSetAsFullscreen(wx,fullScreen);
  }

bool SystemApi::isFullscreen(SystemApi::Window *w) {
  return inst().implIsFullscreen(w);
  }

void SystemApi::setCursorPosition(SystemApi::Window *w, int x, int y) {
  return inst().implSetCursorPosition(w,x,y);
  }

void SystemApi::showCursor(SystemApi::Window *w, CursorShape show) {
  return inst().implShowCursor(w,show);
  }

void SystemApi::clearInput() {
  return dispatcher.clearModKeys();
  }

void SystemApi::shutdown() {
#if defined(ANDROID)
  inst().implShutdown();
  // if (api)
  //   delete api; api = nullptr;
#endif
}

void SystemApi::registerApplication(Application *p) {
#if defined(ANDROID)
  inst().implRegisterApplication(p);
#else
  (void)p;
#endif
}

void SystemApi::unregisterApplication(Application *p) {
#if defined(ANDROID)
  inst().implUnregisterApplication(p);
#else
  (void)p;
#endif
}
