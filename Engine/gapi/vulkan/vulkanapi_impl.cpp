#include <string>
#if defined(TEMPEST_BUILD_VULKAN)
#include <initializer_list>

#include "vulkanapi_impl.h"

#include <Tempest/Log>
#include <Tempest/Platform>

#include "exceptions/exception.h"
#include "vdevice.h"

#include <set>
#include <thread>
#include <cstring>
#include <array>

#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME   "VK_KHR_win32_surface"
#define VK_KHR_XLIB_SURFACE_EXTENSION_NAME    "VK_KHR_xlib_surface"
#define VK_KHR_ANDROID_SURFACE_EXTENSION_NAME "VK_KHR_android_surface"

#if defined(__WINDOWS__)
#define SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#elif defined(__ANDROID__)
#define SURFACE_EXTENSION_NAME VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
#elif defined(__LINUX__)
#define SURFACE_EXTENSION_NAME VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#endif

using namespace Tempest::Detail;

static const std::initializer_list<const char*> validationLayersKHR = {
  "VK_LAYER_KHRONOS_validation"
  };

static const std::initializer_list<const char*> validationLayersLunarg = {
  "VK_LAYER_LUNARG_core_validation"
  };

#if !defined(ANDROID)
static const std::initializer_list<const char*> pMetaLayers = {
        "VK_LAYER_LUNARG_standard_validation",
};
#endif

static const std::initializer_list<const char*> pValidationLayers = {
        "VK_LAYER_GOOGLE_threading",       "VK_LAYER_LUNARG_parameter_validation", "VK_LAYER_LUNARG_object_tracker",
        "VK_LAYER_LUNARG_core_validation", "VK_LAYER_LUNARG_device_limits",        "VK_LAYER_LUNARG_image",
        "VK_LAYER_LUNARG_swapchain",       "VK_LAYER_GOOGLE_unique_objects",
};


/// @brief Helper function to add external layers to a list of active ones.
/// @param activeLayers List of active layers to be used.
/// @param supportedLayers List of supported layers.
inline void VulkanInstance::addExternalLayers(std::vector<const char *> &activeLayers,
                              const std::vector<VkLayerProperties> &supportedLayers)
{
  for (auto &layer : externalLayers)
  {
    for (auto &supportedLayer : supportedLayers)
    {
      if (layer == (const char*)supportedLayer.layerName)
      {
        activeLayers.push_back((const char*)supportedLayer.layerName);
        Tempest::Log::i("Found external layer: %s\n", supportedLayer.layerName);
        break;
      }
    }
  }
}

void VulkanInstance::addSupportedLayers(std::vector<const char *> &activeLayers, const std::vector<VkLayerProperties> &instanceLayers,
                             std::initializer_list<const char*> requestedLayers)
{
  for(const auto& layer : requestedLayers)
  {
    for (auto &ext : instanceLayers)
    {
      if (strcmp((const char*)ext.layerName, layer) == 0)
      {
        activeLayers.push_back(layer);
        break;
      }
    }
  }
}

#define VkCheck(x) {                                                                          \
  do {                                                                                        \
    VkResult err = x;                                                                         \
    if (err) {                                                                                \
      Log::e("Detected Vulkan error ", err, " at ", __FILE__, ":", __LINE__, "."); \
      abort();                                                                                \
    }                                                                                         \
  } while (0);                                                                                \
}                                                                                             \

VulkanInstance::VulkanInstance(bool validation)
  :validation(validation) {
  std::vector<const char *> validationLayers = {};
  if (validation) {
    validationLayers = checkValidationLayerSupport();
    if (validationLayers.size() == 0)
      Log::d("VulkanApi: no validation layers available");
  }

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "OpenGothic";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "Tempest";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;
  appInfo.pNext = nullptr;

  VkInstanceCreateInfo createInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  createInfo.pApplicationInfo = &appInfo;
  createInfo.pNext = nullptr;
  createInfo.flags = 0;

  extensions = std::vector<const char *>{
          VK_KHR_SURFACE_EXTENSION_NAME,
          SURFACE_EXTENSION_NAME,
          VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
          VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
          VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME
          //VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME,
          //VK_EXT_VALIDATION_FLAGS_EXTENSION_NAME,
          //VK_EXT_VALIDATION_CACHE_EXTENSION_NAME
  };

  uint32_t instanceExtensionCount = 0;
  std::vector<const char *> activeInstanceExtensions;
  VkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));
  std::vector<VkExtensionProperties> instanceExtensions(instanceExtensionCount);
  VkCheck(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount,
                                                 instanceExtensions.data()));

  if (validation) {
    for (const auto &instanceExt : instanceExtensions)
      Log::e("Instance extension: ", instanceExt.extensionName);

    uint32_t instanceLayerCount = 0;
    VkCheck(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));
    std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
    VkCheck(vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data()));

    // A layer could have VK_EXT_debug_report extension.
//    for (auto &layer : instanceLayers) {
//      uint32_t count;
//      VkCheck(vkEnumerateInstanceExtensionProperties(layer.layerName, &count, nullptr));
//      std::vector<VkExtensionProperties> extensions(count);
//      VkCheck(vkEnumerateInstanceExtensionProperties(layer.layerName, &count, extensions.data()));
//      for (auto &ext : extensions) {
//        if(std::find_if(instanceExtensions.begin(),instanceExtensions.end(),[&](VkExtensionProperties& ep){
//            return ep.extensionName == ext.extensionName;
//        })==instanceExtensions.end())
//          instanceExtensions.push_back(ext);
//      }
//    }

    // On desktop, the LunarG loader exposes a meta-layer that combines all
    // relevant validation layers.
    std::vector<const char *> activeLayers;
#if !defined(ANDROID)
    addSupportedLayers(activeLayers, instanceLayers, pMetaLayers);
#endif

    // On Android, add all relevant layers one by one.
    if (activeLayers.empty()) {
      addSupportedLayers(activeLayers, instanceLayers, pValidationLayers);
    }

    if (activeLayers.empty())
      Log::e("Did not find validation layers.\n");
    else
      Log::e("Found validation layers!\n");

    addExternalLayers(activeLayers, instanceLayers);
    createInfo.enabledLayerCount = (uint32_t)activeLayers.size();
    createInfo.ppEnabledLayerNames = activeLayers.data(); // FIXME: no stack memory
  }
//
//  std::array<const char*, 20> vlout;
//  for (size_t idx = 0; idx < activeLayers.size(); idx++) {
//    vlout[idx] = activeLayers[idx];
//  }

  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();
  if (validation) {
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = activeLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;
  }

  instance = static_cast<VkInstance *>(malloc(sizeof(VkInstance)));

  VkResult ret = vkCreateInstance(&createInfo,nullptr,instance);
  if(ret==VK_ERROR_EXTENSION_NOT_PRESENT) {
    Log::e("Vulkan create instance failed - extension not present - turn off validation");
    //throw std::system_error(Tempest::GraphicsErrc::NoDevice);
  }
  else if(ret!=VK_SUCCESS) {
    Log::e("Vulkan create instance failed: ", ret);
    throw std::system_error(Tempest::GraphicsErrc::NoDevice);
  }

  if(validation) {
    auto vkCreateDebugReportCallbackEXT = PFN_vkCreateDebugReportCallbackEXT (vkGetInstanceProcAddr(*instance,"vkCreateDebugReportCallbackEXT"));
    vkDestroyDebugReportCallbackEXT     = PFN_vkDestroyDebugReportCallbackEXT(vkGetInstanceProcAddr(*instance,"vkDestroyDebugReportCallbackEXT"));

    /* Setup callback creation information */
    VkDebugReportCallbackCreateInfoEXT callbackCreateInfo;
    callbackCreateInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    callbackCreateInfo.pNext       = nullptr;
    callbackCreateInfo.flags       = VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                                     VK_DEBUG_REPORT_DEBUG_BIT_EXT |
                                     VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                     VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                     VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    callbackCreateInfo.pfnCallback = &debugReportCallback;
    callbackCreateInfo.pUserData   = nullptr;

    /* Register the callback */
    VkResult result = vkCreateDebugReportCallbackEXT(*instance, &callbackCreateInfo, nullptr, &callback);
    if(result!=VK_SUCCESS)
      Log::e("unable to setup validation callback");
    }
  }

VulkanInstance::~VulkanInstance(){
  if(vkDestroyDebugReportCallbackEXT)
    vkDestroyDebugReportCallbackEXT(*instance,callback,nullptr);
  vkDestroyInstance(*instance,nullptr);
  }

const std::vector<const char*>& VulkanInstance::checkValidationLayerSupport() {
  uint32_t layerCount=0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

#if defined(ANDROID)
//  if(layerSupport(availableLayers,validationLayersArmMali))
//    activeLayers.insert(activeLayers.end(),validationLayersArmMali.begin(),validationLayersArmMali.end());
#endif

  if(layerSupport(availableLayers,validationLayersKHR))
    activeLayers.insert(activeLayers.end(),validationLayersKHR.begin(),validationLayersKHR.end());

  if(layerSupport(availableLayers,validationLayersLunarg))
    activeLayers.insert(activeLayers.end(),validationLayersLunarg.begin(),validationLayersLunarg.end());

  return activeLayers;
  }

bool VulkanInstance::layerSupport(const std::vector<VkLayerProperties>& sup,
                                 const std::initializer_list<const char*> dest) {
  for(auto& i:dest) {
    bool found=false;
    for(auto& r:sup)
      if(std::strcmp((const char*)r.layerName,i)==0) {
        found = true;
        break;
        }
    if(!found)
      return false;
    }
  return true;
  }

std::vector<Tempest::AbstractGraphicsApi::Props> VulkanInstance::devices() const {
  std::vector<Tempest::AbstractGraphicsApi::Props> devList;
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);

  if(deviceCount==0)
    return devList;

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(*instance, &deviceCount, devices.data());

  devList.resize(devices.size());
  for(size_t i=0;i<devList.size();++i) {
    getDevicePropsShort(devices[i],devList[i]);
    }
  return devList;
  }

void VulkanInstance::getDeviceProps(VkPhysicalDevice physicalDevice, VkProp& c) {
  getDevicePropsShort(physicalDevice,c);

  VkPhysicalDeviceProperties prop={};
  vkGetPhysicalDeviceProperties(physicalDevice,&prop);
  c.nonCoherentAtomSize = size_t(prop.limits.nonCoherentAtomSize);
  if(c.nonCoherentAtomSize==0)
    c.nonCoherentAtomSize=1;

  c.bufferImageGranularity = size_t(prop.limits.bufferImageGranularity);
  if(c.bufferImageGranularity==0)
    c.bufferImageGranularity=1;
  }

void VulkanInstance::getDevicePropsShort(VkPhysicalDevice physicalDevice, Tempest::AbstractGraphicsApi::Props& c) {
  /*
   * formats support table: https://vulkan.lunarg.com/doc/view/1.0.30.0/linux/vkspec.chunked/ch31s03.html
   */
  VkFormatFeatureFlags imageRqFlags    = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_BLIT_DST_BIT|VK_FORMAT_FEATURE_BLIT_SRC_BIT;
  VkFormatFeatureFlags imageRqFlagsBC  = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
  VkFormatFeatureFlags attachRqFlags   = VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT;
  VkFormatFeatureFlags depthAttflags   = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
  VkFormatFeatureFlags storageAttflags = VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;

  VkPhysicalDeviceProperties prop={};
  vkGetPhysicalDeviceProperties(physicalDevice,&prop);

  VkPhysicalDeviceFeatures supportedFeatures={};
  vkGetPhysicalDeviceFeatures(physicalDevice,&supportedFeatures);

  std::memcpy((char*)c.name,(char*)prop.deviceName,sizeof(c.name));

  c.vbo.maxAttribs    = size_t(prop.limits.maxVertexInputAttributes);
  c.vbo.maxRange      = size_t(prop.limits.maxVertexInputBindingStride);

  c.ibo.maxValue      = size_t(prop.limits.maxDrawIndexedIndexValue);

  c.ssbo.offsetAlign  = size_t(prop.limits.minUniformBufferOffsetAlignment);
  c.ssbo.maxRange     = size_t(prop.limits.maxStorageBufferRange);

  c.ubo.offsetAlign   = size_t(prop.limits.minUniformBufferOffsetAlignment);
  c.ubo.maxRange      = size_t(prop.limits.maxUniformBufferRange);
  
  c.push.maxRange     = size_t(prop.limits.maxPushConstantsSize);

  c.anisotropy        = supportedFeatures.samplerAnisotropy;
  c.maxAnisotropy     = prop.limits.maxSamplerAnisotropy;
  c.tesselationShader = supportedFeatures.tessellationShader;
  c.geometryShader    = supportedFeatures.geometryShader;

  c.storeAndAtomicVs  = supportedFeatures.vertexPipelineStoresAndAtomics;
  c.storeAndAtomicFs  = supportedFeatures.fragmentStoresAndAtomics;

  c.mrt.maxColorAttachments = prop.limits.maxColorAttachments;

  c.compute.maxGroups.x = (int)prop.limits.maxComputeWorkGroupCount[0];
  c.compute.maxGroups.y = (int)prop.limits.maxComputeWorkGroupCount[1];
  c.compute.maxGroups.z = (int)prop.limits.maxComputeWorkGroupCount[2];

  c.compute.maxGroupSize.x = (int)prop.limits.maxComputeWorkGroupSize[0];
  c.compute.maxGroupSize.y = (int)prop.limits.maxComputeWorkGroupSize[1];
  c.compute.maxGroupSize.z = (int)prop.limits.maxComputeWorkGroupSize[2];

  c.tex2d.maxSize = prop.limits.maxImageDimension2D;

  switch(prop.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
      c.type = AbstractGraphicsApi::DeviceType::Cpu;
      break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
      c.type = AbstractGraphicsApi::DeviceType::Virtual;
      break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      c.type = AbstractGraphicsApi::DeviceType::Integrated;
      break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      c.type = AbstractGraphicsApi::DeviceType::Discrete;
      break;
    default:
      c.type = AbstractGraphicsApi::DeviceType::Unknown;
      break;
    }

  uint64_t smpFormat=0, attFormat=0, dattFormat=0, storFormat=0;
  for(uint32_t i=0;i<TextureFormat::Last;++i){
    VkFormat f = Detail::nativeFormat(TextureFormat(i));

    VkFormatProperties frm={};
    vkGetPhysicalDeviceFormatProperties(physicalDevice,f,&frm);
    if(isCompressedFormat(TextureFormat(i))){
      if((frm.optimalTilingFeatures & imageRqFlagsBC)==imageRqFlagsBC){
        smpFormat |= (1ull<<i);
        }
      } else {
      if((frm.optimalTilingFeatures & imageRqFlags)==imageRqFlags){
        smpFormat |= (1ull<<i);
        }
      }
    if((frm.optimalTilingFeatures & attachRqFlags)==attachRqFlags){
      attFormat |= (1ull<<i);
      }
    if((frm.optimalTilingFeatures & depthAttflags)==depthAttflags){
      dattFormat |= (1ull<<i);
      }
    if((frm.optimalTilingFeatures & storageAttflags)==storageAttflags){
      storFormat |= (1ull<<i);
      }
    }
  c.setSamplerFormats(smpFormat);
  c.setAttachFormats (attFormat);
  c.setDepthFormats  (dattFormat);
  c.setStorageFormats(storFormat);
  }

void VulkanInstance::submit(VDevice* dev, VCommandBuffer** cmd, size_t count, VFence* doneCpu) {
  size_t waitCnt = 0;
  for(size_t i=0; i<count; ++i) {
    for(auto& s:cmd[i]->swapchainSync) {
      if(s->state!=Detail::VSwapchain::S_Pending)
        continue;
      s->state = Detail::VSwapchain::S_Draw0;
      ++waitCnt;
      }
    }

  VkCommandBuffer                    cxStk[32] = {};
  std::unique_ptr<VkCommandBuffer[]> cxHeap;
  auto*                              cx = (VkCommandBuffer*)cxStk;
  if(count>32) {
    cxHeap.reset(new VkCommandBuffer[count]);
    cx = cxHeap.get();
    }

  VkSemaphore                             wxStk [32] = {};
  VkPipelineStageFlags                    flgStk[32] = {};
  std::unique_ptr<VkSemaphore[]>          wxHeap;
  std::unique_ptr<VkPipelineStageFlags[]> flgHeap;
  auto                                    wx  = (VkSemaphore*)wxStk;
  auto                                    flg = (VkPipelineStageFlags*)flgStk;

  if(waitCnt>32) {
    flgHeap.reset(new VkPipelineStageFlags[waitCnt]);
    wxHeap .reset(new VkSemaphore[waitCnt]);
    wx  = wxHeap.get();
    flg = flgHeap.get();
    }

  implSubmit(dev, cx, cmd, count,
             wx, flg, waitCnt,
             doneCpu);
  }

void VulkanInstance::implSubmit(VDevice* dx,
                                VkCommandBuffer* command, VCommandBuffer** cmd, size_t count,
                                VkSemaphore* wait, VkPipelineStageFlags* waitStages, size_t waitCnt,
                                VFence* fence) {
  size_t waitId = 0;
  for(size_t i=0; i<count; ++i) {
    command[i] = cmd[i]->impl;
    for(auto& s:cmd[i]->swapchainSync) {
      if(s->state!=Detail::VSwapchain::S_Draw0)
        continue;
      s->state = Detail::VSwapchain::S_Draw1;
      wait[waitId] = s->aquire;
      ++waitId;
      }
    }

  for(size_t i=0; i<waitCnt; ++i) {
    // NOTE: our sw images are draw-only
    waitStages[i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  submitInfo.waitSemaphoreCount   = uint32_t(waitCnt);
  submitInfo.pWaitSemaphores      = wait;
  submitInfo.pWaitDstStageMask    = waitStages;

  submitInfo.commandBufferCount   = uint32_t(count);
  submitInfo.pCommandBuffers      = command;

  submitInfo.signalSemaphoreCount = 0;
  submitInfo.pSignalSemaphores    = nullptr;

  if(fence!=nullptr)
    fence->reset();

  dx->dataMgr().wait();
  dx->graphicsQueue->submit(1, &submitInfo, fence==nullptr ? VK_NULL_HANDLE : fence->impl);
  }

VkBool32 VulkanInstance::debugReportCallback(VkDebugReportFlagsEXT      flags,
                                            VkDebugReportObjectTypeEXT objectType,
                                            uint64_t                   object,
                                            size_t                     location,
                                            int32_t                    messageCode,
                                            const char                *pLayerPrefix,
                                            const char                *pMessage,
                                            void                      *pUserData) {
#if VK_HEADER_VERSION==135
  // https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/1712
  if(objectType==VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT)
    return VK_FALSE;
#endif
  // // some errors are flooding the log, so we just print them once for every message type
  // static std::vector<int32_t> messageCodesReceived;
  // if(std::find(messageCodesReceived.begin(),messageCodesReceived.end(),messageCode)!=messageCodesReceived.end())
  //   return VK_TRUE;
  // messageCodesReceived.emplace_back(messageCode);
  std::string msg(pMessage);
  if(msg.find("VUID-vkCmdBeginRenderPass")!=std::string::npos)
    Log::i("I AM STUPID");
  Log::e(pMessage," object=",object,", type=",objectType," th:",std::this_thread::get_id());
  return VK_TRUE;
  }

#endif
