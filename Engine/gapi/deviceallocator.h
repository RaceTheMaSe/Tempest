#pragma once

#include <cstdint>
#include <forward_list>
#include <mutex>
#include <algorithm>

namespace Tempest {
namespace Detail {

template<class MemoryProvider>
class DeviceAllocator {
  struct Page;
  struct Block;
  public:
    enum {
      DEFAULT_PAGE_SIZE=128*1024*1024
      };
    using Memory=typename MemoryProvider::DeviceMemory;
    static const constexpr Memory null=Memory{};

    explicit DeviceAllocator(MemoryProvider& device):device(device){}

    DeviceAllocator(const DeviceAllocator&)=delete;

    ~DeviceAllocator(){
      for(auto& i:pages)
        device.free(i.memory,i.size,i.type);
      }

    struct Allocation {
      Page*  page  =nullptr;
      size_t offset=0,size=0;
      };

    Allocation alloc(size_t size, size_t align, uint32_t heapId, uint32_t typeId, bool hostVisible) {
      std::lock_guard<std::mutex> guard(sync);
      for(auto& i:pages){
        if(i.type==heapId && i.allocated+size<=i.allSize){
          auto ret=i.alloc(size,align,device);
          if(ret.page!=nullptr)
            return ret;
          }
        }
      return rawAlloc(size,align,heapId,typeId,hostVisible,false);
      }

    void free(const Allocation& a){
      std::lock_guard<std::mutex> guard(sync);
      a.page->free(a);
      if(a.page->allocated==0){
        device.free(a.page->memory,a.page->size,a.page->type);
        pages.remove(*a.page);
        }
      }

    Allocation dedicatedAlloc(size_t size, size_t align, uint32_t heapId, uint32_t typeId, bool hostVisible) {
      std::lock_guard<std::mutex> guard(sync);
      return rawAlloc(size,align,heapId,typeId,hostVisible,true);
      }

  private:
    Allocation rawAlloc(size_t size, size_t align, uint32_t heapId, uint32_t typeId, bool hostVisible, bool dedicated){
      const uint32_t pgSize = (dedicated ? uint32_t(size) : std::max<uint32_t>(DEFAULT_PAGE_SIZE,uint32_t(size)));
      Page pg(pgSize);
      pg.memory      = device.alloc(pg.allSize,typeId);
      pg.type        = heapId;
      pg.hostVisible = hostVisible;
      if(pg.memory==null)
        return Allocation();
      try {
        pages.push_front(Page(0));
        }
      catch(...){
        device.free(pg.memory,pg.size,pg.type);
        throw;
        }
      pages.front() = std::move(pg);
      return pages.front().alloc(size,align,device);
      }

    void       freeDevMemory(Memory mem){
      return device.free(mem);
      }

    MemoryProvider&         device;
    std::mutex              sync;
    std::forward_list<Page> pages;
  };

template<class MemoryProvider>
struct DeviceAllocator<MemoryProvider>::Block {
  Block*   next  =nullptr;
  uint32_t size  =0;
  uint32_t offset=0;
  };

template<class MemoryProvider>
struct DeviceAllocator<MemoryProvider>::Page : Block {
  using    Block::next;
  using    Block::size;
  using    Block::offset;

  Memory     memory = null;
  std::mutex mmapSync;
  uint32_t   type        = 0;
  uint32_t   allSize     = 0;
  uint32_t   allocated   = 0;
  bool       hostVisible = false;

  explicit Page(uint32_t sz) noexcept {
    size   =sz;
    allSize=sz;
    }

  Page(Page&& p) noexcept : mmapSync() {
    std::swap(next,p.next);
    size  =p.size;
    offset=p.offset;
    std::swap(memory,p.memory);
    type        = p.type;
    allSize     = p.allSize;
    hostVisible = p.hostVisible;
    }

  ~Page(){
    Block* b=next;
    while(b!=nullptr){
      Block* p=b->next;
      delete b;
      b=p;
      }
    }

  Page& operator=(Page&& p) noexcept {
    std::swap(next,p.next);
    size  =p.size;
    offset=p.offset;
    std::swap(memory,p.memory);
    type        = p.type;
    allSize     = p.allSize;
    hostVisible = p.hostVisible;
    return *this;
    }

  bool operator==(const Page& other) const noexcept {
    return memory==other.memory;
    }

  Allocation alloc(size_t size,size_t align,MemoryProvider& /*prov*/) noexcept {
    Block*   b=this;
    while(b!=nullptr) {
      if(size<=b->size) {
        size_t padding=b->offset%align;
        if(padding==0)
          return alloc(*b,size);

        padding=align-padding;
        if(size+padding==b->size) {
          return alloc(*b,size,padding);
          } else
        if(size+padding<b->size){
          auto* bp=new(std::nothrow) Block();
          if(bp!=nullptr){
            bp->next=b->next;
            b->next =bp;

            bp->size  =uint32_t(b->size-padding);
            bp->offset=uint32_t(b->offset+padding);
            b->size   =uint32_t(padding);

            return alloc(*bp,size);
            }
          }
        }
      b=b->next;
      }
    return Allocation{};
    }

  Allocation alloc(Block& b,size_t size) noexcept {
    Allocation a;
    a.offset=b.offset;
    a.page  =this;
    a.size  =size;

    b.offset +=uint32_t(size);
    b.size   -=uint32_t(size);
    allocated+=uint32_t(size);
    return a;
    }

  Allocation alloc(Block& b,size_t size,size_t padding) noexcept {
    Allocation a;
    a.offset=b.offset+padding;
    a.page  =this;
    a.size  =size;

    auto sz=uint32_t(size+padding);
    b.offset +=sz;
    b.size   -=sz;
    allocated+=uint32_t(size);
    return a;
    }

  void free(const Allocation& a) noexcept {
    allocated -= uint32_t(a.size);

    Block* b=this;
    while(b!=nullptr && (b->offset+b->size)<a.offset)
      b=b->next;

    if(b==nullptr)
      return;
    
    if(b->offset+b->size==a.offset){
      b->size+=uint32_t(a.size);
      mergeWithNext(b);
      return;
      }
    if(b->offset==a.offset+a.size){
      b->offset=uint32_t(a.offset);
      b->size +=uint32_t(a.size);
      return;
      }

    auto* r=new(std::nothrow) Block();
    if(r==nullptr)
      return; // no error, but sort of leak in Page
    *r=*b;
    b->next  =r;
    b->size  =uint32_t(a.size);
    b->offset=uint32_t(a.offset);
    }

  void mergeWithNext(Block* b) noexcept {
    while(b->next!=nullptr){
      if(b->offset+b->size==b->next->offset){
        auto rm=b->next;
        b->size+=b->next->size;
        b->next=b->next->next;
        delete rm;
        } else {
        return;
        }
      }
    }
  };
}}

