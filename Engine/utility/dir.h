#pragma once

#include <functional>
#include <string>

namespace Tempest {

class Dir {
  public:
    Dir();

    enum FileType : uint8_t {
      FT_Dir  = 1,
      FT_File = 3
      };
    static bool scan(const char*        path,const std::function<void(const std::string&,FileType)>& cb);
    static bool scan(const std::string& path,const std::function<void(const std::string&,FileType)>& cb);

    static bool scan(const char16_t*       path,const std::function<void(const std::u16string&,FileType)>& cb);
    static bool scan(const std::u16string& path,const std::function<void(const std::u16string&,FileType)>& cb);
  };

}
