#pragma once

#include "ui/event.h"
#include <Tempest/Window>
#include <Tempest/Event>

#include <unordered_map>

namespace Tempest {

const size_t trackingPoints=5;
class EventDispatcher final {
  public:
    EventDispatcher();
    EventDispatcher(Widget& root);

    void dispatchMouseDown (Widget& wnd, Tempest::MouseEvent& event);
    void dispatchMouseUp   (Widget& wnd, Tempest::MouseEvent& event);
    void dispatchMouseMove (Widget& wnd, Tempest::MouseEvent& event);
    void dispatchMouseWheel(Widget& wnd, Tempest::MouseEvent& event);

    void dispatchKeyDown   (Widget& wnd, Tempest::KeyEvent&   event, uint32_t scancode);
    void dispatchKeyUp     (Widget& wnd, Tempest::KeyEvent&   event, uint32_t scancode);

    void dispatchKeyDown   (Widget &wnd, GamepadKeyEvent &e, uint32_t scancode);
    void dispatchKeyUp     (Widget &wnd, GamepadKeyEvent &e, uint32_t scancode);
    void dispatchGamepadMove(Widget &wnd, AnalogEvent &e);

    void dispatchPointerDown(Widget &wnd, PointerEvent &e);
    void dispatchPointerUp  (Widget &wnd, PointerEvent &e);
    void dispatchPointerMove(Widget &wnd, PointerEvent &e);

    void dispatchResize    (Widget& wnd, Tempest::SizeEvent&  event);
    void dispatchClose     (Widget& wnd, Tempest::CloseEvent& event);
    void dispatchAppState  (Widget& wnd, Tempest::AppStateEvent& event);
    void dispatchFocus     (Widget& wnd, Tempest::FocusEvent& event);

    void dispatchRender    (Window& wnd);
    void dispatchOverlayRender(Window& wnd,Tempest::PaintEvent& e);
    void addOverlay        (UiOverlay* ui);
    void takeOverlay       (UiOverlay* ui);

    void setKeyRepeatDelay         (uint64_t delay);
    void setKeyFirstRepeatDelay    (uint64_t delay);
    void setTickCount              (uint64_t tickCount);
    uint64_t getTickCount          () const;
    uint64_t getKeyRepeatDelay     () const;
    uint64_t getKeyFirstRepeatDelay() const;

    void clearModKeys();

    void dispatchDestroyWindow(SystemApi::Window* w);

  private:
    std::shared_ptr<Widget::Ref> implDispatch(Tempest::Widget &w, Tempest::MouseEvent& event);
    std::shared_ptr<Widget::Ref> implDispatch(Tempest::Widget &w, Tempest::AnalogEvent& event);
    std::shared_ptr<Widget::Ref> implDispatch(Tempest::Widget &w, Tempest::PointerEvent& event);
    void                         implMouseWheel(Widget &w, MouseEvent &event);

    bool                         implShortcut(Tempest::Widget &w, Tempest::KeyEvent& event);
    std::shared_ptr<Widget::Ref> implDispatch(Tempest::Widget &w, Tempest::KeyEvent& event);
    std::shared_ptr<Widget::Ref> implDispatch(Tempest::Widget &w, Tempest::GamepadKeyEvent& event);
    void                         implSetMouseOver(const std::shared_ptr<Widget::Ref>& s, MouseEvent& orig);
    void                         implSetPointerOver(const std::shared_ptr<Widget::Ref>& s, PointerEvent& orig);
    void                         implExcMouseOver(Widget *w, Widget *old);
    void                         handleModKey(const KeyEvent& e);
    size_t                       mapPointerId(int pId, int x, int y);
    std::shared_ptr<Widget::Ref> lock(std::weak_ptr<Widget::Ref>& w);

    Widget*                      customRoot=nullptr;
    std::weak_ptr<Widget::Ref>   mouseUp;
    std::weak_ptr<Widget::Ref>   mouseLast;
    std::weak_ptr<Widget::Ref>   mouseOver;
    std::weak_ptr<Widget::Ref>   pointerOver[trackingPoints];
    std::weak_ptr<Widget::Ref>   pointerUp[trackingPoints];
    std::weak_ptr<Widget::Ref>   analogUp;

    std::vector<UiOverlay*>      overlays;
    uint64_t                     mouseLastTime = 0;

    uint64_t                     tickCount=0;
    uint64_t                     repeatDelay=150;
    uint64_t                     keyDelayFirst=0;

    Point pointers[trackingPoints]={ Point(-1,-1) };


    struct Modify final {
      bool ctrlL   = false;
      bool ctrlR   = false;

#ifdef __OSX__
      bool cmdL    = false;
      bool cmdR    = false;
#endif

      bool altL    = false;
      bool altR    = false;

      bool shiftL  = false;
      bool shiftR  = false;
      };
    Modify keyMod;
    KeyEvent::Modifier mkModifier() const;

    std::unordered_map<uint32_t,std::weak_ptr<Widget::Ref>> keyUp;
    std::unordered_map<uint32_t,uint64_t>                   lastEmit;

    void updateTrackingPoints(PointerEvent &event);
};

}

