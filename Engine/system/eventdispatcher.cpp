#include "eventdispatcher.h"

#include <Tempest/Application>
#include <Tempest/Shortcut>
#include <Tempest/Platform>
#include <Tempest/UiOverlay>
#include <Tempest/Log>

using namespace Tempest;

EventDispatcher::EventDispatcher() = default;

EventDispatcher::EventDispatcher(Widget &root)
  :customRoot(&root){
  }

void EventDispatcher::setTickCount(uint64_t t) {
  tickCount=t;
  }

uint64_t EventDispatcher::getTickCount() const {
  return tickCount;
  }

void EventDispatcher::setKeyRepeatDelay(uint64_t delay) {
  repeatDelay=delay;
  }

uint64_t EventDispatcher::getKeyRepeatDelay() const {
  return repeatDelay;
  }

void EventDispatcher::setKeyFirstRepeatDelay(uint64_t delay) {
  keyDelayFirst=delay;
  }

uint64_t EventDispatcher::getKeyFirstRepeatDelay() const {
  return keyDelayFirst;
  }

void EventDispatcher::clearModKeys() {
  keyMod={};
  }

void EventDispatcher::dispatchMouseDown(Widget &wnd, MouseEvent &e) {
  MouseEvent e1( e.x,
                 e.y,
                 e.button,
                 mkModifier(),
                 e.delta,
                 e.mouseID,
                 Event::MouseDown );

  for(auto i:overlays) {
    if(!i->bind(wnd))
      continue;
    mouseUp = implDispatch(*i,e1);
    if(!mouseUp.expired())
      break;
    if(e1.type()==MouseEvent::MouseDown)
      mouseLast = mouseUp;
    }


  if(mouseUp.expired())
    mouseUp = implDispatch(wnd,e1);
  if(e1.type()==MouseEvent::MouseDown)
    mouseLast = mouseUp;

  if(auto w = mouseUp.lock()) {
    if(w->widget->focusPolicy() & ClickFocus) {
      w->widget->implSetFocus(true,Event::FocusReason::ClickReason);
      }
    }
  }

void EventDispatcher::dispatchMouseUp(Widget& /*wnd*/, MouseEvent &e) {
  auto ptr = mouseUp;
  mouseUp.reset();

  if(auto w = ptr.lock()) {
    auto p = e.pos() - w->widget->mapToRoot(Point());
    MouseEvent e1( p.x,
                   p.y,
                   e.button,
                   mkModifier(),
                   e.delta,
                   e.mouseID,
                   Event::MouseUp );
    w->widget->mouseUpEvent(e1);
    if(!e1.isAccepted())
      return;
    }
  }

void EventDispatcher::dispatchMouseMove(Widget &wnd, MouseEvent &e) {
  if(auto w = lock(mouseUp)) {
    auto p = e.pos() - w->widget->mapToRoot(Point());
    MouseEvent e0( p.x,
                   p.y,
                   Event::ButtonNone,
                   mkModifier(),
                   e.delta,
                   e.mouseID,
                   Event::MouseDrag  );
    w->widget->mouseDragEvent(e0);
    if(e0.isAccepted())
      return;
    }

  if(auto w = lock(mouseUp)) {
    auto p = e.pos() - w->widget->mapToRoot(Point());
    MouseEvent e1( p.x,
                   p.y,
                   Event::ButtonNone,
                   e.modifier,
                   e.delta,
                   e.mouseID,
                   Event::MouseMove  );
    w->widget->mouseMoveEvent(e1);
    if(e.isAccepted()) {
      implSetMouseOver(mouseUp.lock(),e);
      return;
      }
    }

  MouseEvent e1( e.x,
                 e.y,
                 Event::ButtonNone,
                 mkModifier(),
                 e.delta,
                 e.mouseID,
                 Event::MouseMove  );
  for(auto i:overlays) {
    if(!i->bind(wnd))
      continue;
    auto wptr = implDispatch(*i,e1);
    if(wptr!=nullptr) {
      implSetMouseOver(wptr,e1);
      return;
      }
    }
  auto wptr = implDispatch(wnd,e1);
  implSetMouseOver(wptr,e1);
  }

void EventDispatcher::dispatchMouseWheel(Widget &wnd, MouseEvent &e) {
  MouseEvent e1( e.x,
                 e.y,
                 e.button,
                 mkModifier(),
                 e.delta,
                 e.mouseID,
                 Event::MouseWheel );
  for(auto i:overlays) {
    if(!i->bind(wnd))
      continue;
    implMouseWheel(*i,e1);
    if(e.isAccepted())
      return;
    }
  implMouseWheel(wnd,e1);
  }

void EventDispatcher::dispatchKeyDown(Widget &wnd, KeyEvent &e, uint32_t scancode) {
  auto& k = keyUp[scancode];
  if(auto w = lock(k)) {
    if(lastEmit[scancode]<=tickCount+repeatDelay) return;
    KeyEvent e1(e.key,e.code,mkModifier(),Event::KeyRepeat);
    w->widget->keyRepeatEvent(e1);
    return;
    }

  handleModKey(e);
  KeyEvent e1(e.key,e.code,mkModifier(),e.type());
  for(auto i:overlays) {
    if(!i->bind(wnd))
      continue;
    if(implShortcut(*i,e1))
      return;
    k = implDispatch(*i,e1);
    if(!k.expired())
      return;
    }

  if(implShortcut(wnd,e1))
    return;
  k = implDispatch(wnd,e1);
  }

void EventDispatcher::dispatchKeyUp(Widget &/*wnd*/, KeyEvent &e, uint32_t scancode) {
  auto it = keyUp.find(scancode);
  if(it==keyUp.end())
    return;
  KeyEvent e1(e.key,e.code,mkModifier(),e.type());
  handleModKey(e);
  if(auto w = lock((*it).second)){
    keyUp.erase(it);
    auto r = lastEmit.find(scancode);
    if(r!=lastEmit.end())
      lastEmit.erase(r);
    w->widget->keyUpEvent(e1);
    }
  }

void EventDispatcher::dispatchKeyDown(Widget &wnd, GamepadKeyEvent &e, uint32_t scancode) {
  auto& k = keyUp[scancode];
  if(auto w = lock(k)) {
    auto& r = lastEmit[scancode];
    if(r+repeatDelay<tickCount) return;
    GamepadKeyEvent e1(e.key,e.code,e.scanCode, Event::GamepadKeyRepeat);
    w->widget->keyRepeatEvent(e1);
    return;
  }

  GamepadKeyEvent e1(e.key,e.code,e.type());
  for(auto i:overlays) {
    if(!i->bind(wnd))
      continue;
    k = implDispatch(*i,e1);
    if(!k.expired())
      return;
  }

  k = implDispatch(wnd,e1);
}

void EventDispatcher::dispatchKeyUp(Widget &/*wnd*/, GamepadKeyEvent &e, uint32_t scancode) {
  auto it = keyUp.find(scancode);
  if(it==keyUp.end())
    return;
  GamepadKeyEvent e1(e.key,e.code,scancode,e.type());
  if(auto w = lock((*it).second)){
    keyUp.erase(it);
    auto r = lastEmit.find(scancode);
    if(r!=lastEmit.end())
      lastEmit.erase(r);
    w->widget->keyUpEvent(e1);
  }
}

void EventDispatcher::dispatchGamepadMove(Widget &wnd, AnalogEvent &e) {
  if(auto w = lock(analogUp)) {
    auto p = e.pos() - w->widget->mapToRoot(Point());
    AnalogEvent e1( e.axis,
                   (float)p.x,
                   (float)p.y,
                   0,
                   Event::Type::GamepadAxis );
    //Tempest::Log::i("Analog move");
    w->widget->analogMoveEvent(e1);
    if(e.isAccepted()) {
      //implSetMouseOver(analogUp.lock(),e);
      return;
    }
  }
/*
  MouseEvent e1( e.x,
                 e.y,
                 Event::ButtonNone,
                 0,
                 0,
                 Event::MouseMove  );
  for(auto i:overlays) {
    if(!i->bind(wnd))
      continue;
    auto wptr = implDispatch(*i,e1);
    if(wptr!=nullptr) {
      implSetMouseOver(wptr,e1);
      return;
    }
  }*/
  auto wptr = implDispatch(wnd,e);
//  implSetMouseOver(wptr,e1);
}

void EventDispatcher::dispatchPointerDown(Widget &wnd, PointerEvent &e) {
  //Tempest::Log::i("Pointer down dispatcher: ", e.pointerId," (", e.x,",",e.y,")");
  //updateTrackingPoints(e);
  PointerEvent e1(e.pointerId,
                  e.x,
                  e.y,
                  Event::Type::PointerDown );
  if(auto w = lock(pointerUp[e.pointerId])) {
    //Tempest::Log::i("Pointer down");
    auto p = e.pos() - w->widget->mapToRoot(Point());
    PointerEvent e2(e.pointerId,
                  (float)p.x,
                  (float)p.y,
                  Event::Type::PointerDown );
    
    w->widget->pointerDownEvent(e2);
    if(e.isAccepted()) {
      return;
    }
  }
  
  for(auto i:overlays) {
    if(!i->bind(wnd))
      continue;
    pointerUp[e.pointerId] = implDispatch(*i,e1);
    if(!pointerUp[e.pointerId].expired())
      break;
    }

  if(pointerUp[e.pointerId].expired())
    pointerUp[e.pointerId] = implDispatch(wnd,e1);

  auto wptr = implDispatch(wnd,e);
}

void EventDispatcher::dispatchPointerUp(Widget &wnd, PointerEvent &e) {
  //Tempest::Log::i("Pointer up dispatcher: ", e.pointerId," (", e.x,",",e.y,")");
  auto ptr = pointerUp[e.pointerId];
  pointerUp[e.pointerId].reset();

  if(auto w = ptr.lock()) {
    auto p = e.pos() - w->widget->mapToRoot(Point());
    PointerEvent e1( e.pointerId,
                     (float)p.x,
                     (float)p.y,
                     Event::PointerUp );
    w->widget->pointerUpEvent(e1);
    //Tempest::Log::i("Pointer up event fired:",e1.isAccepted()? "accepted" : "ignored");
    if(!e1.isAccepted())
      return;
    }
  auto wptr = implDispatch(wnd,e);
}

void EventDispatcher::dispatchPointerMove(Widget &wnd, PointerEvent &e) {
  //Tempest::Log::i("Pointer move dispatcher: ", e.pointerId,"(", e.x,",",e.y,")");
  if(auto w = lock(pointerUp[e.pointerId])) { //mapPointerId(e.pointerId,e.x,e.y) keep track if pointers here in dispatcher or in android api
    auto p = Point((int)e.x, (int)e.y) - w->widget->mapToRoot(Point());
    PointerEvent e1( e.pointerId,
                     (float)p.x,
                     (float)p.y,
                     Event::PointerMove );
    w->widget->pointerMoveEvent(e1);
    //Tempest::Log::i("Pointer move: ", e.x,",", e.y);
    if(e.isAccepted()) {
       //Tempest::Log::i("...accepted, pointer over: ", e.x,",", e.y);
       implSetPointerOver(pointerUp[e.pointerId].lock(),e);
      return;
      }
    }

  PointerEvent e1(e.pointerId,
                  e.x,
                  e.y,
                  Event::PointerMove );
  for(auto i:overlays) {
    if(!i->bind(wnd))
      continue;
    //Tempest::Log::i("Pointer move dispatch overlay: ", e.x,",", e.y);
    auto wptr = implDispatch(*i,e1);
    if(wptr!=nullptr) {
       //Tempest::Log::i("Pointer move set pointer over overlay: ", e.x,",", e.y);
       implSetPointerOver(wptr,e1);
      return;
      }
    }
  //Tempest::Log::i("Pointer move dispatch and pointer over routed entity: ", e.x,",", e.y);
  auto wptr = implDispatch(wnd,e1);
   implSetPointerOver(wptr,e1);
}

void EventDispatcher::dispatchResize(Widget& wnd, SizeEvent& e) {
  wnd.resize(int(e.w),int(e.h));
  }

void EventDispatcher::dispatchClose(Widget& wnd, CloseEvent& e) {
  for(auto i:overlays) {
    if(!i->bind(wnd))
      continue;
    i->closeEvent(e);
    if(e.isAccepted())
      return;
    }
  wnd.closeEvent(e);
  }

void EventDispatcher::dispatchAppState(Widget& wnd, AppStateEvent& e) {
  wnd.appStateEvent(e);
  }

void EventDispatcher::dispatchFocus(Widget& wnd, FocusEvent& e) {
  wnd.focusEvent(e);
  }

void EventDispatcher::dispatchRender(Window& wnd) {
  if(wnd.w()>0 && wnd.h()>0)
    wnd.render();
  }

void EventDispatcher::dispatchOverlayRender(Window& wnd, PaintEvent& e) {
  for(size_t i=overlays.size(); i>0;) {
    --i;
    auto w = overlays[i];
    if(!w->bind(wnd))
      continue;
    w->astate.needToUpdate = false;
    w->dispatchPaintEvent(e);
    }
  }

void EventDispatcher::addOverlay(UiOverlay* ui) {
  overlays.insert(overlays.begin(),ui);
  }

void EventDispatcher::takeOverlay(UiOverlay* ui) {
  for(size_t i=0;i<overlays.size();++i)
    if(overlays[i]==ui) {
      overlays.erase(overlays.begin()+int(i));
      return;
      }
  }

void EventDispatcher::dispatchDestroyWindow(SystemApi::Window* w) {
  for(auto & overlay:overlays)
    overlay->dispatchDestroyWindow(w);
  }

std::shared_ptr<Widget::Ref> EventDispatcher::implDispatch(Widget& w, MouseEvent &event) {
  if(!w.isVisible()) {
    event.ignore();
    return nullptr;
    }
  Point            pos=event.pos();
  Widget::Iterator it(&w);
  it.moveToEnd();

  for(;it.hasPrev();it.prev()) {
    Widget* i=it.get();
    if(i->rect().contains(pos)) {
      MouseEvent ex(event.x - i->x(),
                    event.y - i->y(),
                    event.button,
                    event.modifier,
                    event.delta,
                    event.mouseID,
                    event.type());
      auto ptr = implDispatch(*i,ex);
      if(ex.isAccepted() && it.owner!=nullptr) {
        event.accept();
        return ptr;
        }
      }
    }

  if(it.owner!=nullptr) {
    if(event.type()==Event::MouseDown) {
      auto     last     = mouseLast.lock();
      bool     dblClick = false;
      uint64_t time     = Application::tickCount();
      if(time-mouseLastTime<1000 && last!=nullptr && last->widget==it.owner) {
        dblClick = true;
        }
      mouseLastTime = time;
      if(dblClick)
        it.owner->mouseDoubleClickEvent(event); else
        it.owner->mouseDownEvent(event);
      } else {
      it.owner->mouseMoveEvent(event);
      }

    if(event.isAccepted() && it.owner)
      return it.owner->selfReference();
    }
  return nullptr;
  }

std::shared_ptr<Widget::Ref> EventDispatcher::implDispatch(Widget& w,AnalogEvent &event) {
  if(!w.isVisible()) {
    event.ignore();
    return nullptr;
  }
  Point            pos=event.pos();
  Widget::Iterator it(&w);
  it.moveToEnd();

  for(;it.hasPrev();it.prev()) {
    Widget* i=it.get();
    if(i->rect().contains(pos)) {
      AnalogEvent ex(
                    event.axis,
                    event.x,
                    event.y,
                    event.controllerID,
                    event.type());
      auto ptr = implDispatch(*i,ex);
      if(ex.isAccepted() && it.owner!=nullptr) {
        event.accept();
        return ptr;
      }
  }
  }

  if(it.owner!=nullptr) {
//    if(event.type()==Event::MouseDown)
//      it.owner->mouseDownEvent(event); else
      it.owner->analogMoveEvent(event);

    if(event.isAccepted() && it.owner)
      return it.owner->selfReference();
  }
  return nullptr;
}

std::shared_ptr<Widget::Ref> EventDispatcher::implDispatch(Widget& w,PointerEvent &event) {
  if(!w.isVisible()) {
    //Tempest::Log::i("Dispatch to invisible - ignore. Came here with:",event.isAccepted()? "accepted" : "ignored");
    event.ignore();
    return nullptr;
  }
  Point            pos=event.pos();
  Widget::Iterator it(&w);
  it.moveToEnd();

  for(;it.hasPrev();it.prev()) {
    Widget* i=it.get();
    if(i->rect().contains(pos)) {
      PointerEvent ex(
                    event.pointerId,
                    event.x,
                    event.y,
                    event.type);
      auto ptr = implDispatch(*i,ex);
      //Tempest::Log::i("Dispatched to chained widget:",ex.isAccepted()? "accepted" : "ignored");
      if(ex.isAccepted() && it.owner!=nullptr) {
        //Tempest::Log::i("and no owner. Done");
        event.accept();
        return ptr;
      }
  }
  }

  if(it.owner!=nullptr) {
   if(event.type==Event::PointerDown)
     it.owner->pointerDownEvent(event); else
      it.owner->pointerMoveEvent(event);
    //Tempest::Log::i("Dispatch to owner:",event.isAccepted()? "accepted" : "ignored");
    if(event.isAccepted() && it.owner)
      return it.owner->selfReference();
  }
  return nullptr;
}

void EventDispatcher::implMouseWheel(Widget& w,MouseEvent &event) {
  if(!w.isVisible()) {
    event.ignore();
    return;
    }
  Point            pos=event.pos();
  Widget::Iterator it(&w);
  it.moveToEnd();
  for(;it.hasPrev();it.prev()) {
    Widget* i=it.get();
    if(i->rect().contains(pos)){
      MouseEvent ex(event.x - i->x(),
                    event.y - i->y(),
                    event.button,
                    event.modifier,
                    event.delta,
                    event.mouseID,
                    event.type());
      if(it.owner!=nullptr) {
        implMouseWheel(*i,ex);
        if(ex.isAccepted()) {
          event.accept();
          return;
          }
        }
      }
    }

  if(it.owner!=nullptr)
    it.owner->mouseWheelEvent(event);
  }

bool EventDispatcher::implShortcut(Widget& w, KeyEvent& event) {
  if(!w.isVisible())
    return false;

  Widget::Iterator it(&w);
  for(;it.hasNext();it.next()) {
    Widget* i=it.get();
    if(implShortcut(*i,event))
      return true;
    }

  if(!w.astate.focus && !w.wstate.focus)
    return false;

  std::lock_guard<std::recursive_mutex> guard(Widget::syncSCuts);
  for(auto& sc:w.sCuts) {
    if(!sc->isEnable())
      continue;
    if(sc->key() !=event.key  && sc->key() !=KeyEvent::K_NoKey)
      continue;
    if(sc->lkey()!=event.code && sc->lkey()!=0)
      continue;

    if((sc->modifier()&event.modifier)!=sc->modifier())
      continue;

    sc->onActivated();
    return true;
    }

  return false;
  }

std::shared_ptr<Widget::Ref> EventDispatcher::implDispatch(Widget &root, KeyEvent &event) {
  Widget* w = &root;
  while(w->astate.focus!=nullptr) {
    w = w->astate.focus;
    }
  if(w->wstate.focus || &root==w) {
    auto ptr = w->selfReference();
    w->keyDownEvent(event);
    if(event.isAccepted())
      return ptr;
    }
  return nullptr;
  }

std::shared_ptr<Widget::Ref> EventDispatcher::implDispatch(Widget &root, GamepadKeyEvent &event) {
  Widget* w = &root;
  while(w->astate.focus!=nullptr) {
    w = w->astate.focus;
  }
  if(w->wstate.focus || &root==w) {
    auto ptr = w->selfReference();
    w->keyDownEvent(event);
    if(event.isAccepted())
      return ptr;
  }
  return nullptr;
}

void EventDispatcher::implSetMouseOver(const std::shared_ptr<Widget::Ref> &wptr,MouseEvent& orig) {
  auto    widget = wptr==nullptr ? nullptr : wptr->widget;
  Widget* oldW   = nullptr;
  if(auto old = mouseOver.lock())
    oldW = old->widget;

  if(widget==oldW)
    return;

  implExcMouseOver(widget,oldW);

  if(oldW!=nullptr) {
    auto p = orig.pos() - oldW->mapToRoot(Point());
    MouseEvent e( p.x,
                  p.y,
                  Event::ButtonNone,
                  orig.modifier,
                  0,
                  0,
                  Event::MouseLeave );
    oldW->mouseLeaveEvent(e);
    }

  mouseOver = wptr;
  if(widget!=nullptr) {
    auto p = orig.pos() - widget->mapToRoot(Point());
    MouseEvent e( p.x,
                  p.y,
                  Event::ButtonNone,
                  orig.modifier,
                  0,
                  0,
                  Event::MouseLeave );
    widget->mouseEnterEvent(e);
    }
  }

void EventDispatcher::implSetPointerOver(const std::shared_ptr<Widget::Ref> &wptr,PointerEvent& orig) {
  auto    widget = wptr==nullptr ? nullptr : wptr->widget;
  Widget* oldW   = nullptr;
  if(auto old = pointerOver[orig.pointerId].lock())
    oldW = old->widget;

  if(widget==oldW)
    return;

  // for(size_t i=0;i<5;i++) {
  //   Widget* oldW2   = pointerOver[i].lock()->widget;
  //   implExcMouseOver(widget,oldW2); // widgets have a field moveOver that we use here .. lets see if reducing to one variable is okay or not
  // }
  implExcMouseOver(widget,oldW); // widgets have a field moveOver that we use here .. lets see if reducing to one variable is okay or not

  if(oldW!=nullptr) {
    auto p = orig.pos() - oldW->mapToRoot(Point());
    PointerEvent e( orig.pointerId,
                  (float)p.x,
                  (float)p.y,
                  Event::PointerLeave );
    oldW->pointerLeaveEvent(e);
  }

  pointerOver[orig.pointerId] = wptr;
  if(widget!=nullptr) {
    auto p = orig.pos() - widget->mapToRoot(Point());
    PointerEvent e( orig.pointerId,
                    (float)p.x,
                    (float)p.y,
                    Event::PointerLeave );
    widget->pointerEnterEvent(e);
  }
}

void EventDispatcher::implExcMouseOver(Widget* w, Widget* old) {
  auto* wx = old;
  while(wx!=nullptr) {
    wx->wstate.moveOver = false;
    wx = wx->owner();
    }

  wx = w;
  if(w!=nullptr) {
    auto root = w;
    while(root->owner()!=nullptr)
      root = root->owner();
    if(auto r = dynamic_cast<Window*>(root))
      r->implShowCursor(w->wstate.cursor);
    if(auto r = dynamic_cast<UiOverlay*>(root))
      r->implShowCursor(w->wstate.cursor);
    }
  while(wx!=nullptr) {
    wx->wstate.moveOver = true;
    wx = wx->owner();
    }
  }

void EventDispatcher::updateTrackingPoints(PointerEvent &event) {
  auto closest=size_t(-1);
  Point closestP(-1000,-1000);
  Point pointE((int)event.x, (int)event.y);
  for(size_t i=0;i<trackingPoints; i++) {
    if (pointers[i].x >= 0 && pointers[i].y >= 0) {
      Point delta = pointers[i] - pointE;
      if (delta.quadLength() < closestP.quadLength()) {
        closestP = pointers[i];
        closest = i;
      }
    }
  }
  size_t sel=closest!=size_t(-1) ? closest : 0;
  pointers[sel]=pointE;
  for(size_t i=trackingPoints-event.totalPointers;i<trackingPoints; i++) {
    if(pointers[i].x>=0 && pointers[i].y>=0) {
      Point delta=pointers[i]-Point((int)event.x,(int)event.y);
      (void)delta;
    }
  }
}

void EventDispatcher::handleModKey(const KeyEvent& e) {
  const bool v = e.type()==Event::KeyDown ? true : false;
  switch(e.key) {
    case Event::K_LControl:
      keyMod.ctrlL = v;
      break;
    case Event::K_RControl:
      keyMod.ctrlR = v;
      break;
#ifdef __OSX__
    case Event::K_LCommand:
      keyMod.cmdL = v;
      break;
    case Event::K_RCommand:
      keyMod.cmdR = v;
      break;
#endif
    case Event::K_LShift:
      keyMod.shiftL = v;
      break;
    case Event::K_RShift:
      keyMod.shiftR = v;
      break;
    case Event::K_LAlt:
      keyMod.altL = v;
      break;
    case Event::K_RAlt:
      keyMod.altR = v;
      break;
    default:
      break;
    }
  }

std::shared_ptr<Widget::Ref> EventDispatcher::lock(std::weak_ptr<Widget::Ref>& w) {
  auto ptr = w.lock();
  if(ptr==nullptr)
    return nullptr;
  Widget* wx = ptr.get()->widget;
  while(wx->owner()!=nullptr) {
    wx = wx->owner();
    }
  if(dynamic_cast<Window*>(wx) || dynamic_cast<UiOverlay*>(wx) || wx==customRoot)
    return ptr;
  return nullptr;
  }

Event::Modifier EventDispatcher::mkModifier() const {
  uint8_t ret = 0;
  if(keyMod.ctrlL || keyMod.ctrlR)
    ret |= Event::Modifier::M_Ctrl;
  if(keyMod.altL || keyMod.altR)
    ret |= Event::Modifier::M_Alt;
  if(keyMod.shiftL || keyMod.shiftR)
    ret |= Event::Modifier::M_Shift;
#ifdef __OSX__
  if(keyMod.cmdL || keyMod.cmdR)
    ret |= Event::Modifier::M_Command;
#endif
  return Event::Modifier(ret);
  }
