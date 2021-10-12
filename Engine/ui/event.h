#pragma once

#include <Tempest/Size>
#include <cstdint>

namespace Tempest {

class PaintDevice;
class TextureAtlas;
class Painter;

class Event {
  public:
    Event()=default;
    virtual ~Event()= default;

    void accept(){ accepted = true;  }
    void ignore(){ accepted = false; }

    bool isAccepted() const {
      return accepted;
      }

    enum Type : uint16_t {
      NoEvent = 0,
      MouseDown,
      MouseUp,
      MouseMove,
      MouseDrag,
      MouseWheel,
      MouseEnter,
      MouseLeave,
      PointerDown,
      PointerUp,
      PointerMove,
      PointerEnter,
      PointerLeave,
      KeyDown,
      KeyRepeat,
      KeyUp,
      GamepadKeyDown,
      GamepadKeyRepeat,
      GamepadKeyUp,
      GamepadAxis,
      Focus,
      Resize,
      Shortcut,
      Paint,
      Close,
      Polish,
      Gesture,
      AppStateChange,

      Custom = 512
      };

    enum MouseButton : uint8_t {
      ButtonNone = 0,
      ButtonLeft,
      ButtonRight,
      ButtonMid,
      ButtonBack,
      ButtonForward
      };

    enum KeyType : uint8_t {
      K_NoKey = 0,
      K_ESCAPE,

      K_RControl,
      K_LControl,

      K_RCommand, // APPLE command key
      K_LCommand,

      K_RShift,
      K_LShift,

      K_LAlt,
      K_RAlt,

      K_Left,
      K_Up,
      K_Right,
      K_Down,

      K_Back,
      K_Tab,
      K_Delete,
      K_Insert,
      K_Return,
      K_PageUp,
      K_PageDown,
      K_Home,
      K_End,
      K_Pause,
      K_Space,
      K_CapsLock,
      K_NumLock,

      K_Multiply,
      K_Add,
      K_Separator,
      K_Subtract,
      K_Decimal,
      K_Divide,
      K_Equal,
      K_KP_Return,
      K_OpenQuote, // left to numkey row

      K_F1,
      K_F2,
      K_F3,
      K_F4,
      K_F5,
      K_F6,
      K_F7,
      K_F8,
      K_F9,
      K_F10,
      K_F11,
      K_F12,
      K_F13,
      K_F14,
      K_F15,
      K_F16,
      K_F17,
      K_F18,
      K_F19,
      K_F20,
      K_F21,
      K_F22,
      K_F23,
      K_F24,

      K_A,
      K_B,
      K_C,
      K_D,
      K_E,
      K_F,
      K_G,
      K_H,
      K_I,
      K_J,
      K_K,
      K_L,
      K_M,
      K_N,
      K_O,
      K_P,
      K_Q,
      K_R,
      K_S,
      K_T,
      K_U,
      K_V,
      K_W,
      K_X,
      K_Y,
      K_Z,

      K_0,
      K_1,
      K_2,
      K_3,
      K_4,
      K_5,
      K_6,
      K_7,
      K_8,
      K_9,

      K_KP_0,
      K_KP_1,
      K_KP_2,
      K_KP_3,
      K_KP_4,
      K_KP_5,
      K_KP_6,
      K_KP_7,
      K_KP_8,
      K_KP_9,

      K_Last
      };

    enum GamepadKeyType : uint8_t {
        // based on Android input.h
        G_NoKey = 0,
        G_Left,
        G_Right,
        G_Up,
        G_Down,

        G_A,
        G_B,
        G_C,
        G_L1,
        G_L2,
        G_R1,
        G_R2,
        G_X,
        G_Y,
        G_Z,
        G_ThumbStickLeft,
        G_ThumbStickRight,
        G_Start,
        G_Select,
        G_Mode,

        G_Last
    };

    enum GamepadAxisType : uint8_t {
        AxisNone = 0,
        AxisLeftStickX,
        AxisLeftStickY,
        AxisRightStickX,
        AxisRightStickY,
        AxisLeftTrigger,
        AxisRightTrigger,

        Axis2D,
        AxisLast
    };

    enum Modifier : uint8_t {
      M_NoModifier = 0,
      M_Shift      = 1<<0,
      M_Alt        = 1<<1,
      M_Ctrl       = 1<<2,
      M_Command    = 1<<3 // APPLE command key
      };

    enum FocusReason : uint8_t {
      TabReason,
      ClickReason,
      WheelReason,
      HoverReason,
      WindowManager,
      UnknownReason
      };

    enum AppState : uint8_t {
      Running,
      Paused,
      Stopped,
      Resumed,
      SaveState,
      LoadState,
      UnknownState
      };

    Type type () const{ return etype; }

  protected:
    void setType( Type t ){ etype = t; }

  private:
    bool accepted=true;
    Type etype   =NoEvent;
  };

class PaintEvent: public Event {
  public:
    PaintEvent(PaintDevice & p,TextureAtlas& taIn,uint32_t w,uint32_t h)
      : PaintEvent(p,taIn,int(w),int(h)) {
      }
    PaintEvent(PaintDevice & p,TextureAtlas& taIn,int32_t wIn,int32_t hIn)
      : dev(p),ta(taIn),outW(uint32_t(wIn)),outH(uint32_t(hIn)),
        vp(0,0,wIn,hIn) {
      setType( Paint );
      }

    PaintEvent(PaintEvent& parent,int32_t dx,int32_t dy,int32_t x,int32_t y,int32_t w,int32_t h)
      : dev(parent.dev),ta(parent.ta),outW(parent.outW),outH(parent.outH),
        dp(parent.dp.x+dx,parent.dp.y+dy),vp(x,y,w,h){
      setType( Paint );
      }

    PaintDevice& device()  { return dev;  }
    uint32_t     w() const { return outW; }
    uint32_t     h() const { return outH; }

    const Point& orign()    const { return dp; }
    const Rect&  viewPort() const { return vp; }

  private:
    PaintDevice&  dev;
    TextureAtlas& ta;
    uint32_t      outW=0;
    uint32_t      outH=0;

    Point         dp;
    Rect          vp;

    using Event::accept;

  friend class Painter;
  };

/*!
 * \brief The SizeEvent class contains event parameters for resize events.
 */
class SizeEvent : public Event {
  public:
    SizeEvent(int wIn,int hIn): w(uint32_t(std::max(wIn,0))), h(uint32_t(std::max(hIn,0))) {
      setType( Resize );
      }
    SizeEvent(uint32_t wIn,uint32_t hIn): w(wIn), h(hIn) {
      setType( Resize );
      }

    const uint32_t w, h;

    Tempest::Size size() const { return {int(w),int(h)}; }
  };

/*!
 * \brief The MouseEvent class contains event parameters for mouse and touch events.
 */
class MouseEvent : public Event {
  public:
    MouseEvent( int mx = -1, int my = -1,
                MouseButton b = ButtonNone,
                Modifier m = M_NoModifier,
                int mdelta = 0,
                int mouseIDIn = 0,
                Type t = MouseMove )
      :x(mx), y(my), delta(mdelta), button(b), modifier(m), mouseID(mouseIDIn){
      setType( t );
      }

    const int x, y, delta;
    const MouseButton button;
    const Modifier    modifier = M_NoModifier;

    const int mouseID;

    Point pos() const { return {x,y}; }
  };

class KeyEvent: public Event {
  public:
    KeyEvent(KeyType  k = K_NoKey, Modifier m = M_NoModifier, Type t = KeyDown):key(k),modifier(m){
      setType( t );
      }
    KeyEvent(uint32_t k, Modifier m = M_NoModifier, Type t = KeyDown ):code(k), modifier(m){
      setType( t );
      }
    KeyEvent(KeyType k, uint32_t k1, Modifier m = M_NoModifier, Type t = KeyDown):key(k), code(k1), modifier(m){
      setType( t );
      }

    const KeyType  key      = K_NoKey;
    const uint32_t code     = 0;
    const Modifier modifier = M_NoModifier;
  };

class GamepadKeyEvent : public Event {
public:
  GamepadKeyEvent(GamepadKeyType  k = G_NoKey, uint32_t /*code*/ = 0, uint32_t sc = 0, Type t = GamepadKeyDown) : key(k), code(k), scanCode(sc) {
    setType(t
    );
  }

  const GamepadKeyType  key = G_NoKey;
  const uint32_t        code = 0;
  const uint32_t        scanCode = 0;
};

/*!
* \brief The PointerEvent class contains event parameters for touch events
*/
class PointerEvent : public Event {
public:
    PointerEvent(size_t pId = 0,
                float xIn = 0.0f,
                float yIn = 0.0f,
                Type t = NoEvent,
                size_t tp = 0 )
            : x(xIn), y(yIn), type(t), pointerId(pId), totalPointers(tp) { }

    const float x;
    const float y;
    const Type type;

    const size_t pointerId;
    const size_t totalPointers;

    Point pos() const { return {(int)x,(int)y}; }
};

/*!
* \brief The AnalogEvent class contains event parameters for any type of events that don't fit other categories
*/
class AnalogEvent : public Event {
public:
    AnalogEvent(GamepadAxisType b = AxisNone,
                float xIn = 0.0f,
                float yIn = 0.0f, // optional - select Axis2D
                int controllerIDIn = 0,
                Type t = GamepadAxis )
            : x(xIn), y(yIn), axis(b), controllerID(controllerIDIn){
      setType( t );
    }

    const float x;
    const float y;
    const GamepadAxisType axis;

    const int controllerID;

    // might only have x value used, check GamepadAxisType
    Point pos() const { return {(int)x,(int)y}; }
};

class FocusEvent: public Event {
  public:
    FocusEvent( bool focusIn, FocusReason reasonIn ): in(focusIn), reason(reasonIn) {
      setType( Focus );
      }

    const bool        in;
    const FocusReason reason;
  };

class AppStateEvent: public Event {
  public:
    AppStateEvent( AppState appStateIn ): appState(appStateIn) {
      setType( AppStateChange );
      }

    const AppState appState;
  };

class CloseEvent: public Event {
  public:
    CloseEvent() { setType(Close); }
  };

class PolishEvent: public Event {
  public:
    PolishEvent() { setType(Polish); }
  };
}
