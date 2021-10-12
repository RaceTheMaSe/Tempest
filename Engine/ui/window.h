#pragma once

#include <Tempest/SystemApi>
#include <Tempest/Widget>

namespace Tempest {

class VectorImage;
class TextureAtlas;

class Window : public Widget {
  public:
    enum ShowMode : uint8_t {
      Minimized,
      Normal,
      Maximized,
      FullScreen
      };

    Window(const char* title);
    Window(ShowMode sm , const char* title);
    ~Window() override;

    void updateHandle(SystemApi::Window* handle) { id = handle; }

  protected:
    virtual void render();
    using        Widget::dispatchPaintEvent;
    void         dispatchPaintEvent(VectorImage &e,TextureAtlas &ta);
    void         closeEvent       (Tempest::CloseEvent& event) override;

    SystemApi::Window* hwnd() const { return id; }

    void         setCursorPosition(int x, int y);
    void         setCursorPosition(const Point& p);

    void         implShowCursor(CursorShape s);

  private:
    SystemApi::Window* id=nullptr;

  friend class Widget;
  friend class UiOverlay;
  friend class EventDispatcher;
  };

}
