#include "log.h"

#include <Tempest/Platform>
#include "utility/utility.h"

#ifdef __ANDROID__
#include <android/log.h>
const char* logCategory("LogOpenGothic");
#endif

#if defined(__WINDOWS_PHONE__) || defined(_MSC_VER)
#include <windows.h>
#define snprintf sprintf_s
#endif

#include <iostream>
#include <cstdio>
#include <cstring>

using namespace Tempest;

void Log::setOutputCallback(std::function<void (Mode, const char*)> f) {
  std::lock_guard<std::recursive_mutex> g(globals().sync);
  globals().outFn = f;
  }

Log::Globals& Log::globals() {
  static Globals g;
  return g;
  }

void Log::flush(Context& ctx, char *&out, size_t &count) {
  std::lock_guard<std::recursive_mutex> g(globals().sync);

  if(count!=sizeof(ctx.buffer)) {
    *out = '\0';
#ifdef __ANDROID__
    switch(ctx.mode) {
      case Error:
        __android_log_print(ANDROID_LOG_ERROR, logCategory, "%s", ctx.buffer);
        break;
      case Debug:
        __android_log_print(ANDROID_LOG_DEBUG, logCategory, "%s", ctx.buffer);
        break;
      case Info:
      default:
        __android_log_print(ANDROID_LOG_INFO,  logCategory, "%s", ctx.buffer);
        break;
      }
#elif defined(__WINDOWS_PHONE__)
#if !defined(_DEBUG)
  OutputDebugStringA(ctx.buffer);
  OutputDebugStringA("\r\n");
#endif
#else
#if defined(_MSC_VER) && !defined(_NDEBUG)
  (void)ctx;
  OutputDebugStringA(ctx.buffer);
  OutputDebugStringA("\r\n");
#else
  if(ctx.mode==Error){
    std::cerr << ctx.buffer << std::endl;
    std::cerr.flush();
    } else
    std::cout << ctx.buffer << std::endl;
#endif
#endif
    }

  if(globals().outFn) {
    char cpy[sizeof(ctx.buffer)] = {};
    std::memcpy(cpy,ctx.buffer,count);
    out   = ctx.buffer;
    count = sizeof(ctx.buffer);
    globals().outFn(ctx.mode,ctx.buffer);
    } else {
    out   = ctx.buffer;
    count = sizeof(ctx.buffer);
    }
  }

void Log::printImpl(Context& ctx, char* out, size_t count) {
  flush(ctx,out,count);
  }

void Log::write(Context& ctx, char *&out, size_t &count, const Tempest::Rect& msg) {
  char sym[48];
  snprintf((char*)sym,sizeof(sym),"(%i,%i,%i,%i)",msg.x,msg.y,msg.w,msg.h);
  write(ctx,out,count,(char*)sym);
  }

void Log::write(Context& ctx, char *&out, size_t &count, const Tempest::Size& msg) {
  char sym[48];
  snprintf((char*)sym,sizeof(sym),"(%i,%i)",msg.w,msg.h);
  write(ctx,out,count,(char*)sym);
  }

void Log::write(Context& ctx, char *&out, size_t &count, const Tempest::Point& msg) {
  char sym[48];
  snprintf((char*)sym,sizeof(sym),"(%f,%f)",double(msg.x),double(msg.y));
  write(ctx,out,count,(char*)sym);
  }

void Log::write(Context& ctx, char *&out, size_t &count, const Tempest::Vec4& msg) {
  char sym[48];
  snprintf((char*)sym,sizeof(sym),"(%f,%f,%f,%f)",double(msg.x),double(msg.y),double(msg.z),double(msg.w));
  write(ctx,out,count,(char*)sym);
  }

void Log::write(Context& ctx, char *&out, size_t &count, const Tempest::Vec3& msg) {
  char sym[48];
  snprintf((char*)sym,sizeof(sym),"(%f,%f,%f)",double(msg.x),double(msg.y),double(msg.z));
  write(ctx,out,count,(char*)sym);
  }

void Log::write(Context& ctx, char *&out, size_t &count, const Tempest::Vec2& msg) {
  char sym[48];
  snprintf((char*)sym,sizeof(sym),"(%f,%f)",double(msg.x),double(msg.y));
  write(ctx,out,count,(char*)sym);
  }

void Log::write(Context& ctx, char *&out, size_t &count, const Tempest::Vec1& msg) {
  char sym[48];
  snprintf((char*)sym,sizeof(sym),"(%f)",double(msg.x));
  write(ctx,out,count,(char*)sym);
  }

void Log::write(Context& ctx, char *&out, size_t &count, const std::string &msg) {
  write(ctx,out,count,msg.c_str());
  }

void Log::write(Context& ctx, char *&out, size_t &count, const std::string_view msg) {
  write(ctx,out,count,msg.data());
  }

void Log::write(Context& ctx, char *&out, size_t &count, float msg) {
  char sym[16];
  snprintf(sym,sizeof(sym),"%f",double(msg));
  write(ctx,out,count,sym);
  }

void Log::write(Context& ctx, char *&out, size_t &count, double msg) {
  char sym[16];
  snprintf(sym,sizeof(sym),"%f",msg);
  write(ctx,out,count,sym);
  }

void Log::write(Context& ctx, char *&out, size_t &count, const void *msg) {
  char sym[sizeof(void*)*2+3];
  sym[sizeof(sym)-1] = '\0';
  int pos = sizeof(sym)-2;

  auto ptr = uintptr_t(msg);
  for(size_t i=0; i<sizeof(void*)*2; ++i){
    auto num = ptr%16;
    sym[pos] = num<=9 ? char(num+'0') : char(num-10+'a');
    ptr/=16;
    --pos;
    }
  sym[pos] = 'x';
  --pos;
  sym[pos] = '0';
  write(ctx,out,count,sym+pos);
  }

void Log::write(Context& ctx, char*& out, size_t& count, std::thread::id msg) {
  std::hash<std::thread::id> h;
  write(ctx,out,count,uint64_t(h(msg)));
  }

void Log::write(Context& ctx, char*& out, size_t& count, int16_t msg){
  writeInt(ctx,out,count,msg);
  }

void Log::write(Context& ctx, char *&out, size_t& count, uint16_t msg) {
  writeUInt(ctx,out,count,msg);
  }

void Log::write(Context& ctx, char*& out, size_t& count, const int32_t& msg){
  writeInt(ctx,out,count,msg);
  }

void Log::write(Context& ctx, char*& out, size_t& count, const uint32_t& msg) {
  writeUInt(ctx,out,count,msg);
  }

void Log::write(Context& ctx, char*& out, size_t& count, const int64_t& msg) {
  writeInt(ctx,out,count,msg);
  }

void Log::write(Context& ctx, char*& out, size_t& count, const uint64_t& msg) {
  writeUInt(ctx,out,count,msg);
  }

void Log::write(Context& ctx, char*& out, size_t& count, const char* msg){
  while(*msg) {
    if(count<=1)
      flush(ctx,out,count);
    if(*msg=='\n'){
      flush(ctx,out,count);
      ++msg;
      } else {
      *out = *msg;
      ++out;
      ++msg;
      --count;
      }
    }
  }

void Log::write(Context& ctx, char *&out, size_t &count, char msg) {
  if(count<=1)
    flush(ctx,out,count);
  *out = msg;
  ++out;
  ++msg;
  --count;
  }

void Log::write(Context& ctx, char *&out, size_t &count, int8_t msg) {
  writeInt(ctx,out,count,msg);
  }

void Log::write(Context& ctx, char *&out, size_t &count, uint8_t msg) {
  writeUInt(ctx,out,count,msg);
  }

template< class T >
void Log::writeInt(Context& ctx, char*& out, size_t& count, T msg){
  char sym[32];
  sym[sizeof(sym)-1] = '\0';
  int pos = sizeof(sym)-2;

  if(msg<0){
    write(ctx,out,count,'-');
    msg = -msg;
    }

  while(pos>=0){
    sym[pos] = msg%10+'0';
    msg/=10;
    if(msg==0)
      break;
    --pos;
    }
  write(ctx,out,count,sym+pos);
  }
