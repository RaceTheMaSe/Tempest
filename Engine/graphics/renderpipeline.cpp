#include "renderpipeline.h"

#include <Tempest/Device>

namespace Tempest {

RenderPipeline::RenderPipeline(Detail::DSharedPtr<AbstractGraphicsApi::Pipeline *> &&p,
                               Detail::DSharedPtr<AbstractGraphicsApi::PipelineLay*>&& ulay)
  : ulay(std::move(ulay)),impl(std::move(p)) {
  }

RenderPipeline &RenderPipeline::operator =(RenderPipeline&& other) noexcept {
  ulay = std::move(other.ulay);
  impl = std::move(other.impl);
  return *this;
  }
}
