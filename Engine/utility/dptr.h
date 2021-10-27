#pragma once

#include <utility>
#include <atomic>

namespace Tempest {

class DescriptorSet;
class CommandBuffer;
template<class T>
class Encoder;

namespace Detail {

template<class Handler>
class DPtr {
  public:
    DPtr()=default;
    DPtr(const DPtr&)=delete;
    DPtr(Handler h):handler(h){}
    DPtr(DPtr&& other) noexcept :handler(other.handler){ other.handler=Handler(); }
    ~DPtr()= default;

    DPtr& operator = (DPtr&& other) noexcept { std::swap(handler,other.handler); return *this; }
    DPtr& operator = (const DPtr& other) = delete;

    bool operator !() const { return !handler; }
    operator bool() const { return bool(handler); }

    Handler handler{};
  };

template<class Handler>
class DSharedPtr {
  public:
    DSharedPtr()=default;
    explicit DSharedPtr(Handler h):handler(h){
      if(handler)
        addRef();
      }

    DSharedPtr(const DSharedPtr& t):handler(t.handler){
      if(handler)
        addRef();
      }

    DSharedPtr(DSharedPtr&& t) noexcept :handler(t.handler) {
      t.handler=nullptr;
      }

    ~DSharedPtr(){
      if(handler)
        decRef();
      }

    DSharedPtr& operator=(DSharedPtr&& other) noexcept {
      std::swap(handler,other.handler);
      return *this;
      }

    DSharedPtr& operator=(const DSharedPtr& other){
      if(other.handler) {
        other.addRef();
        if(handler)
          decRef();
        handler=other.handler;
        } else {
        if(handler)
          decRef();
        handler=nullptr;
        }
      return *this;
      }

    bool operator !() const { return !handler; }
    operator bool() const { return bool(handler); }

    Handler handler{};

  private:
    void addRef() const {
      handler->counter.fetch_add(1,std::memory_order_relaxed);
      }

    void decRef() const {
      if(handler->counter.fetch_sub(1,std::memory_order_release)==1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete handler;
        }
      }
  };

template<class T>
class ResourcePtr {
  public:
    ResourcePtr()=default;
    ResourcePtr(const T& t):impl(t.impl){}

    bool operator !() const { return !impl; }
    operator bool() const { return bool(impl); }

    bool operator==(const ResourcePtr& other) const { return impl.handler==other.impl.handler; }
    bool operator!=(const ResourcePtr& other) const { return impl.handler!=other.impl.handler; }

  private:
    decltype(T::impl) impl;

  friend class Tempest::DescriptorSet;
  friend class Tempest::CommandBuffer;
  template<class TT>
  friend class Tempest::Encoder;
  };
}
}
