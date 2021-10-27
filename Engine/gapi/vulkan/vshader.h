#pragma once

#include <Tempest/AbstractGraphicsApi>
#include <Tempest/PipelineLayout>

#include "gapi/shaderreflection.h"
#include "vulkan_sdk.h"

namespace Tempest {
namespace Detail {

class VDevice;

class VShader:public AbstractGraphicsApi::Shader {
  public:
    VShader(VDevice& device, const void* source, size_t src_size);
    ~VShader() override;

    using Binding = ShaderReflection::Binding;

    VkShaderModule                   impl;
    std::vector<Decl::ComponentType> vdecl;
    std::vector<Binding>             lay;

  private:
    VkDevice                         device;
  };

}}
