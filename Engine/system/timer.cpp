#include "timer.h"

#include <Tempest/Application>

using namespace Tempest;

Timer::Timer() = default;

Timer::~Timer() {
  setRunning(false);
  }

void Timer::start(uint64_t t) {
  m.interval = t;
  setRunning(true);
  }

void Timer::stop() {
  setRunning(false);
  }

void Timer::setRunning(bool b) {
  if(m.running==b)
    return;
  if(b) {
    Application::implAddTimer(*this);
    m.lastEmit = Application::tickCount();
    } else {
    m.interval=0;
    Application::implDelTimer(*this);
    }
  m.running = b;
  }

bool Timer::process(uint64_t now) {
  if(now>=m.lastEmit && now-m.lastEmit>=m.interval && m.interval>0){
    m.lastEmit += m.interval;
    timeout();
    return true;
    }
  return false;
  }
