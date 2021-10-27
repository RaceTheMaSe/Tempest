#pragma once

#include <Tempest/Widget>
#include <Tempest/SystemApi>

namespace Tempest {

class UiOverlay : public Tempest::Widget {
  public:
    UiOverlay();
    ~UiOverlay() override;

    void updateWindow();

  private:
    bool bind(Widget& w);
    bool bind(Window& w);
    void dispatchDestroyWindow(SystemApi::Window* w);
    void implShowCursor(CursorShape s);

    Window* owner=nullptr;

  friend class EventDispatcher;
  };

}

