#include "androidapi.h"

#if defined(__ANDROID__)
#include <android/native_window.h>
#include <Tempest/Event>
#include <Tempest/TextCodec>
#include <Tempest/Window>
#include <Tempest/Log>

#include <atomic>
#include <stdexcept>
#include <cstring>
#include <thread>
#include <unordered_map>

using namespace Tempest;

namespace Tempest {
  ANativeWindow*   AndroidApi::m_Window;
  std::atomic_bool AndroidApi::m_HasFocus(false);
  std::atomic_bool AndroidApi::isExit{false};
  std::mutex       AndroidApi::m_mutex;
  std::thread      AndroidApi::m_renderThread;
  std::thread      AndroidApi::m_eventThread;
  std::atomic_bool AndroidApi::shouldProcess;
  std::vector<Tempest::Application*> AndroidApi::m_Apps;
  int32_t          AndroidApi::width;
  int32_t          AndroidApi::height;
};

android_app* globalAppPtr = nullptr;
static std::unordered_map<Tempest::Window*, SystemApi::Window*> app_windows;

// Convenience helpers for log messages
static const std::vector<const char*> app_commands = {
        "INPUT_CHANGED",
        "INIT_WINDOW",
        "TERM_WINDOW",
        "WINDOW_RESIZED",
        "WINDOW_REDRAW_NEEDED",
        "CONTENT_RECT_CHANGED",
        "GAINED_FOCUS",
        "LOST_FOCUS",
        "CONFIG_CHANGED",
        "LOW_MEMORY",
        "START",
        "RESUME",
        "SAVE_STATE",
        "PAUSE",
        "STOP",
        "DESTROY",
};

static const Tempest::SystemApi::TranslateKeyPair k[] = {
        { AKEYCODE_CTRL_LEFT,     Event::K_LControl },
        { AKEYCODE_CTRL_RIGHT,    Event::K_RControl },

        { AKEYCODE_SHIFT_LEFT,    Event::K_LShift   },
        { AKEYCODE_SHIFT_RIGHT,   Event::K_RShift   },

        { AKEYCODE_ALT_LEFT,      Event::K_LAlt     },

        { AKEYCODE_DPAD_LEFT,     Event::K_Left     },
        { AKEYCODE_DPAD_RIGHT,    Event::K_Right    },
        { AKEYCODE_DPAD_UP,       Event::K_Up       },
        { AKEYCODE_DPAD_DOWN,     Event::K_Down     },

        { AKEYCODE_ESCAPE,        Event::K_ESCAPE   },
        { AKEYCODE_BACK,          Event::K_Back     },
        { AKEYCODE_TAB,           Event::K_Tab      },
        { AKEYCODE_DEL,           Event::K_Delete   },
        { AKEYCODE_INSERT,        Event::K_Insert   },
        { AKEYCODE_MOVE_HOME,     Event::K_Home     },
        { AKEYCODE_MOVE_END,      Event::K_End      },
        { AKEYCODE_BREAK,         Event::K_Pause    },
        { AKEYCODE_ENTER,         Event::K_Return   },
        { AKEYCODE_SPACE,         Event::K_Space    },
        { AKEYCODE_CAPS_LOCK,     Event::K_CapsLock },

        { AKEYCODE_F1,            Event::K_F1       },
        { AKEYCODE_0,             Event::K_0        },
        { AKEYCODE_A,             Event::K_A        },
        { AKEYCODE_Z,             Event::K_Z        },

        { AKEYCODE_DPAD_UP,       Event::K_Up       },
        { AKEYCODE_DPAD_DOWN,     Event::K_Down     },
        { AKEYCODE_DPAD_LEFT,     Event::K_Left     },
        { AKEYCODE_DPAD_RIGHT,    Event::K_Right    },

        // application specific mapping, should be moved to a better spot
        { AKEYCODE_BUTTON_A,      Event::GamepadKeyType::G_A              },
        { AKEYCODE_BUTTON_B,      Event::GamepadKeyType::G_B              },
        { AKEYCODE_BUTTON_C,      Event::GamepadKeyType::G_C              },
        { AKEYCODE_BUTTON_L1,     Event::GamepadKeyType::G_L1             },
        { AKEYCODE_BUTTON_L2,     Event::GamepadKeyType::G_L2             },
        { AKEYCODE_BUTTON_R1,     Event::GamepadKeyType::G_R1             },
        { AKEYCODE_BUTTON_R2,     Event::GamepadKeyType::G_R2             },
        { AKEYCODE_BUTTON_X,      Event::GamepadKeyType::G_X              },
        { AKEYCODE_BUTTON_Y,      Event::GamepadKeyType::G_Y              },
        { AKEYCODE_BUTTON_Z,      Event::GamepadKeyType::G_Z              },
        { AKEYCODE_BUTTON_THUMBL, Event::GamepadKeyType::G_ThumbStickLeft },
        { AKEYCODE_BUTTON_THUMBR, Event::GamepadKeyType::G_ThumbStickRight},
        { AKEYCODE_BUTTON_START,  Event::GamepadKeyType::G_Start          },
        { AKEYCODE_BUTTON_SELECT, Event::GamepadKeyType::G_Select         },
        { AKEYCODE_BUTTON_MODE,   Event::GamepadKeyType::G_Mode           },

        // Xbox360 controller dpad
        { 704,                    Event::GamepadKeyType::G_Left     },
        { 705,                    Event::GamepadKeyType::G_Right    },
        { 706,                    Event::GamepadKeyType::G_Up       },
        { 707,                    Event::GamepadKeyType::G_Down     },

        { 0,                      Event::K_NoKey                      }
};

AndroidApi::AndroidApi() {
  m_App = globalAppPtr;
  assert(m_App);
  Log::i("Waiting for window to be created...");

  m_App->userData = this;
  m_App->onAppCmd = [](struct android_app* app, int32_t cmd) {
    //std::lock_guard<std::mutex> lock(m_mutex);
    if (app->userData)
      ((AndroidApi*)app->userData)->onAppCmd(app, cmd);
  };

  m_App->onInputEvent = [](struct android_app* app, AInputEvent* event) {
    if (app->userData)
      return ((AndroidApi*)app->userData)->onInputEvent(app, event);
    return 0;
  };

  setupKeyTranslate(k,48);

  // Blocking loop until window is available. this happens only when app is started with screen on. so during debugging and screen off we need to wait until screen is unlocked for init boot sequence to work
  int events;
  android_poll_source* source;
  do {
    if (ALooper_pollAll(0, nullptr,
                        &events, (void**)&source) >= 0) {
      std::lock_guard<std::mutex> lock(m_mutex);
      if (source != NULL) source->process(m_App, source);
    }
  } while (m_App->destroyRequested == 0 && m_App->window == NULL);
  if (m_App->destroyRequested != 0)
  {
    return;
  }

  // m_eventThread = std::thread([this](){
  //                 while (!isExit.load()) {
  //                   if(implIsPaused())
  //                     std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  //                   processEvents();
  //                 }});
  // m_eventThread.detach();
   
  width = ANativeWindow_getWidth(m_App->window);
  height = ANativeWindow_getHeight(m_App->window);
}

AndroidApi::~AndroidApi() {
  //m_App->userData = nullptr;
  //ANativeWindow_release(m_App->window);
//  if (globalAppPtr) {
//    globalAppPtr->userData = nullptr;
//    globalAppPtr->onAppCmd = nullptr;
//    globalAppPtr->onInputEvent = nullptr;
//    globalAppPtr->running = 0;
//  }
  Log::i("AndroidApi and window destroyed"); // window stays on display
}

void AndroidApi::implRegisterApplication(Application *p) {
  auto existing = std::find(m_Apps.begin(),m_Apps.end(),p);
  if(existing==m_Apps.end())
    m_Apps.push_back(p);
}

void AndroidApi::implUnregisterApplication(Application *p) {
  auto existing = std::find(m_Apps.begin(),m_Apps.end(),p);
  if(existing!=m_Apps.end())
    m_Apps.erase(existing);
  // if(m_Apps.empty())
  //   SystemApi::shutdown();
}

void AndroidApi::onAppCmd(struct android_app* app, int32_t cmd)
{
  Log::i("Android NativeApp command: ", app_commands[cmd]);

  switch (cmd) {
      // unimplemented
      case APP_CMD_INPUT_CHANGED: break;
      case APP_CMD_WINDOW_RESIZED: break;
      case APP_CMD_WINDOW_REDRAW_NEEDED: break;
      case APP_CMD_CONTENT_RECT_CHANGED: break;
      case APP_CMD_CONFIG_CHANGED: break;
      // implemented
      case APP_CMD_START: break;
      case APP_CMD_RESUME: 
          isExit.store(false);
          loadSavedState();
      break;
      case APP_CMD_LOW_MEMORY: 
          saveAppState(); break;
      case APP_CMD_STOP:
        break;
      case APP_CMD_PAUSE: 
        break;
      case APP_CMD_SAVE_STATE:
          saveAppState(); break;
      case APP_CMD_INIT_WINDOW:
          width  = ANativeWindow_getWidth(app->window);
          height = ANativeWindow_getHeight(app->window);
          if (m_Window == NULL || m_Window != app->window)
          {
            m_renderThread = std::thread([this](){
                bool once=false;
                while (!isExit.load()) {
                  if(implIsRunning()) {
                    if (!once) {
                      std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // wait for main thread to be run
                      once=true;
                    }
                    render();
                  }
                  else
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }
            });
            for(auto& ew : app_windows)
            {
              ew.second = (SystemApi::Window*)app->window;
              ew.first->updateHandle((SystemApi::Window*)app->window);
              SizeEvent se(width, height); // recreate render stuff
              dispatchResize(*ew.first, se);
            }
          }
          else {
            Log::i("Init window called with same window handle");
          }
          Log::i("Android native window (",width,",",height,") created");
          m_Window = app->window;
          break;
      case APP_CMD_DESTROY:
          isExit.store(true);
          break;
      case APP_CMD_TERM_WINDOW:
          for(auto& ew : app_windows)
          {
            CloseEvent ce = CloseEvent();
            dispatchClose(*ew.first, ce);
            ew.first->updateHandle(nullptr);
          }
          break;
      case APP_CMD_GAINED_FOCUS:
          m_HasFocus.store(true); break;
      case APP_CMD_LOST_FOCUS:
          m_HasFocus.store(false); break;
  }
}

void AndroidApi::loadSavedState() {
  if (m_Window && app_windows.size()) {
    for(auto& w : app_windows)
    {
      AppStateEvent As(AppStateEvent::LoadState);
      dispatchAppState(*w.first,As);
    }
  }
}

void AndroidApi::saveAppState() {
  if (m_Window && app_windows.size()) {
    for(auto& w : app_windows)
    {
      AppStateEvent As(AppStateEvent::SaveState);
      dispatchAppState(*w.first,As);
    }
  }
}

void AndroidApi::dispatchMouseButton(int x, int y, int dir, const Event::MouseButton& button) {
  Log::i("Mouse button ", (int)button," ", dir > 0 ? "down" : "up");
  MouseEvent PressEvent(x, y, button, Event::Modifier::M_NoModifier, 0, 0,
                        dir > 0 ? Event::Type::MouseDown : Event::Type::MouseUp);
  for(auto& w : app_windows) {
    if (dir > 0)
      dispatchMouseDown(*w.first, PressEvent);
    else
      dispatchMouseUp(*w.first, PressEvent);
  }
}

Event::GamepadAxisType mapAxis(int android_axis) {
  // tested with xbox360 controller - should be moved to engine and only raw android api info should be routed
  switch (android_axis)
  {
    case AMOTION_EVENT_AXIS_X: return Event::GamepadAxisType::AxisLeftStickX;
    case AMOTION_EVENT_AXIS_Y: return Event::GamepadAxisType::AxisLeftStickY;
    case AMOTION_EVENT_AXIS_Z: return Event::GamepadAxisType::AxisLeftTrigger;
    case AMOTION_EVENT_AXIS_RZ: return Event::GamepadAxisType::AxisRightTrigger;
    case AMOTION_EVENT_AXIS_RX: return Event::GamepadAxisType::AxisRightStickX;
    case AMOTION_EVENT_AXIS_RY: return Event::GamepadAxisType::AxisRightStickY;

    default:  return Event::GamepadAxisType::AxisNone;
  }
}

int AndroidApi::onInputEvent(struct android_app* app, AInputEvent* event)
{
  bool backButtonPressed = false;
  int source = AInputEvent_getSource(event);
  int type   = AInputEvent_getType(event);
  int action = 0;
  if (source & 0x00001000  && !(source & 0x00002000) /*AINPUT_SOURCE_TOUCHSCREEN without mouse presses */)
  {
    int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
    //int pointer = (AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    switch (action) {
    case AMOTION_EVENT_ACTION_MOVE:
    case AMOTION_EVENT_ACTION_HOVER_MOVE:
    {
      int pointerCount = AMotionEvent_getPointerCount(event);
      // if pointer count greater 1 and pointerCount changes we need to associate the indices to the "former" id
      // they were assigned. so map it ourselves or use an awesome library that can handle that shit
      // otherwise the tempest pointer id window enter and leave tracking would not work or I just don't get it
      for (int pointerIndex = 0; pointerIndex < pointerCount; ++pointerIndex)
      {
        int x = AMotionEvent_getX(event, pointerIndex);
        int y = AMotionEvent_getY(event, pointerIndex);

        PointerEvent pe(pointerIndex,x,y,Event::Type::PointerMove);
        for(auto& w : app_windows) {
          dispatchPointerMove(*w.first, pe);
        }
      }
    }
    return 1;
    case AMOTION_EVENT_ACTION_DOWN:
    case AMOTION_EVENT_ACTION_POINTER_DOWN:
    {
      int pointerCount = AMotionEvent_getPointerCount(event);
      for (int pointerIndex = 0; pointerIndex < pointerCount; ++pointerIndex)
      {
        int x = AMotionEvent_getX(event, pointerIndex);
        int y = AMotionEvent_getY(event, pointerIndex);

        PointerEvent pe(pointerIndex,x,y,Event::Type::PointerDown);
        for(auto& w : app_windows) {
          dispatchPointerDown(*w.first, pe);
        }
      }
    }
    return 1;
    case AMOTION_EVENT_ACTION_UP:
    case AMOTION_EVENT_ACTION_POINTER_UP:
    {
      int pointerCount = AMotionEvent_getPointerCount(event);
      for (int pointerIndex = 0; pointerIndex < pointerCount; ++pointerIndex)
      {
        int x = AMotionEvent_getX(event, pointerIndex);
        int y = AMotionEvent_getY(event, pointerIndex);

        PointerEvent pe(pointerIndex,x,y,Event::Type::PointerUp);
        for(auto& w : app_windows) {
          dispatchPointerUp(*w.first, pe);
        }
      }
    }
    return 1;
    }
  }
  int keyCode = AKeyEvent_getKeyCode(event);
  if (keyCode == AKEYCODE_BACK || keyCode == AKEYCODE_SOFT_LEFT || keyCode == AKEYCODE_SOFT_RIGHT) {
    backButtonPressed = true;
    if (source & AINPUT_SOURCE_MOUSE)
    {
      int x = AMotionEvent_getX(event, 0);
      int y = AMotionEvent_getY(event, 0);
      dispatchMouseButton(x, y, 1, Event::MouseButton::ButtonRight);
      dispatchMouseButton(x, y, 0, Event::MouseButton::ButtonRight);
    }
    return 1;
  }
  if (source & AINPUT_SOURCE_TOUCHPAD || source & AINPUT_SOURCE_MOUSE || source & AINPUT_SOURCE_MOUSE_RELATIVE || source & AINPUT_SOURCE_BLUETOOTH_STYLUS || source & AINPUT_SOURCE_STYLUS || source & AINPUT_SOURCE_TRACKBALL)
  {
    Log::i("Input event mouse: s: %08X ", source);
    if (type == AINPUT_EVENT_TYPE_KEY)
    {
      action = AKeyEvent_getAction(event);
      int x = AMotionEvent_getX(event, 0);
      int y = AMotionEvent_getY(event, 0);
      Log::i("Mouse event key - source: ",source," type: ",type," action: ", action );
      int dir = action == AKEY_EVENT_ACTION_DOWN ? 1 : action == AKEY_EVENT_ACTION_UP ? -1 : 0;
      if (action == AMOTION_EVENT_BUTTON_PRIMARY) {
        dispatchMouseButton(x, y, dir, Event::MouseButton::ButtonLeft);
      }
      if (AMOTION_EVENT_BUTTON_SECONDARY) {
        dispatchMouseButton(x, y, dir, Event::MouseButton::ButtonRight);
      }
      if (AMOTION_EVENT_BUTTON_TERTIARY) {
        dispatchMouseButton(x, y, dir, Event::MouseButton::ButtonMid);
      }
      if (AMOTION_EVENT_BUTTON_BACK) {
        dispatchMouseButton(x, y, dir, Event::MouseButton::ButtonBack);
      }
      if (AMOTION_EVENT_BUTTON_FORWARD) {
        dispatchMouseButton(x, y, dir, Event::MouseButton::ButtonForward);
      }
    }
    else if (type == AINPUT_EVENT_TYPE_MOTION)
    {
      action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
      int x = AMotionEvent_getX(event, 0);
      int y = AMotionEvent_getY(event, 0);

      if (action == AMOTION_EVENT_ACTION_HOVER_MOVE || action == AMOTION_EVENT_ACTION_MOVE)
      {
        // relative mouse movement - not functional yet
        //x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RELATIVE_X, pointerIndex);
        //y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RELATIVE_Y, pointerIndex);

        MouseEvent MoveEvent(x,y);
        for(auto& w : app_windows) {
          dispatchMouseMove(*w.first, MoveEvent);
        }
        return 1;
      }
      // hover enter and exit not functional
      if (action == AMOTION_EVENT_ACTION_HOVER_ENTER || action == AMOTION_EVENT_ACTION_HOVER_EXIT )
      {
        implShowCursor(app_windows[0],action == AMOTION_EVENT_ACTION_HOVER_ENTER ? CursorShape::Arrow : CursorShape::Hidden);
        return 1;
      }
      bool handled = false;
      int pointerCount = AMotionEvent_getPointerCount(event);
      for (int pointerIndex = 0; pointerIndex < pointerCount; ++pointerIndex) {

        if(action == AMOTION_EVENT_ACTION_SCROLL)
        {
          int vscroll = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_VSCROLL, pointerIndex);
          Log::i("Mouse scroll - action: ", vscroll);
          MouseEvent ScrollEvent(x, y, Event::ButtonNone, Event::Modifier::M_NoModifier, vscroll, 0, Event::Type::MouseWheel);
          for(auto& w : app_windows)
          {
            dispatchMouseWheel(*w.first, ScrollEvent);
          }
          handled = true;
        }
        int dir = (action == AMOTION_EVENT_ACTION_BUTTON_PRESS || action == AMOTION_EVENT_ACTION_DOWN) ? 1 : (action == AMOTION_EVENT_ACTION_BUTTON_RELEASE || action == AMOTION_EVENT_ACTION_UP) ? -1 : 0;
        if (dir) {
          Log::i("Mouse button clicked - action: ",action);
          int button = AMotionEvent_getButtonState(event);
          // thank you Epic Games
          bool bDown = (action == AMOTION_EVENT_ACTION_DOWN);
          if (!bDown)
          {
            button = mouseButtonStatePrevious;
          }
          if (button & AMOTION_EVENT_BUTTON_PRIMARY) {
            dispatchMouseButton(x, y, dir, Event::MouseButton::ButtonLeft);
            handled = true;
          }
          if (button == AMOTION_EVENT_BUTTON_SECONDARY) {
            dispatchMouseButton(x, y, dir, Event::MouseButton::ButtonRight);
            handled = true;
          }
          if (action == AMOTION_EVENT_BUTTON_TERTIARY) {
            dispatchMouseButton(x, y, dir, Event::MouseButton::ButtonMid);
            handled = true;
          }
          if (action == AMOTION_EVENT_BUTTON_BACK) {
            dispatchMouseButton(x, y, dir, Event::MouseButton::ButtonBack);
            handled = true;
          }
          if (action == AMOTION_EVENT_BUTTON_FORWARD) {
            dispatchMouseButton(x, y, dir, Event::MouseButton::ButtonForward);
            handled = true;
          }
          mouseButtonStatePrevious = button;
        }
      }
      if (handled)
        return 1;

      if (!mouseInputActive) mouseInputActive = true;
      Log::i("Unhandled mouse motion - source:", source," type: ",type," action: ",action);
      return 1;
    }

    Log::i("Unhandled MOUSE-like input detected - source:", source," type: ",type," action: ",action);
    return 1;
  }
  // gamepad motions are joystick and button presses are gamepad - so pressing a button results in multiple event, a motion event and a button press
  if (source & AINPUT_SOURCE_JOYSTICK) {
    int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;

    if(action == AMOTION_EVENT_ACTION_MOVE) {
//      float axis0 = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0); // left analog stick x
//      float axis1 = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, 0); // left analog stick y
//      float axis2 = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z, 0); // left trigger
//      float axis3 = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ, 0); // right trigger
//      float axis4 = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RX, 0); // right analog stick x
//      float axis5 = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RY, 0); // right analog stick y
/*
      Log::i("---------------------");
*/

      for(size_t axis=0; axis < AMOTION_EVENT_AXIS_GENERIC_16; axis++)
      {
        if (mapAxis(axis)) {
          float axis_value = AMotionEvent_getAxisValue(event, axis, 0);

          if (axis_values[mapAxis(axis)] != axis_value) {
//            Log::i("Axis %2i: %f", axis, axis_value);
            axis_values[mapAxis(axis)] = axis_value;
//            if ()
//              AnalogEvent Event(mapAxis(axis), axis_value, 0);
          }
        }
      }

      return 1;
    }
    if(action == AMOTION_EVENT_ACTION_BUTTON_PRESS)
    {
      Log::i("Gamepad motion btn: s: %08X a: %08X", source,
                          action);
    }
    return 1;
  }
  if (source & 0x00000400 /*AINPUT_SOURCE_GAMEPAD*/) {
    int action = AKeyEvent_getAction(event);
    int scanCode = AKeyEvent_getScanCode(event);
    int keyCode = AKeyEvent_getKeyCode(event);
    int metaState = AKeyEvent_getMetaState(event);
    int key = SystemApi::translateKey(keyCode);
    if (key == Event::K_NoKey)
      key = SystemApi::translateKey(scanCode); // use scan code as it seems more right with the hardware (xbox360 controller) available
    Log::i("Gamepad event: s: ",source," a: ",action," sc: ",scanCode," kc: ",keyCode," k: ",key," ms: ", metaState);
    // AKEY_EVENT_ACTION_MULTIPLE are ignored
    Tempest::GamepadKeyEvent e(Event::GamepadKeyType(key), (action & AKEY_EVENT_ACTION_DOWN) ? Event::KeyDown : Event::KeyUp);
    for(auto& w : app_windows) {
    if(action == AKEY_EVENT_ACTION_DOWN)
      dispatchKeyDown(*w.first, e, scanCode); else
      dispatchKeyUp(*w.first, e, scanCode);
    }
    if(action == AKEY_EVENT_ACTION_MULTIPLE)
    {
      Log::i("Gamepad multievent: s: %08X a: %08X sc: %i kc: %i ms: %i", source,
                          action, scanCode, keyCode, metaState);
      return 1;
    }
    return 1;

  }
  if (source & AINPUT_SOURCE_KEYBOARD) {
    Log::i("Input event key: s: ", source);
      int action = AKeyEvent_getAction(event);
      int scanCode = AKeyEvent_getScanCode(event);
      int keyCode = AKeyEvent_getKeyCode(event);
      int metaState = AKeyEvent_getMetaState(event);
      int key = SystemApi::translateKey(keyCode);
      int ModifierState = Event::M_NoModifier;
      if (metaState & AMETA_ALT_ON) ModifierState &= Event::M_Alt;
      if (metaState & AMETA_SHIFT_ON) ModifierState &= Event::M_Shift;
      if (metaState & AMETA_CTRL_ON) ModifierState &= Event::M_Ctrl;

      if (keyCode == AKEYCODE_VOLUME_DOWN || keyCode == AKEYCODE_VOLUME_MUTE || keyCode == AKEYCODE_VOLUME_UP)
      {
        return 0;
      }
      if (keyCode == AKEYCODE_BACK || keyCode == AKEYCODE_SOFT_LEFT || keyCode == AKEYCODE_SOFT_RIGHT) {
        Log::i("Back button pressed: s: ",source," k: ", keyCode);
        // assuming we have a mouse attached, this is handled as right click. android usually interprets right click as back button
        // https://stackoverflow.com/questions/12130618/android-ndk-how-to-override-onbackpressed-in-nativeactivity-without-java
        if(mouseInputActive)
        {
          int pointerCount = AMotionEvent_getPointerCount(event);
          for (int pointerIndex = 0; pointerIndex < pointerCount; ++pointerIndex) {
            int x = AMotionEvent_getRawX(event, pointerIndex);
            int y = AMotionEvent_getRawY(event, pointerIndex);

            int dir = action == AKEY_EVENT_ACTION_DOWN ? 1 : action == AKEY_EVENT_ACTION_UP ? -1 : 0;
            if (dir != 0)
            {
              MouseEvent PressEvent(x, y, Event::ButtonRight, Event::Modifier::M_NoModifier, 0, 0, dir > 0 ? Event::Type::MouseDown : Event::Type::MouseUp);
              for(auto& w : app_windows)
              {
              if(dir>0)
                dispatchMouseDown(*w.first, PressEvent);
              else
                dispatchMouseUp(*w.first, PressEvent);
              }
            }
          }
        }

        return 1; // <-- prevent default handler
      };
      if (keyCode == AKEYCODE_HOME) {
        Log::i("Home button pressed: s: %08X ", source);
        // assuming we have a mouse attached, this is handled as middle click. android usually interprets middle click as home button
        if (mouseInputActive) {
          int pointerCount = AMotionEvent_getPointerCount(event);
          for (int pointerIndex = 0; pointerIndex < pointerCount; ++pointerIndex) {
            int x = AMotionEvent_getRawX(event, pointerIndex);
            int y = AMotionEvent_getRawY(event, pointerIndex);

            int dir = action == AKEY_EVENT_ACTION_DOWN ? 1 : action == AKEY_EVENT_ACTION_UP ? -1 : 0;
            if (dir != 0) {
              MouseEvent PressEvent(x, y, Event::ButtonMid, Event::Modifier::M_NoModifier, 0, 0,
                                    dir > 0 ? Event::Type::MouseDown : Event::Type::MouseUp);
              for(auto& w : app_windows) {
                if (dir > 0)
                  dispatchMouseDown(*w.first, PressEvent);
                else
                  dispatchMouseUp(*w.first, PressEvent);
              }
            }
          }
        }

        // AKEY_EVENT_ACTION_MULTIPLE would be better, we omit multiple event from android with this statement: (action & AKEY_EVENT_ACTION_DOWN) and not considering multiple
        Tempest::KeyEvent e(Event::KeyType(key), 0, (Event::Modifier) metaState,
                            (action & AKEY_EVENT_ACTION_DOWN) ? Event::KeyDown : Event::KeyUp);
        for(auto& w : app_windows) {
          if (action == AKEY_EVENT_ACTION_DOWN)
            dispatchKeyDown(*w.first, e, scanCode);
          else
            dispatchKeyUp(*w.first, e, scanCode);
        }
        return 1;
      }

      if (key)
      {
        // AKEY_EVENT_ACTION_MULTIPLE would be better, we omit multiple event from android with this statement: (action & AKEY_EVENT_ACTION_DOWN) and not considering multiple
        Tempest::KeyEvent e(Event::KeyType(key), 0, (Event::Modifier)metaState, (action & AKEY_EVENT_ACTION_DOWN) ? Event::KeyDown : Event::KeyUp);
        for(auto& w : app_windows)
        {
          if(action == AKEY_EVENT_ACTION_DOWN)
            dispatchKeyDown(*w.first, e, scanCode); else
            dispatchKeyUp  (*w.first, e, scanCode);
        }
        return 1;
      }
  }

  Log::i("Unhandled input event: s: ", source);
  return 0;
}

SystemApi::Window* AndroidApi::implCreateWindow(Tempest::Window* owner, uint32_t w, uint32_t h, const char* title) {
  app_windows[owner] = (SystemApi::Window*)m_Window;
  return (SystemApi::Window*)m_App->window;
}

SystemApi::Window* AndroidApi::implCreateWindow(Tempest::Window* owner, SystemApi::ShowMode sm, const char* /*title*/) {
  switch (sm)
  {
  case SystemApi::ShowMode::Hidden:
    //app_windows[owner] = (SystemApi::Window*)m_App->window;
    return (SystemApi::Window*)m_App->window;
    break;
  case SystemApi::ShowMode::Maximized:
  default:
    app_windows[owner] = (SystemApi::Window*)m_App->window;
    return (SystemApi::Window*)m_App->window;
    break;
  }
  return (SystemApi::Window*)m_App->window;
}

void AndroidApi::implDestroyWindow(SystemApi::Window* w) {
  for(auto& ew : app_windows)
  {
    if (ew.second == w) {
      ANativeWindow_release((ANativeWindow*)ew.second);
      app_windows.erase(ew.first);
      //ew.first->updateHandle(nullptr);
      //ew.second = nullptr;
    }
  }
}

void AndroidApi::implExit() {
  isExit.store(true);
  m_App->destroyRequested += 1;
}

Rect AndroidApi::implWindowClientRect(SystemApi::Window* /*w*/) {
  if (m_App->window) {
    return Rect(0, 0, width, height);
  }
  return Rect(0, 0, 0, 0);
}

bool AndroidApi::implSetAsFullscreen(SystemApi::Window* /*w*/, bool /*fullScreen*/) {
  return true;
}

bool AndroidApi::implIsFullscreen(SystemApi::Window* /*w*/) {
  return true;
}

void AndroidApi::implSetCursorPosition(SystemApi::Window* /*w*/, int /*x*/, int /*y*/) {
  // not implemented
}

void AndroidApi::implShowCursor(SystemApi::Window* /*w*/, CursorShape /*show*/) {
  // not implemented
}

bool AndroidApi::implIsRunning() {
  return !isExit.load() && (m_App->activityState == APP_CMD_START || m_App->activityState == APP_CMD_RESUME);
}

bool AndroidApi::implIsPaused() {
  return m_App->activityState != APP_CMD_START && m_App->activityState != APP_CMD_RESUME;
}

bool AndroidApi::implIsResumeRequested() {
  return m_App->activityState == APP_CMD_RESUME;
}

int AndroidApi::implExec(SystemApi::AppCallBack& cb) {
  while (!isExit.load()) {
    implProcessEvents(cb);
  }
  return 0;
}

void AndroidApi::inputHandling() {
  // gamepad analog stick handling
  Vec2 LeftStick(axis_values[Event::GamepadAxisType::AxisLeftStickX], axis_values[Event::GamepadAxisType::AxisLeftStickY]);
  Vec2 RightStick(axis_values[Event::GamepadAxisType::AxisRightStickX], axis_values[Event::GamepadAxisType::AxisRightStickY]);

  // Left stick direction
  Event::KeyType backForth;
  Event::KeyType leftRight;
  if (LeftStick.dotProduct(LeftStick, Vec2(0.0f, 1.0f)) > deadzone) {
    backForth = Event::KeyType::K_Down;
  }
  else if (LeftStick.dotProduct(LeftStick, Vec2(0.0f, -1.0f)) > deadzone) {
    backForth = Event::KeyType::K_Up;
  }
  if (LeftStick.dotProduct(LeftStick, Vec2(-1.0f, 0.0f)) > deadzone) {
    leftRight = Event::KeyType::K_Left;
  }
  else if (LeftStick.dotProduct(LeftStick, Vec2(1.0f, 0.0f)) > deadzone) {
    leftRight = Event::KeyType::K_Right;
  }

  if (RightStick.length() > deadzone_look) {
    AnalogEvent GamepadXAxis(Event::GamepadAxisType::AxisRightStickX, RightStick.x);
    AnalogEvent GamepadYAxis(Event::GamepadAxisType::AxisRightStickY, RightStick.y);
    for(auto& w : app_windows) {
      dispatchGamepadMove(*w.first, GamepadXAxis);
      dispatchGamepadMove(*w.first, GamepadYAxis);
    }
  }
  RightStickPrevious = RightStick;
}

void AndroidApi::processEvents() {
  // main message loop - run in its own thread to not block in case of recursive systemApi events that call implProcessEvents
  int events;
  android_poll_source* source;
  if(shouldProcess.load())
  {
    if (ALooper_pollAll(m_HasFocus.load() ? 1 : 0, nullptr, &events,
                        (void **) &source) >= 0) {
      if (source != NULL) source->process(m_App, source);
    }
  }
  inputHandling();
  shouldProcess.store(false);
}

void AndroidApi::implProcessEvents(SystemApi::AppCallBack& cb) {
  shouldProcess.store(true);
  processEvents();
  if (m_App->activityState == APP_CMD_START || m_App->activityState == APP_CMD_RESUME) {
    if (cb.onTimer() == 0)
      std::this_thread::yield();
  }
}

void AndroidApi::render() {
  if (m_App && m_Window && m_HasFocus.load()) {
    for (auto& i : app_windows) {
      if (i.second)
        SystemApi::dispatchRender(*i.first);
    }
  }
}

void AndroidApi::implShutdown() {
  if (m_App) {
    m_App->destroyRequested += 1;
    int events;
    android_poll_source* source;
    do {
      if (ALooper_pollAll(m_HasFocus.load() ? 1 : 0, nullptr, &events,
          (void **) &source) >= 0)
        {
          if (source != NULL) source->process(m_App, source);
        }
    }
    while (!isExit.load());
  }
}

#endif