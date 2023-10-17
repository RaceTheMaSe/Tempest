cmake_minimum_required(VERSION 3.4)

project(OpenAL)
SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../../cmake")

if(MSVC)
  add_definitions(-DNOMINMAX)
endif()

add_definitions(-DAL_LIBTYPE_STATIC)
add_definitions(-DAL_BUILD_LIBRARY)

include_directories("."
                    "include"
                    "openal32/include")

### Platforms
if(WIN32)
  add_definitions(-DHAVE_WINMM)
  #add_definitions(-DHAVE_DSOUND)
  #add_definitions(-DHAVE_SSE)
  add_definitions(-DHAVE_XMMINTRIN_H)
  file(GLOB AlcBackend
    "Alc/backends/winmm.c"
    "Alc/backends/dsound.c"
    "Alc/backends/null.c"
    "Alc/backends/loopback.c"
    )
elseif(ANDROID)
  add_definitions(-DHAVE_OPENSL)
  add_definitions(-DHAVE_NEON)
  add_definitions(-DHAVE_ARM_NEON_H)
  file(GLOB AlcBackend
    "Alc/backends/opensl.c"
    "Alc/backends/loopback.c"
    "Alc/backends/null.c"
    )
elseif(APPLE)
  file(GLOB AlcBackend
    "Alc/backends/null.c"
    "Alc/backends/loopback.c"
    )
elseif(UNIX)
  add_definitions(-DHAVE_ALSA)
  add_definitions(-DHAVE_SSE)
  add_definitions(-DHAVE_XMMINTRIN_H)
  file(GLOB AlcBackend
    "Alc/backends/alsa.c"
    "Alc/backends/null.c"
    "Alc/backends/loopback.c"
    )
endif()

file(GLOB SOURCES
    "*.h"
    "*.c"
    "**/*.h"
    "**/*.c"
  )

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${AlcBackend})

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE dxguid dsound winmm kernel32)
elseif(ANDROID)
  target_link_libraries(${PROJECT_NAME} PRIVATE OpenSLES)
elseif(APPLE)
  #TODO
elseif(UNIX)
  target_link_libraries(${PROJECT_NAME} PRIVATE asound)
endif()