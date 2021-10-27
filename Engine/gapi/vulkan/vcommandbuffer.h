#pragma once

#include <Tempest/AbstractGraphicsApi>
#include "vulkan_sdk.h"

#include "gapi/resourcestate.h"
#include "vcommandpool.h"
#include "vframebuffer.h"
#include "vswapchain.h"
#include "../utility/dptr.h"

namespace Tempest {
namespace Detail {

class VRenderPass;
class VFramebuffer;
class VFramebufferLayout;

class VDevice;
class VCommandPool;

class VDescriptorArray;
class VPipeline;
class VBuffer;
class VTexture;

class VCommandBuffer:public AbstractGraphicsApi::CommandBuffer {
  public:
    enum RpState : uint8_t {
      NoRecording,
      NoPass,
      RenderPass
      };

    VCommandBuffer()=delete;
    VCommandBuffer(VDevice &device, VkCommandPoolCreateFlags flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    ~VCommandBuffer() override;

    VkCommandBuffer                impl=nullptr;
    std::vector<VSwapchain::Sync*> swapchainSync;

    void reset() override;

    void begin() override;
    void begin(VkCommandBufferUsageFlags flg);
    void end() override;
    bool isRecording() const override;

    void beginRenderPass(AbstractGraphicsApi::Fbo* f,
                         AbstractGraphicsApi::Pass*  p,
                         uint32_t width,uint32_t height) override;
    void endRenderPass() override;

    void setViewport(const Rect& r) override;

    void setPipeline(AbstractGraphicsApi::Pipeline& p) override;
    void setBytes   (AbstractGraphicsApi::Pipeline& p, const void* data, size_t size) override;
    void setUniforms(AbstractGraphicsApi::Pipeline& p, AbstractGraphicsApi::Desc &u) override;

    void setComputePipeline(AbstractGraphicsApi::CompPipeline& p) override;
    void setBytes   (AbstractGraphicsApi::CompPipeline& p, const void* data, size_t size) override;
    void setUniforms(AbstractGraphicsApi::CompPipeline& p, AbstractGraphicsApi::Desc &u) override;

    void draw       (const AbstractGraphicsApi::Buffer& vbo, size_t  offset, size_t size, size_t firstInstance, size_t instanceCount) override;
    void drawIndexed(const AbstractGraphicsApi::Buffer& ivbo, const AbstractGraphicsApi::Buffer& iibo, Detail::IndexClass cls,
                     size_t ioffset, size_t isize, size_t voffset, size_t firstInstance, size_t instanceCount) override;
    void dispatch   (size_t x, size_t y, size_t z) override;

    void changeLayout(AbstractGraphicsApi::Buffer&  buf, BufferLayout  prev, BufferLayout  next) override;
    void changeLayout(AbstractGraphicsApi::Attach&  img, TextureLayout prev, TextureLayout next, bool byRegion) override;
    void changeLayout(AbstractGraphicsApi::Texture& tex, TextureLayout prev, TextureLayout next, uint32_t mipId);

    void copy(AbstractGraphicsApi::Buffer& dest, TextureLayout defLayout, uint32_t width, uint32_t height, uint32_t mip, AbstractGraphicsApi::Texture& src, size_t offset) override;
    void generateMipmap(AbstractGraphicsApi::Texture& image, TextureLayout defLayout, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels) override;

    void copy(AbstractGraphicsApi::Texture& dest, size_t width, size_t height, size_t mip, const AbstractGraphicsApi::Buffer&  src, size_t offset);
    void copy(AbstractGraphicsApi::Buffer&  dest, size_t offsetDest, const AbstractGraphicsApi::Buffer& src, size_t offsetSrc, size_t size);

    void blit(AbstractGraphicsApi::Texture& src, uint32_t srcW, uint32_t srcH, uint32_t srcMip,
              AbstractGraphicsApi::Texture& dst, uint32_t dstW, uint32_t dstH, uint32_t dstMip);

  private:
    void implCopy(AbstractGraphicsApi::Buffer&  dest, size_t width, size_t height, size_t mip,
                  const AbstractGraphicsApi::Texture& src, size_t offset);
    void implChangeLayout(VkImage dest, VkFormat imageFormat,
                          VkImageLayout oldLayout, VkImageLayout newLayout, bool discardOld,
                          uint32_t mipBase, uint32_t mipCount, bool byRegion);

    void addDependency(VSwapchain& s, size_t imgId);

    VDevice&                                device;
    VCommandPool                            pool;

    ResourceState                           resState;

    RpState                                 state        = NoRecording;
    VFramebuffer*                           curFbo       = nullptr;
    VRenderPass*                            curRp        = nullptr;
    VDescriptorArray*                       curUniforms  = nullptr;
    VkBuffer                                curVbo       = VK_NULL_HANDLE;
    bool                                    ssboBarriers = false;
    bool                                    isInCompute  = false;
  };

}}
