#pragma once

#include <Tempest/AbstractGraphicsApi>
#include <vector>
#include <string>

#include "vulkan_sdk.h"
#include "utility/spinlock.h"

namespace Tempest {
namespace Detail {

class VDevice;
class VCommandBuffer;
class VSemaphore;
class VFence;

class VulkanInstance {
  public:
    VulkanInstance(bool enableValidationLayers=true);
    ~VulkanInstance();

    std::vector<AbstractGraphicsApi::Props> devices() const;

    VkInstance*       instance;

    struct VkProp:Tempest::AbstractGraphicsApi::Props {
      uint32_t graphicsFamily=uint32_t(-1);
      uint32_t presentFamily =uint32_t(-1);

      size_t   nonCoherentAtomSize=0;
      size_t   bufferImageGranularity=0;

      bool     hasMemRq2        =false;
      bool     hasDedicatedAlloc=false;
      };

    static void      getDeviceProps(VkPhysicalDevice physicalDevice, VkProp& c);
    static void      getDevicePropsShort(VkPhysicalDevice physicalDevice, AbstractGraphicsApi::Props& c);


    void submit(VDevice *d, VCommandBuffer** cmd, size_t count, VFence *doneCpu);

  private:
    void implSubmit(VDevice *d,
                    VkCommandBuffer* command, VCommandBuffer** cmd, size_t count,
                    VkSemaphore* ws, VkPipelineStageFlags* wflg, size_t waitCnt,
                    VFence *fence);

    const std::vector<const char*>& checkValidationLayerSupport();
    bool layerSupport(const std::vector<VkLayerProperties>& sup,const std::initializer_list<const char*> dest);
    void addExternalLayers(std::vector<const char *> &activeLayers, const std::vector<VkLayerProperties> &supportedLayers);
    void addSupportedLayers(std::vector<const char *> &activeLayers, const std::vector<VkLayerProperties> &instanceLayers,
                            std::initializer_list<const char*> requestedLayers);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(
        VkDebugReportFlagsEXT       flags,
        VkDebugReportObjectTypeEXT  objectType,
        uint64_t                    object,
        size_t                      location,
        int32_t                     messageCode,
        const char*                 pLayerPrefix,
        const char*                 pMessage,
        void*                       pUserData);

    const bool                                      validation;
    VkDebugReportCallbackEXT                        callback={};
    PFN_vkDestroyDebugReportCallbackEXT             vkDestroyDebugReportCallbackEXT = nullptr;
    std::vector<std::string> externalLayers;
    std::vector<const char*> activeLayers;
    std::vector<const char*> extensions;

    void VkCheck(VkResult err);
  };

}}
