#pragma once

#include <cstdint>
#include <memory>

namespace Tempest {

class Timer;
class Style;
class Font;

class Application {
  public:
    Application();
    Application(const Application&)=delete;
    virtual ~Application();

    static void         sleep(unsigned int msec);
    static uint64_t     tickCount();

    int                 exec();
    static bool         isRunning();
    static bool         isPaused();
    static bool         isResumeRequested();
    static void         processEvents();

    static void         setStyle(const Style* stl);
    static const Style& style();

    static void         setFont(const Font& fnt);
    static const Font&  font();

  private:
    struct Impl;
    static Impl impl;

    static void     implAddTimer(Timer& t);
    static void     implDelTimer(Timer& t);

  friend class Timer;
  };

}
