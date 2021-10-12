#pragma once

#include "videobuffer.h"

namespace Tempest {

template<class T>
class UniformBuffer final {
  public:
    UniformBuffer()=default;
    UniformBuffer(UniformBuffer&&) noexcept =default;
    UniformBuffer& operator=(UniformBuffer&&) noexcept =default;

    void   update(const T* data,size_t offset,size_t size) { return impl.update(data,offset,size,sizeof(T),alignedTSZ); }
    size_t size() const { return arrTSZ; }

  private:
    UniformBuffer(Tempest::VideoBuffer&& implIn, size_t alignedTSZIn)
      :alignedTSZ(alignedTSZIn), arrTSZ(implIn.size()/alignedTSZIn), impl(std::move(implIn)) {
      }

    size_t               alignedTSZ=0;
    size_t               arrTSZ=0;
    Tempest::VideoBuffer impl;

  friend class Tempest::Device;
  friend class Tempest::CommandBuffer;
  friend class Tempest::DescriptorSet;
  };

}
