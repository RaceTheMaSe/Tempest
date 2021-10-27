#include "device.h"
#include "graphics/framebuffer.h"

#include <Tempest/Fence>
#include <Tempest/PipelineLayout>
#include <Tempest/UniformBuffer>
#include <Tempest/File>
#include <Tempest/Pixmap>
#include <Tempest/Except>

#include <mutex>

using namespace Tempest;

static uint32_t mipCount(uint32_t w, uint32_t h) {
  uint32_t s = std::max(w,h);
  uint32_t n = 1;
  while(s>1) {
    ++n;
    s = s/2;
    }
  return n;
  }

Device::Impl::Impl(AbstractGraphicsApi &api, const char* name)
  :api(api) {
  dev=api.createDevice(name);
  }

Device::Impl::~Impl() {
  api.destroy(dev);
  }

Device::Device(AbstractGraphicsApi& api)
  :Device(api,nullptr){
  }

Device::Device(AbstractGraphicsApi &api, const char* name)
  :api(api), impl(api,name), dev(impl.dev), builtins(*this) {
  api.getCaps(dev,devProps);
  }

Device::~Device() = default;

void Device::waitIdle() {
  impl.dev->waitIdle();
  }

void Device::submit(const CommandBuffer &cmd) {
  api.submit(dev,cmd.impl.handler,nullptr);
  }

void Device::submit(const CommandBuffer &cmd, Fence &fdone) {
  api.submit(dev,cmd.impl.handler,fdone.impl.handler);
  }

void Device::submit(const Tempest::CommandBuffer *cmd[], size_t count, Fence *fdone) {
  if(count<=64) {
    void* ptr[64];
    auto cx = reinterpret_cast<AbstractGraphicsApi::CommandBuffer**>(ptr);

    implSubmit(cmd,  cx, count, fdone->impl.handler);
    } else {
    std::unique_ptr<void*[]> ptr(new void*[count]);
    auto cx = reinterpret_cast<AbstractGraphicsApi::CommandBuffer**>(ptr.get());

    implSubmit(cmd, cx, count, fdone->impl.handler);
    }
  }

void Device::present(Swapchain& sw) {
  api.present(dev,sw.impl.handler);
  }

void Device::implSubmit(const CommandBuffer* cmd[], AbstractGraphicsApi::CommandBuffer*  hcmd[], size_t count, AbstractGraphicsApi::Fence* fdone) {
  for(size_t i=0;i<count;++i)
    hcmd[i] = cmd[i]->impl.handler;

  api.submit(dev, hcmd, count, fdone);
  }

Shader Device::loadShader(RFile &file) {
  const size_t fileSize=file.size();

  std::unique_ptr<uint8_t[]> buffer(new uint8_t[fileSize]);
  file.read(reinterpret_cast<char*>(buffer.get()),fileSize);

  size_t size=uint32_t(fileSize);

  Shader f(*this,api.createShader(dev,reinterpret_cast<const char*>(buffer.get()),size));
  return f;
  }

Shader Device::loadShader(const char *filename) {
  Tempest::RFile file(filename);
  return loadShader(file);
  }

Shader Device::loadShader(const char16_t *filename) {
  Tempest::RFile file(filename);
  return loadShader(file);
  }

Shader Device::shader(const void *source, const size_t length) {
  Shader f(*this,api.createShader(dev,source,length));
  return f;
  }

Swapchain Device::swapchain(SystemApi::Window* w) const {
  return Swapchain(api.createSwapchain(w,impl.dev));
  }

const Device::Props& Device::properties() const {
  return devProps;
  }

Attachment Device::attachment(TextureFormat frm, const uint32_t w, const uint32_t h, const bool mips) {
  if(!devProps.hasSamplerFormat(frm) && !devProps.hasAttachFormat(frm))
    throw std::system_error(Tempest::GraphicsErrc::UnsupportedTextureFormat);
  if(w>devProps.tex2d.maxSize || h>devProps.tex2d.maxSize)
    throw std::system_error(Tempest::GraphicsErrc::UnsupportedTextureFormat);
  uint32_t mipCnt = mips ? mipCount(w,h) : 1;
  Texture2d t(*this,api.createTexture(dev,w,h,mipCnt,frm),w,h,frm);
  return Attachment(std::move(t));
  }

ZBuffer Device::zbuffer(TextureFormat frm, const uint32_t w, const uint32_t h) {
  if(!devProps.hasDepthFormat(frm))
    throw std::system_error(Tempest::GraphicsErrc::UnsupportedTextureFormat);
  if(w>devProps.tex2d.maxSize || h>devProps.tex2d.maxSize)
    throw std::system_error(Tempest::GraphicsErrc::UnsupportedTextureFormat);
  Texture2d t(*this,api.createTexture(dev,w,h,1,frm),w,h,frm);
  return ZBuffer(std::move(t),devProps.hasSamplerFormat(frm));
  }

Texture2d Device::loadTexture(const Pixmap &pm, bool mips) {
  TextureFormat format = Pixmap::toTextureFormat(pm.format());
  uint32_t      mipCnt = mips ? mipCount(pm.w(),pm.h()) : 1;
  const Pixmap* p=&pm;
  Pixmap        alt;

  if(pm.w()>devProps.tex2d.maxSize || pm.h()>devProps.tex2d.maxSize)
    throw std::system_error(Tempest::GraphicsErrc::UnsupportedTextureFormat);

  if(isCompressedFormat(format)){
    if(devProps.hasSamplerFormat(format) && (!mips || pm.mipCount()>1)){
      mipCnt = pm.mipCount();
      } else {
      alt    = Pixmap(pm,Pixmap::Format::RGBA);
      p      = &alt;
      format = TextureFormat::RGBA8;
      }
    }

  if(!devProps.hasSamplerFormat(format)) {
    if(format==TextureFormat::RGB8){
      alt    = Pixmap(pm,Pixmap::Format::RGBA);
      p      = &alt;
      format = TextureFormat::RGBA8;
      }
    else if(format==TextureFormat::RGB16){
      alt    = Pixmap(pm,Pixmap::Format::RGBA16);
      p      = &alt;
      format = TextureFormat::RGBA16;
      }
    else if(format==TextureFormat::RGB32F){
      alt    = Pixmap(pm,Pixmap::Format::RGBA32F);
      p      = &alt;
      format = TextureFormat::RGBA32F;
      }
    else {
      throw std::system_error(Tempest::GraphicsErrc::UnsupportedTextureFormat);
      }
    }

  Texture2d t(*this,api.createTexture(dev,*p,format,mipCnt),p->w(),p->h(),format);
  return t;
  }

StorageImage Device::image2d(TextureFormat frm, const uint32_t w, const uint32_t h, const bool mips) {
  if(!devProps.hasStorageFormat(frm))
    throw std::system_error(Tempest::GraphicsErrc::UnsupportedTextureFormat);
  if(w>devProps.tex2d.maxSize || h>devProps.tex2d.maxSize)
    throw std::system_error(Tempest::GraphicsErrc::UnsupportedTextureFormat);
  uint32_t mipCnt = mips ? mipCount(w,h) : 1;
  StorageImage t(*this,api.createStorage(dev,w,h,mipCnt,frm),w,h,frm);
  return t;
  }

Pixmap Device::readPixels(const Texture2d &t, uint32_t mip) {
  Pixmap pm;
  api.readPixels(dev,pm,t.impl,TextureLayout::Sampler,t.format(),uint32_t(t.w()),uint32_t(t.h()),mip);
  return pm;
  }

Pixmap Device::readPixels(const Attachment& t, uint32_t mip) {
  Pixmap pm;
  auto& tx = textureCast(t);
  uint32_t w = t.w();
  uint32_t h = t.h();
  for(uint32_t i=0; i<mip; ++i) {
    w = (w==1 ? 1 : w/2);
    h = (h==1 ? 1 : h/2);
    }
  api.readPixels(dev,pm,tx.impl,TextureLayout::Sampler,tx.format(),w,h,mip);
  return pm;
  }

Pixmap Device::readPixels(const StorageImage& t, uint32_t mip) {
  Pixmap pm;
  uint32_t w = t.w();
  uint32_t h = t.h();
  for(uint32_t i=0; i<mip; ++i) {
    w = (w==1 ? 1 : w/2);
    h = (h==1 ? 1 : h/2);
    }
  api.readPixels(dev,pm,t.impl,TextureLayout::Unordered,t.format(),w,h,mip);
  return pm;
  }

void Device::readBytes(const StorageBuffer& ssbo, void* out, size_t size) {
  api.readBytes(dev,ssbo.impl.impl.handler,out,size);
  }

TextureFormat Device::formatOf(const Attachment& a) {
  if(a.sImpl.swapchain!=nullptr)
    return TextureFormat::Undefined;
  return a.tImpl.frm;
  }

FrameBuffer Device::frameBuffer(Attachment &out) {
  TextureFormat att[1] = {formatOf(out)};
  auto          w      = uint32_t(out.w());
  auto          h      = uint32_t(out.h());

  AbstractGraphicsApi::Texture*   cl[1]    = { out.tImpl.impl.handler };
  AbstractGraphicsApi::Swapchain* sw[1]    = { out.sImpl.swapchain    };
  uint32_t                        imgId[1] = { out.sImpl.id           };
  auto                            lay      = FrameBufferLayout(api.createFboLayout(dev,(AbstractGraphicsApi::Swapchain**)sw,(Tempest::TextureFormat*)att,1));

  auto fbo = api.createFbo(dev,lay.impl.handler,w,h,1, (AbstractGraphicsApi::Swapchain**)sw,(AbstractGraphicsApi::Texture**)cl,(const uint32_t*)imgId,nullptr);
  return FrameBuffer(std::move(fbo),std::move(lay),w,h);
  }

FrameBuffer Device::frameBuffer(Attachment& out, ZBuffer& zbuf) {
  TextureFormat att[2] = {formatOf(out),zbuf.tImpl.frm};
  auto          w      = uint32_t(out.w());
  auto          h      = uint32_t(out.h());
  auto          zImpl  = zbuf.tImpl.impl.handler;

  if(out.w()!=zbuf.w() || out.h()!=zbuf.h())
    throw IncompleteFboException();

  AbstractGraphicsApi::Texture*   cl[1]    = { out.tImpl.impl.handler };
  AbstractGraphicsApi::Swapchain* sw[1]    = { out.sImpl.swapchain    };
  uint32_t                        imgId[1] = { out.sImpl.id           };
  auto                            lay      = FrameBufferLayout(api.createFboLayout(dev,(AbstractGraphicsApi::Swapchain**)sw,(Tempest::TextureFormat*)att,2));

  auto fbo = api.createFbo(dev,lay.impl.handler,w,h,1, (AbstractGraphicsApi::Swapchain**)sw,(AbstractGraphicsApi::Texture**)cl,(const uint32_t*)imgId,zImpl);
  return FrameBuffer(std::move(fbo),std::move(lay),w,h);
  }

FrameBuffer Device::frameBuffer(Attachment& out0, Attachment& out1, ZBuffer& zbuf) {
  Attachment* out[2] = {&out0, &out1};
  return frameBuffer((Tempest::Attachment**)out,2,&zbuf);
  }

FrameBuffer Device::frameBuffer(Attachment& out0, Attachment& out1, Attachment& out2, ZBuffer& zbuf) {
  Attachment* out[3] = {&out0, &out1, &out2};
  return frameBuffer((Tempest::Attachment**)out,3,&zbuf);
  }

FrameBuffer Device::frameBuffer(Attachment& out0, Attachment& out1, Attachment& out2, Attachment& out3, ZBuffer& zbuf) {
  Attachment* out[4] = {&out0, &out1, &out2, &out3};
  return frameBuffer((Tempest::Attachment**)out,4,&zbuf);
  }

FrameBuffer Device::frameBuffer(Attachment** out, uint8_t count, ZBuffer* zbuf) {
  TextureFormat att[257] = {}; // 256+zbuf
  auto          w        = uint32_t(out[0]->w());
  auto          h        = uint32_t(out[0]->h());

  AbstractGraphicsApi::Texture*   zImpl      = nullptr;
  AbstractGraphicsApi::Swapchain* sw[256]    = {};
  AbstractGraphicsApi::Texture*   cl[256]    = {};
  uint32_t                        imgId[256] = {};

  for(size_t i=0; i<count; ++i) {
    att[i] = formatOf(*out[i]);
    if(out[i]->w()!=int(w) || out[i]->h()!=int(h))
      throw IncompleteFboException();
    sw[i]    = out[i]->sImpl.swapchain;
    cl[i]    = out[i]->tImpl.impl.handler;
    imgId[i] = out[i]->sImpl.id;
    }
  if(zbuf!=nullptr) {
    zImpl = zbuf->tImpl.impl.handler;
    if(zbuf->w()!=int(w) || zbuf->h()!=int(h))
      throw IncompleteFboException();
    att[count] = zbuf->tImpl.frm;
    }

  auto lay = FrameBufferLayout(api.createFboLayout(dev,(AbstractGraphicsApi::Swapchain**)sw,(Tempest::TextureFormat*)att,count+(zbuf!=nullptr ? 1 : 0)));
  auto fbo = api.createFbo(dev,lay.impl.handler,w,h,count,(AbstractGraphicsApi::Swapchain**)sw,(AbstractGraphicsApi::Texture**)cl,(const uint32_t*)imgId,zImpl);
  return FrameBuffer(std::move(fbo),std::move(lay),w,h);
  }

RenderPass Device::pass(const FboMode &color) {
  const FboMode* att[1]={&color};
  RenderPass f(api.createPass(dev,(const Tempest::FboMode**)att,1));
  return f;
  }

RenderPass Device::pass(const FboMode& color, const FboMode& depth) {
  const FboMode* att[2]={&color,&depth};
  RenderPass f(api.createPass(dev,(const Tempest::FboMode**)att,2));
  return f;
  }

RenderPass Device::pass(const FboMode& c0, const FboMode& c1, const FboMode& depth) {
  const FboMode* att[3]={&c0,&c1,&depth};
  RenderPass f(api.createPass(dev,(const Tempest::FboMode**)att,3));
  return f;
  }

RenderPass Device::pass(const FboMode& c0, const FboMode& c1, const FboMode& c2, const FboMode& depth) {
  const FboMode* att[4]={&c0,&c1,&c2,&depth};
  RenderPass f(api.createPass(dev,(const Tempest::FboMode**)att,4));
  return f;
  }

RenderPass Device::pass(const FboMode& c0, const FboMode& c1,
                        const FboMode& c2, const FboMode& c3, const FboMode& depth) {
  const FboMode* att[5]={&c0,&c1,&c2,&c3,&depth};
  RenderPass f(api.createPass(dev,(const Tempest::FboMode**)att,5));
  return f;
  }

RenderPass Device::pass(const FboMode** color, uint8_t count) {
  RenderPass f(api.createPass(dev,color,count));
  return f;
  }

Fence Device::fence() {
  Fence f(*this,api.createFence(dev));
  return f;
  }

ComputePipeline Device::pipeline(const Shader& comp) {
  if(!comp.impl)
    return ComputePipeline();

  auto ulay = api.createPipelineLayout(dev,nullptr,nullptr,nullptr,nullptr,nullptr,comp.impl.handler);
  auto pipe = api.createComputePipeline(dev,*ulay.handler,comp.impl.handler);
  ComputePipeline f(std::move(pipe),std::move(ulay));
  return f;
  }

RenderPipeline Device::implPipeline(const RenderState &st,
                                    const Shader* sh[],
                                    size_t   stride,
                                    Topology tp) {
  if(sh[0]==nullptr || sh[4]==nullptr)
    return RenderPipeline();
  if(!sh[0]->impl || !sh[4]->impl)
    return RenderPipeline();

  AbstractGraphicsApi::Shader* shv[5] = {};
  for(size_t i=0; i<5; ++i)
    shv[i] = sh[i]!=nullptr ? sh[i]->impl.handler : nullptr;

  auto ulay = api.createPipelineLayout(dev,shv[0],shv[1],shv[2],shv[3],shv[4],nullptr);
  auto pipe = api.createPipeline(dev,st,stride,tp,*ulay.handler,shv[0],shv[1],shv[2],shv[3],shv[4]);
  RenderPipeline f(std::move(pipe),std::move(ulay));
  return f;
  }

CommandBuffer Device::commandBuffer() {
  CommandBuffer buf(*this,api.createCommandBuffer(dev));
  return buf;
  }

const Builtin& Device::builtin() const {
  return builtins;
  }

const std::vector<AbstractGraphicsApi::Props> Device::deviceList() const {
  return api.devices();
  }

VideoBuffer Device::createVideoBuffer(const void *data, size_t count, size_t size, size_t alignedSz, MemUsage usage, BufferHeap flg) {
  VideoBuffer buf(api.createBuffer(dev,data,count,size,alignedSz,usage,flg),count*alignedSz);
  return  buf;
  }

DescriptorSet Device::descriptors(const PipelineLayout &ulay) {
  if(ulay.impl.handler==nullptr || ulay.impl.handler->descriptorsCount()==0)
    return DescriptorSet(nullptr);
  DescriptorSet ubo(api.createDescriptors(dev,*ulay.impl.handler));
  return ubo;
  }

