#pragma once

#include <Tempest/AbstractGraphicsApi>
#include "vulkan_sdk.h"

#include "vrenderpass.h"

namespace Tempest {
namespace Detail {

class VSwapchain;

class VFramebufferLayout : public AbstractGraphicsApi::FboLayout {
  public:
    VFramebufferLayout(VDevice &dev, VSwapchain** sw, const VkFormat* attach, uint8_t attCount);
    VFramebufferLayout(VFramebufferLayout&& other) noexcept;
    ~VFramebufferLayout() override;

    VFramebufferLayout& operator =(VFramebufferLayout &&other) noexcept;

    VkRenderPass                   impl=VK_NULL_HANDLE;
    std::unique_ptr<VkFormat[]>    frm;
    std::unique_ptr<VSwapchain*[]> swapchain;
    uint8_t                        attCount=0;
    uint8_t                        colorCount=0;

    bool                        isCompatible(const VFramebufferLayout& other) const;
    bool                        equals(const FboLayout& other) const override;

  private:
    VkDevice     device=nullptr;
  };

}}
