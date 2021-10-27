#include "application.h"

#include <Tempest/SystemApi>
#include <Tempest/Timer>
#include <Tempest/Style>
#include <Tempest/Font>

#include <vector>
#include <thread>
#include <chrono>

using namespace Tempest;

struct Application::Impl : SystemApi::AppCallBack {
  std::vector<Timer*> timer;
  size_t              timerI=size_t(-1);

  const Style*        style=nullptr;
  Font                font;

  void addTimer(Timer& t){
    timer.push_back(&t);
    }

  void delTimer(Timer& t){
    for(size_t i=0;i<timer.size();++i)
      if(timer[i]==&t){
        timer.erase(timer.begin()+int(i));
        if(timerI>=i)
          timerI--;
        return;
        }
    }

  uint32_t onTimer() override {
    auto now = Application::tickCount();
    size_t count=0;
    for(timerI=0;timerI<timer.size();++timerI){
      Timer& t = *timer[timerI];
      if(t.process(now))
        count++;
      }
    return uint32_t(count);
    }

  void setStyle(const Style* s) {
    if(style!=nullptr)
      style->implDecRef();
    style = s;
    if(style!=nullptr)
      style->implAddRef();
    }
  };

Application::Impl Application::impl = {};

Application::Application() {
  SystemApi::registerApplication(this);
  }

Application::~Application(){
  SystemApi::unregisterApplication(this);
  impl.font = Font();
//  SystemApi::shutdown();
  }

void Application::sleep(unsigned int msec) {
  std::this_thread::sleep_for(std::chrono::milliseconds(msec));
  }

uint64_t Application::tickCount() {
  auto now = std::chrono::steady_clock::now();
  auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
  return uint64_t(ms.time_since_epoch().count())-uint64_t(INT64_MIN);
  }

int Application::exec(){
  return SystemApi::exec(impl);
  }

bool Application::isRunning() {
  return SystemApi::isRunning();
  }

bool Application::isPaused() {
  return SystemApi::isPaused();
  }

bool Application::isResumeRequested() {
  return SystemApi::isResumeRequested();
  }

void Application::processEvents() {
  SystemApi::processEvent(impl);
  }

void Application::setStyle(const Style* stl) {
  impl.setStyle(stl);
  }

const Style& Application::style() {
  if(impl.style!=nullptr) {
    return *impl.style;
    }
  static Style def;
  return def;
  }

void Application::setFont(const Font& fnt) {
  impl.font = fnt;
  }

const Font& Application::font() {
  return impl.font;
  }

void Application::implAddTimer(Timer &t) {
  impl.addTimer(t);
  }

void Application::implDelTimer(Timer &t) {
  impl.delTimer(t);
  }
