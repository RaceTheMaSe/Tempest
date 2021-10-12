#include "texture2d.h"

#include <Tempest/Device>
#include <algorithm>

using namespace Tempest;

Texture2d::Texture2d(Device &, AbstractGraphicsApi::PTexture&& impl, uint32_t w, uint32_t h, TextureFormat frm)
  :impl(std::move(impl)),texW(int(w)),texH(int(h)),frm(frm) {
  }

Texture2d::Texture2d(Texture2d&& other) noexcept
  :impl(std::move(other.impl)), texW(other.texW), texH(other.texH), frm(other.frm) {
  other.texW = 0;
  other.texH = 0;
  }

Texture2d::~Texture2d()= default;

Texture2d& Texture2d::operator=(Texture2d&& other) noexcept {
  std::swap(impl, other.impl);
  std::swap(texW, other.texW);
  std::swap(texH, other.texH);
  std::swap(frm,  other.frm);
  return *this;
  }

uint32_t Texture2d::mipCount() const {
  return impl.handler ? impl.handler->mipCount() : 0;
  }
