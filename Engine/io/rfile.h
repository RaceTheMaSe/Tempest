#pragma once

#include <Tempest/IDevice>
#include <Tempest/Platform>
#include <string>

namespace Tempest {

class RFile : public Tempest::IDevice {
  public:
    explicit RFile(const char*     path);
    explicit RFile(const std::string& path);
    explicit RFile(const char16_t* path);
    explicit RFile(const std::u16string& path);
    RFile(RFile&& other) noexcept;
    ~RFile() override;

    RFile& operator = (RFile&& other) noexcept;

    size_t  read(void* to,size_t size) override;
    size_t  size() const override;

    uint8_t peek() override;
    size_t  seek(size_t advance) override;
    size_t  unget(size_t advance) override;

#ifdef __WINDOWS__
    __time64_t lastModificationTime() const;
#else
    __time_t   lastModificationTime() const;
#endif

  private:
    void* handle=nullptr;
#ifdef __WINDOWS__
    static void* implOpen(const wchar_t* wstr);
    std::wstring fn;
    __time64_t   lastModification;
#else
    static void* implOpen(const char* cstr);
    std::string  fn;
    __time_t     lastModification;
#endif
  };

}
