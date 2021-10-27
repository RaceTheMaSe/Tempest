#pragma once

#include <Tempest/AbstractGraphicsApi>
#include "vulkan_sdk.h"

namespace Tempest {
namespace Detail {

class VDevice;

class VCommandPool {
  public:
    VCommandPool(VDevice &device, VkCommandPoolCreateFlags flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VCommandPool(VCommandPool&& other) noexcept ;
    ~VCommandPool();

    VkCommandPool impl=VK_NULL_HANDLE;

  private:
    VkDevice      device=nullptr;
  };

}}
