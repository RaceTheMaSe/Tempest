#pragma once

#include <Tempest/Texture2d>
#include <Tempest/Pixmap>
#include <Tempest/Rect>
#include "../gapi/rectalllocator.h"

#include <vector>
#include <mutex>
#include <atomic>

namespace Tempest {

class Device;
class Sprite;

class TextureAtlas {
  public:
    TextureAtlas(Device& device);
    TextureAtlas(const TextureAtlas&)=delete;
    virtual ~TextureAtlas();

    Sprite load(const Pixmap& pm);

  private:
    Device& device;

    struct Memory {
      Memory()=default;
      Memory(uint32_t w,uint32_t h):cpu(w,h,Pixmap::Format::RGBA){}
      Memory(Memory&&)=default;

      Memory& operator=(Memory&&)=default;

      Pixmap            cpu;
      mutable Texture2d gpu;
      mutable bool      changed=true;
      };

    struct MemoryProvider {
      using DeviceMemory=Memory;

      DeviceMemory alloc(uint32_t w,uint32_t h){
        DeviceMemory ret(w,h);
        return ret;
        }

      void free(DeviceMemory& m){
        // nop
        m=DeviceMemory();
        }
      };

    using Allocation = typename Tempest::RectAlllocator<MemoryProvider>::Allocation;

    MemoryProvider                          provider;
    Tempest::RectAlllocator<MemoryProvider> alloc;
    void emplace(Allocation& dest, const Pixmap& p, uint32_t x, uint32_t y);

  friend class Sprite;
  };

}
