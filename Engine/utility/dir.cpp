#include "dir.h"

#include <Tempest/Platform>
#include <Tempest/TextCodec>

#ifdef __WINDOWS__
#include <windows.h>
#else
#include <cerrno>
#include <utility>
#include <dirent.h>
#if __ANDROID__
#include <errno.h>
#endif
#endif

using namespace Tempest;

Dir::Dir() = default;

bool Dir::scan(const std::string &path, const std::function<void (const std::string &, Dir::FileType)>& cb) {
  return scan(path.c_str(),cb);
  }

bool Dir::scan(const char *name, const std::function<void(const std::string&,FileType)>& cb) {
#if defined(__WINDOWS__) || defined(__WINDOWS_PHONE__)
  WIN32_FIND_DATAW ffd;
  HANDLE hFind = INVALID_HANDLE_VALUE;

  std::wstring path;
  const int len=MultiByteToWideChar(CP_UTF8,0,name,-1,nullptr,0);
  if(len>1){
    path.resize(size_t(len+1));
    MultiByteToWideChar(CP_UTF8,0,name,-1,&path[0],int(path.size()));
    path[path.size()-2]='/';
    path[path.size()-1]='*';
    }

  hFind = FindFirstFileExW( path.c_str(),
                            FindExInfoStandard, &ffd,
                            FindExSearchNameMatch, nullptr, 0);
  if(INVALID_HANDLE_VALUE==hFind)
    return false;

  std::string tmp;
  while( FindNextFileW(hFind, &ffd)!=0 ){
    const int l = WideCharToMultiByte(CP_UTF8,0,ffd.cFileName,-1,nullptr,0,nullptr,nullptr);
    if(len<=0)
      continue;

    tmp.resize(size_t(l));
    WideCharToMultiByte(CP_UTF8,0,ffd.cFileName,-1,&tmp[0],int(tmp.size()),nullptr,nullptr);

    if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      cb(tmp,FT_Dir); else
      cb(tmp,FT_File);
    }
  if( GetLastError() != ERROR_NO_MORE_FILES )
    return false;
#else
  DIR *dirp = opendir(name);
  if(dirp==nullptr)
    return false;

  try {
    std::string tmp;
    while (dirp) {
      errno = 0;
      if(dirent* dp = readdir(dirp)) {
        tmp = (char*)dp->d_name;
        if(dp->d_type==DT_DIR)
          cb(tmp,FT_Dir); else
          cb(tmp,FT_File);
        } else {
        if( errno == 0 ) {
          closedir(dirp);
          return true;
          }
        closedir(dirp);
        return false;
        }
      }
    }
  catch(...) {
    closedir(dirp);
    throw;
    }
#endif
  return true;
  }

bool Dir::scan(const std::u16string &path, const std::function<void (const std::u16string &, Dir::FileType)>& cb) {
  return scan(path.c_str(),cb);
  }

bool Dir::scan(const char16_t *path, const std::function<void (const std::u16string &, Dir::FileType)>& cb) {
#if defined(__WINDOWS__) || defined(__WINDOWS_PHONE__)
  WIN32_FIND_DATAW ffd;
  HANDLE hFind = INVALID_HANDLE_VALUE;

  std::u16string rg = path;
  rg += u"/*";

  hFind = FindFirstFileExW( reinterpret_cast<const WCHAR*>(rg.c_str()),
                            FindExInfoStandard, &ffd,
                            FindExSearchNameMatch, nullptr, 0);
  if(INVALID_HANDLE_VALUE==hFind)
    return false;

  while( FindNextFileW(hFind, &ffd)!=0 ){
    std::u16string filepath=reinterpret_cast<const char16_t*>(ffd.cFileName);
    if(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      cb(filepath,FT_Dir); else
      cb(filepath,FT_File);
    }
  if( GetLastError() != ERROR_NO_MORE_FILES )
    return false;
#else
  std::string name = TextCodec::toUtf8(path);
  DIR *dirp = opendir(name.c_str());
  if(dirp==nullptr)
    return false;

  try {
    std::u16string tmp;
    while (dirp) {
      errno = 0;
      if(dirent* dp = readdir(dirp)) {
        tmp = TextCodec::toUtf16((char*)dp->d_name);
        if(dp->d_type==DT_DIR)
          cb(tmp,FT_Dir); else
          cb(tmp,FT_File);
        } else {
        if( errno == 0 ) {
          closedir(dirp);
          return true;
          }
        closedir(dirp);
        return false;
        }
      }
    }
  catch(...) {
    closedir(dirp);
    throw;
    }
#endif
  return true;
  }
