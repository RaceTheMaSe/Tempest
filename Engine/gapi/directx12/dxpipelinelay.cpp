#if defined(TEMPEST_BUILD_DIRECTX12)

#include "dxpipelinelay.h"

#include <Tempest/PipelineLayout>

#include "dxdevice.h"
#include "guid.h"

#include <cassert>

using namespace Tempest;
using namespace Tempest::Detail;

static D3D12_SHADER_VISIBILITY nativeFormat(ShaderReflection::Stage stage){
  switch(stage) {
    case ShaderReflection::Stage::Vertex:
      return D3D12_SHADER_VISIBILITY_VERTEX;
    case ShaderReflection::Stage::Fragment:
      return D3D12_SHADER_VISIBILITY_PIXEL;
    case ShaderReflection::Stage::Geometry:
      return D3D12_SHADER_VISIBILITY_GEOMETRY;
    case ShaderReflection::Stage::Compute:
      return D3D12_SHADER_VISIBILITY_ALL;
    default: break;
    }
  return D3D12_SHADER_VISIBILITY_ALL;
  }

DxPipelineLay::DescriptorPool::DescriptorPool(DxPipelineLay& vlay) {
  auto& device = *vlay.dev.device;

  try {
    for(size_t i=0;i<vlay.heaps.size();++i) {
      D3D12_DESCRIPTOR_HEAP_DESC d = {};
      d.Type           = vlay.heaps[i].type;
      d.NumDescriptors = vlay.heaps[i].numDesc * POOL_SIZE;
      d.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
      dxAssert(device.CreateDescriptorHeap(&d, uuid<ID3D12DescriptorHeap>(), reinterpret_cast<void**>(&heap[i])));

      // auto  gpu = heap[i]->GetCPUDescriptorHandleForHeapStart();
      // Log::d("src_gpu.ptr = ",gpu.ptr);
      // if(gpu.ptr==0)
      //   Log::d("src_gpu.ptr = ",gpu.ptr);
      }
    }
  catch(...){
    for(auto i:heap)
      if(i!=nullptr)
        i->Release();
    throw;
    }
  }

DxPipelineLay::DescriptorPool::DescriptorPool(DxPipelineLay::DescriptorPool&& oth) noexcept
  :allocated(oth.allocated) {
  for(size_t i=0;i<MAX_BINDS;++i) {
    heap[i] = oth.heap[i];
    oth.heap[i] = nullptr;
    }
  }

DxPipelineLay::DescriptorPool::~DescriptorPool() {
  for(auto i:heap)
    if(i!=nullptr)
      i->Release();
  }


DxPipelineLay::DxPipelineLay(DxDevice& dev, const std::vector<ShaderReflection::Binding>& comp)
  :dev(dev) {
  ShaderReflection::PushBlock pb;
  ShaderReflection::merge(lay,pb, comp);
  init(lay,pb);
  }

DxPipelineLay::DxPipelineLay(DxDevice& dev, const std::vector<ShaderReflection::Binding>* sh[], size_t cnt)
  : dev(dev) {
  ShaderReflection::PushBlock pb;
  ShaderReflection::merge(lay, pb, sh, cnt);
  init(lay,pb);
  }

size_t DxPipelineLay::descriptorsCount() {
  return lay.size();
  }

void DxPipelineLay::init(const std::vector<Binding>& layer, const ShaderReflection::PushBlock& pb) {
  auto& device = *dev.device;
  descSize = device.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  smpSize  = device.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

  uint32_t lastBind=0;
  for(auto& i:layer)
    lastBind = std::max(lastBind,i.layout);
  if(layer.size()>0)
    prm.resize(lastBind+1);

  std::vector<Parameter> desc;
  for(size_t i=0;i<layer.size();++i) {
    auto& l = layer[i];
    if(l.stage==ShaderReflection::Stage(0))
      continue;
    switch(l.cls) {
      case ShaderReflection::Ubo: {
        add(l,D3D12_DESCRIPTOR_RANGE_TYPE_CBV,desc);
        break;
        }
      case ShaderReflection::SsboR: {
        add(l,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,desc);
        break;
        }
      case ShaderReflection::SsboRW: {
        add(l,D3D12_DESCRIPTOR_RANGE_TYPE_UAV,desc);
        break;
        }
      case ShaderReflection::ImgR: {
        add(l,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,desc);
        break;
        }
      case ShaderReflection::ImgRW: {
        add(l,D3D12_DESCRIPTOR_RANGE_TYPE_UAV,desc);
        break;
        }
      case ShaderReflection::Texture: {
        add(l,D3D12_DESCRIPTOR_RANGE_TYPE_SRV,    desc);
        add(l,D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,desc);
        break;
        }
      case ShaderReflection::Push:
        break;
      }
    }
  std::sort(desc.begin(),desc.end(),[](const Parameter& a, const Parameter& b){
    return std::tie(a.visibility,a.rgn.RangeType)<std::tie(b.visibility,b.rgn.RangeType);
    });

  std::vector<D3D12_DESCRIPTOR_RANGE> rgn(desc.size());
  std::vector<D3D12_ROOT_PARAMETER>   rootPrm;

  uint8_t curVisibility = 255;
  uint8_t curRgnType    = 255;
  for(size_t i=0; i<rgn.size(); ++i) {
    rgn[i] = desc[i].rgn;

    D3D12_DESCRIPTOR_HEAP_TYPE heapType
        = (rgn[i].RangeType==D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) ?
          D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER :
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    Heap* heap = nullptr;
    for(size_t r=0;r<heaps.size();++r) {
      if(heaps[r].type==heapType) {
        heap = &heaps[r];
        break;
        }
      }

    if(heap==nullptr) {
      heaps.emplace_back();
      heap = &heaps.back();
      heap->type    = heapType;
      heap->numDesc = 0;
      }

    if(uint8_t(desc[i].visibility)   !=curVisibility ||
       uint8_t(desc[i].rgn.RangeType)!=curRgnType) {
      D3D12_ROOT_PARAMETER p = {};
      p.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      p.ShaderVisibility                    = desc[i].visibility;
      p.DescriptorTable.pDescriptorRanges   = &rgn[i];
      p.DescriptorTable.NumDescriptorRanges = 0;
      rootPrm.push_back(p);

      RootPrm r = {};
      r.heap       = uint8_t(std::distance(&heaps[0],heap));
      r.heapOffset = heap->numDesc;
      roots.push_back(r);

      curVisibility = uint8_t(desc[i].visibility);
      curRgnType    = uint8_t(desc[i].rgn.RangeType);
      }

    auto& px = rootPrm.back();
    px.DescriptorTable.NumDescriptorRanges++;
    heap->numDesc += rgn[i].NumDescriptors;

    auto& p = prm[desc[i].id];
    p.rgnType = D3D12_DESCRIPTOR_RANGE_TYPE(curRgnType);
    if(curRgnType==D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
      p.heapOffsetSmp = heap->numDesc-1;
      p.heapIdSmp     = uint8_t(std::distance(&heaps[0],heap));
      } else {
      p.heapOffset    = heap->numDesc-1;
      p.heapId        = uint8_t(std::distance(&heaps[0],heap));
      }
    }

  for(auto& i:prm) {
    i.heapOffset    *= descSize;
    i.heapOffsetSmp *= smpSize;
    }

  for(auto& i:roots) {
    if(heaps[i.heap].type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
      i.heapOffset *= descSize; else
      i.heapOffset *= smpSize;
    }

  if(pb.size>0) {
    D3D12_ROOT_PARAMETER prmPush = {};
    prmPush.ParameterType            = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    prmPush.ShaderVisibility         = ::nativeFormat(pb.stage);
    prmPush.Constants.ShaderRegister = 0;
    prmPush.Constants.RegisterSpace  = 0;
    prmPush.Constants.Num32BitValues = UINT((pb.size+3)/4);
    // remap register to match spiv-cross codegen
    uint32_t layout = 0;
    for(layout=0; ; ++layout) {
      bool done = true;
      for(auto& i:lay)
        if(i.stage!=ShaderReflection::Stage(0) && i.cls==ShaderReflection::Ubo && i.layout==layout) {
          done = false;
          break;
          }
      if(done)
        break;
      }
    prmPush.Constants.ShaderRegister = layout;
    pushConstantId = rootPrm.size();
    rootPrm.push_back(prmPush);
    }

  D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
  featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

  D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc={};
  rootSignatureDesc.NumParameters     = UINT(rootPrm.size());
  rootSignatureDesc.pParameters       = rootPrm.data();
  rootSignatureDesc.NumStaticSamplers = 0;
  rootSignatureDesc.pStaticSamplers   = nullptr;
  rootSignatureDesc.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

  ComPtr<ID3DBlob> signature;
  ComPtr<ID3DBlob> error;

  auto hr = dev.dllApi.D3D12SerializeRootSignature(&rootSignatureDesc, featureData.HighestVersion,
                                                   &signature.get(), &error.get());
  if(FAILED(hr)) {
#if !defined(NDEBUG)
    const char* msg = reinterpret_cast<const char*>(error->GetBufferPointer());
    Log::e(msg);
#endif
    dxAssert(hr);
    }
  dxAssert(device.CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                      uuid<ID3D12RootSignature>(), reinterpret_cast<void**>(&impl)));
  }

void DxPipelineLay::add(const ShaderReflection::Binding& b,
                        D3D12_DESCRIPTOR_RANGE_TYPE type,
                        std::vector<Parameter>& root) {
  Parameter rp;

  rp.rgn.RangeType          = type;
  rp.rgn.NumDescriptors     = 1;
  rp.rgn.BaseShaderRegister = b.layout;
  rp.rgn.RegisterSpace      = 0;
  rp.rgn.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

  rp.id         = b.layout;
  rp.visibility = ::nativeFormat(b.stage);

  root.push_back(rp);
  }

DxPipelineLay::PoolAllocation DxPipelineLay::allocDescriptors() {
  std::lock_guard<SpinLock> guard(poolsSync);

  DescriptorPool* pool = nullptr;
  for(size_t i=0; i<pools.size(); ++i) {
    auto& p = pools[i];
    if(p.allocated.all())
      continue;
    pool = &p;
    break;
    }

  if(pool==nullptr) {
    pools.emplace_back(*this);
    pool = &pools.back();
    }

  for(size_t i=0; ; ++i)
    if(!pool->allocated.test(i)) {
      pool->allocated.set(i);

      PoolAllocation a;
      a.pool   = std::distance(pools.data(),pool);
      a.offset = i;
      for(size_t r=0; r<MAX_BINDS; ++r) {
        if(pool->heap[r]==nullptr)
          continue;
        a.heap[r] = pool->heap[r];
        a.cpu [r] = pool->heap[r]->GetCPUDescriptorHandleForHeapStart();
        a.gpu [r] = pool->heap[r]->GetGPUDescriptorHandleForHeapStart();

        UINT64 offset = heaps[r].numDesc*i;
        if(heaps[r].type==D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
          a.cpu[r].ptr += descSize*offset;
          a.gpu[r].ptr += descSize*offset;
          } else {
          a.cpu[r].ptr += smpSize *offset;
          a.gpu[r].ptr += smpSize *offset;
          }
        }
      return a;
      }
  }

void DxPipelineLay::freeDescriptors(DxPipelineLay::PoolAllocation& d) {
  std::lock_guard<SpinLock> guard(poolsSync);
  auto& p = pools[d.pool];
  p.allocated.set(d.offset,false);
  }

#endif

