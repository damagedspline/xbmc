set(SOURCES BaseFrameAllocator.cpp
            GeneralAllocator.cpp
            SysMemFrameAllocator.cpp)

set(HEADERS BaseFrameAllocator.h
            GeneralAllocator.h
            SysMemFrameAllocator.h)

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES D3D11FrameAllocator.cpp)
  list(APPEND HEADERS D3D11FrameAllocator.h)
endif()

core_add_library(dvdvideocodecsmfx)
