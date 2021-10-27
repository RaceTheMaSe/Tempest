#include "framebuffer.h"
#include <Tempest/Device>

using namespace Tempest;

FrameBufferLayout::FrameBufferLayout(Detail::DSharedPtr<AbstractGraphicsApi::FboLayout *> &&f)
  :impl(std::move(f)) {
  }

FrameBuffer::FrameBuffer(Detail::DSharedPtr<AbstractGraphicsApi::Fbo*> &&impl,
                         FrameBufferLayout &&lay, uint32_t w, uint32_t h)
  :impl(std::move(impl)),lay(std::move(lay)),mw(w),mh(h) {
  }

FrameBuffer::FrameBuffer(FrameBuffer&& other) noexcept
  :impl(std::move(other.impl)),lay(std::move(other.lay)),mw(other.mw),mh(other.mh){
  other.mw = 0;
  other.mh = 0;
  }

FrameBuffer::~FrameBuffer() = default;

FrameBuffer& FrameBuffer::operator =(FrameBuffer&& other) noexcept {
  std::swap(impl,other.impl);
  std::swap(lay, other.lay);
  std::swap(mw,  other.mw);
  std::swap(mh,  other.mh);
  return *this;
  }

const FrameBufferLayout& FrameBuffer::layout() const {
  return lay;
  }

bool FrameBufferLayout::operator != (const FrameBufferLayout &fbo) const {
  return !(*this==fbo);
  }

bool FrameBufferLayout::operator == (const FrameBufferLayout &fbo) const {
  if(impl.handler==fbo.impl.handler)
    return true;
  if(impl.handler==nullptr || fbo.impl.handler==nullptr)
    return false;
  return impl.handler->equals(*fbo.impl.handler);
  }
