#pragma once

#include <Tempest/Rect>
#include <Tempest/SizePolicy>
#include <Tempest/Event>
#include <Tempest/Style>
#include <Tempest/WidgetState>

#include <vector>
#include <memory>
#include <mutex>

namespace Tempest {

class Layout;
class Shortcut;

enum FocusPolicy : uint8_t {
  NoFocus     = 0,
  TabFocus    = 1,
  ClickFocus  = 2,
  StrongFocus = TabFocus|ClickFocus,
  WheelFocus  = 4|StrongFocus
  };

class Widget {
  public:
    Widget();
    Widget(const Widget&)=delete;
    virtual ~Widget();

    void setLayout(Orientation ori) noexcept;
    void setLayout(Layout* lay);
    const Layout& layout() const { return *lay; }
    void  applyLayout();

    size_t        widgetsCount() const   { return wx.size(); }
    Widget&       widget(size_t i)       { return *wx[i]; }
    const Widget& widget(size_t i) const { return *wx[i]; }
    void          removeAllWidgets();
    template<class T>
    T&            addWidget(T* w);
    template<class T>
    T&            addWidget(T* w,size_t at);
    Widget*       takeWidget(Widget* w);

          Widget* owner()       { return ow; }
    const Widget* owner() const { return ow; }

    Point   mapToRoot  ( const Point & p ) const noexcept;
    Point   mapToGlobal( const Point & p ) const noexcept;

    Point                pos()      const { return {wrect.x,wrect.y};}
    const Tempest::Rect& rect()     const { return wrect; }
    Tempest::Size        size()     const { return {wrect.w,wrect.h}; }
    const Tempest::Size& sizeHint() const { return szHint; }

    void  setPosition(int x,int y);
    void  setPosition(const Point& pos);
    void  setGeometry(const Rect& rect);
    void  setGeometry(int x,int y,int w,int h);
    void  resize(const Size& size);
    void  resize(int w,int h);

    int   x() const { return  wrect.x; }
    int   y() const { return  wrect.y; }
    int   w() const { return  wrect.w; }
    int   h() const { return  wrect.h; }

    void setSizePolicy(SizePolicyType hv);
    void setSizePolicy(SizePolicyType h,SizePolicyType  v);
    void setSizePolicy(const SizePolicy& sp);
    const SizePolicy& sizePolicy() const { return szPolicy; }

    FocusPolicy focusPolicy() const { return fcPolicy; }
    void setFocusPolicy( FocusPolicy f );

    void setMargins(const Margin& m);
    const Margin& margins() const { return marg; }

    const Size& minSize() const { return szPolicy.minSize; }
    const Size& maxSize() const { return szPolicy.maxSize; }

    void setMaximumSize( const Size & s );
    void setMinimumSize( const Size & s );

    void setMaximumSize( int w, int h );
    void setMinimumSize( int w, int h );

    void setSpacing( int s );
    int  spacing() const { return spa; }

    Rect clientRect() const;
    Rect absoluteRect() const;

    void setEnabled(bool e);
    bool isEnabled() const;
    bool isEnabledTo(const Widget* ancestor) const;

    void setVisible(bool v);
    bool isVisible() const;

    void setFocus(bool b);
    bool hasFocus() const { return wstate.focus; }

    void setCursorShape(CursorShape cs);
    auto cursorShape() const -> CursorShape { return wstate.cursor; }

    void update() noexcept;
    bool needToUpdate() const { return astate.needToUpdate; }

    bool isMouseOver()  const { return wstate.moveOver; }

    void               setStyle(const Style* stl);
    const Style&       style() const;

    const WidgetState& state() const { return wstate; }

  protected:
    void setSizeHint(const Tempest::Size& s);
    void setSizeHint(const Tempest::Size& s,const Margin& add);
    void setSizeHint(int w,int h) { return setSizeHint(Size(w,h)); }

    void setWidgetState(const WidgetState& st);

    virtual void paintEvent     (Tempest::PaintEvent&  event);
    virtual void dispatchPaintEvent(Tempest::PaintEvent& event);
    void         paintNested    (Tempest::PaintEvent&  event);
    virtual void resizeEvent    (Tempest::SizeEvent&   event);

    virtual void mouseDownEvent (Tempest::MouseEvent&  event);
    virtual void mouseUpEvent   (Tempest::MouseEvent&  event);
    virtual void mouseMoveEvent (Tempest::MouseEvent&  event);
    virtual void mouseDragEvent (Tempest::MouseEvent&  event);
    virtual void mouseWheelEvent(Tempest::MouseEvent&  event);

    virtual void keyDownEvent   (Tempest::KeyEvent&    event);
    virtual void keyRepeatEvent (Tempest::KeyEvent&    event);
    virtual void keyUpEvent     (Tempest::KeyEvent&    event);

    virtual void keyDownEvent   (Tempest::GamepadKeyEvent&    event);
    virtual void keyRepeatEvent (Tempest::GamepadKeyEvent&    event);
    virtual void keyUpEvent     (Tempest::GamepadKeyEvent&    event);
    virtual void analogMoveEvent(Tempest::AnalogEvent&  event);

    virtual void pointerDownEvent(Tempest::PointerEvent&  event);
    virtual void pointerUpEvent  (Tempest::PointerEvent&  event);
    virtual void pointerMoveEvent(Tempest::PointerEvent&  event);

    virtual void pointerEnterEvent(Tempest::PointerEvent&  event);
    virtual void pointerLeaveEvent(Tempest::PointerEvent&  event);

    virtual void mouseEnterEvent(Tempest::MouseEvent&  event);
    virtual void mouseLeaveEvent(Tempest::MouseEvent&  event);

    virtual void appStateEvent  (Tempest::AppStateEvent&  event);
    virtual void focusEvent     (Tempest::FocusEvent&  event);
    virtual void closeEvent     (Tempest::CloseEvent&  event);
    virtual void polishEvent    (Tempest::PolishEvent& event);

  private:
    struct Iterator final {
      Iterator(Widget* owner);
      ~Iterator();

      void    onDelete();
      void    onDelete(size_t i,Widget* wx);
      bool    hasNext() const;
      bool    hasPrev() const;
      void    next();
      void    prev();
      void    moveToEnd();
      Widget* get();
      Widget* getLast();

      size_t                id=0;
      Widget*               owner=nullptr;
      std::vector<Widget*>* nodes;
      std::vector<Widget*>  deleteLater;
      Widget*               getPtr=nullptr;
      };

    struct Ref {
      Ref(Widget* w):widget(w){}
      ~Ref();
      Widget* widget          = nullptr;
      bool    deleteLaterHint = false;
      };

    struct Additive {
      Widget*  focus        = nullptr;
      uint16_t disable      = 0;
      bool     needToUpdate = false;
      };

    Widget*                 ow=nullptr;
    std::vector<Widget*>    wx;
    Tempest::Rect           wrect;
    Tempest::Size           szHint;
    Tempest::SizePolicy     szPolicy;
    FocusPolicy             fcPolicy=NoFocus;
    Tempest::Margin         marg;
    int                     spa=2;
    Iterator*               iterator=nullptr;

    Additive                astate;
    WidgetState             wstate;

    static std::recursive_mutex syncSCuts;
    std::vector<Shortcut*>  sCuts;

    std::shared_ptr<Ref>    selfRef;

    Layout*                 lay=reinterpret_cast<Layout*>(layBuf);
    char                    layBuf[sizeof(void*)*3]={};

    const Style*            stl = nullptr;

    void                    implRegisterSCut(Shortcut* s);
    void                    implUnregisterSCut(Shortcut* s);

    void                    freeLayout() noexcept;
    void                    implDisableSum(Widget *root,int diff) noexcept;
    Widget&                 implAddWidget(Widget* w,size_t at);
    void                    implSetFocus(bool b,Event::FocusReason reason);
    void                    implSetFocus(Widget* Additive::*add, bool WidgetState::*flag, bool value, const FocusEvent* parent);
    static void             implExcFocus(Widget* prev, Widget* next, const FocusEvent& parent);
    static void             implClearFocus(Widget* w,Widget* Additive::*add, bool WidgetState::*flag);
    static Widget*          implTrieRoot(Widget* w);
    bool                    checkFocus() const { return wstate.focus || astate.focus; }
    void                    implAttachFocus();

    void                    dispatchPolishEvent(PolishEvent& e);

    auto                    selfReference() -> const std::shared_ptr<Ref>&;
    void                    setOwner(Widget* w);
    void                    deleteLater() noexcept;

  friend class EventDispatcher;
  friend class ListDelegate;
  friend class Shortcut;
  friend class Layout;
  friend class Window;
  };

template<class T>
T& Widget::addWidget(T* w){
  implAddWidget(w,widgetsCount());
  return *w;
  }

template<class T>
T& Widget::addWidget(T* w,size_t at){
  implAddWidget(w,at);
  return *w;
  }
}
