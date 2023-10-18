#include "uioverlay.h"

#include <Tempest/Window>

using namespace Tempest;

UiOverlay::UiOverlay() = default;

UiOverlay::~UiOverlay() {
  updateWindow();
  SystemApi::takeOverlay(this);
  }

void UiOverlay::updateWindow() {
  if(owner!=nullptr)
    owner->update();
  }

bool UiOverlay::bind(Widget& w) {
  if(auto wnd = dynamic_cast<Window*>(&w))
    return bind(*wnd);
  return false;
  }

bool UiOverlay::bind(Window& w) {
  if(owner==&w) {
    setGeometry(w.rect());
    return true;
    }
  if(owner!=nullptr)
    return false;
  owner = &w;
  owner->update();
  setGeometry(w.rect());
  applyLayout();
  return true;
  }

void UiOverlay::dispatchDestroyWindow(SystemApi::Window* w) {
  if(owner->hwnd()==w)
    owner = nullptr;
  }

void UiOverlay::implShowCursor(CursorShape s) {
  if(owner!=nullptr)
    owner->implShowCursor(s);
  }

