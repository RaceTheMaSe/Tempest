#pragma once

#include <Tempest/AbstractGraphicsApi>
#include "vulkan_sdk.h"

#include "vallocator.h"

namespace Tempest {
namespace Detail {

class VBuffer : public AbstractGraphicsApi::Buffer {
  public:
    VBuffer()=default;
    VBuffer(VBuffer &&other) noexcept ;
    ~VBuffer() override;

    VBuffer& operator=(VBuffer&& other) noexcept ;

    void update  (const void* data, size_t off, size_t count, size_t sz, size_t alignedSz) override;
    void read    (void* data, size_t off, size_t sz) override;

    VkBuffer               impl=VK_NULL_HANDLE;

  private:
    VAllocator*            alloc=nullptr;
    VAllocator::Allocation page={};

  friend class VAllocator;
  };

}}
