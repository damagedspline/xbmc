# -------- Architecture settings ---------

check_symbol_exists(_X86_ "Windows.h" _X86_)
check_symbol_exists(_AMD64_ "Windows.h" _AMD64_)
check_symbol_exists(_ARM_ "Windows.h" _ARM_)

if(_X86_)
   set(ARCH win32)
   set(SDK_TARGET_ARCH x86)
elseif(_AMD64_)
   set(ARCH x64)
   set(SDK_TARGET_ARCH x64)
elseif(_ARM_)
   set(ARCH arm)
   set(SDK_TARGET_ARCH arm)
else()
   message(FATAL_ERROR "Unsupported architecture")
endif()
 
unset(_X86_)
unset(_AMD64_)
unset(_ARM_)

# -------- Paths (mainly for find_package) ---------

set(PLATFORM_DIR platform/win32)
set(CORE_MAIN_SOURCE ${CMAKE_SOURCE_DIR}/xbmc/platform/win10/main.cpp)

# Precompiled headers fail with per target output directory. (needs CMake 3.1)
set(PRECOMPILEDHEADER_DIR ${PROJECT_BINARY_DIR}/${CORE_BUILD_CONFIG}/objs)

set(CMAKE_SYSTEM_NAME WindowsStore)
set(CORE_SYSTEM_NAME "windowsstore")
set(PACKAGE_GUID "281d668b-5739-4abd-b3c2-ed1cda572ed2")
set(APP_MANIFEST_NAME package.appxmanifest)
# change this after paccking dependencies
set(VCPKG_ROOT_DIR ${CMAKE_SOURCE_DIR}/project/win10/vcpkg/installed/${SDK_TARGET_ARCH}-uwp)
set(BUILDDEPENDENCIES_ROOT_DIR ${CMAKE_SOURCE_DIR}/project/win10/BuildDependencies/${SDK_TARGET_ARCH}-uwp)

list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${VCPKG_ROOT_DIR})
list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${VCPKG_ROOT_DIR}/debug)
list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${BUILDDEPENDENCIES_ROOT_DIR})
# for swig
list(APPEND CMAKE_SYSTEM_PREFIX_PATH ${CMAKE_SOURCE_DIR}/project/win10/BuildDependencies/)
# for python
set(PYTHON_INCLUDE_DIR ${BUILDDEPENDENCIES_ROOT_DIR}/include/python)


# -------- Compiler options ---------

add_options(CXX ALL_BUILDS "/wd\"4996\"")
add_options(CXX ALL_BUILDS "/wd\"4146\"")
add_options(CXX ALL_BUILDS "/wd\"4251\"")
set(ARCH_DEFINES -D_WINDOWS -DTARGET_WIN10 -DXBMC_EXPORT -DMS_UWP)
if(NOT SDK_TARGET_ARCH STREQUAL arm)
  set(ARCH_DEFINES "${ARCH_DEFINES} -D__SSE__ -D__SSE2__")
endif()
set(SYSTEM_DEFINES -DNOMINMAX -D_USE_32BIT_TIME_T -DHAS_DX -D__STDC_CONSTANT_MACROS
                   -DFMT_HEADER_ONLY -DTAGLIB_STATIC -DNPT_CONFIG_ENABLE_LOGGING
                   -DPLT_HTTP_DEFAULT_USER_AGENT="UPnP/1.0 DLNADOC/1.50 Kodi"
                   -DPLT_HTTP_DEFAULT_SERVER="UPnP/1.0 DLNADOC/1.50 Kodi"
                   $<$<CONFIG:Debug>:-DD3D_DEBUG_INFO>)

# Make sure /FS is set for Visual Studio in order to prevent simultaneous access to pdb files.
if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set(CMAKE_CXX_FLAGS "/MP /FS /ZW /EHsc ${CMAKE_CXX_FLAGS}")
endif()

# Google Test needs to use shared version of runtime libraries
set(gtest_force_shared_crt ON CACHE STRING "" FORCE)


# -------- Linker options ---------

# For #pragma comment(lib X)
# TODO: It would certainly be better to handle these libraries via CMake modules.
link_directories(${VCPKG_ROOT_DIR}/lib
                 ${VCPKG_ROOT_DIR}/debug/lib
                 ${BUILDDEPENDENCIES_ROOT_DIR}/lib)

list(APPEND DEPLIBS d3d11.lib WS2_32.lib dxguid.lib dloadhelper.lib)
if(ARCH STREQUAL win32 OR ARCH STREQUAL x64)
  list(APPEND DEPLIBS DInput8.lib DSound.lib winmm.lib Mpr.lib Iphlpapi.lib PowrProf.lib setupapi.lib dwmapi.lib)
endif()
# NODEFAULTLIB option

set(_nodefaultlibs_RELEASE libcmt)
set(_nodefaultlibs_DEBUG libcmt msvcrt)
foreach(_lib ${_nodefaultlibs_RELEASE})
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /NODEFAULTLIB:\"${_lib}\"")
endforeach()
foreach(_lib ${_nodefaultlibs_DEBUG})
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:\"${_lib}\"")
endforeach()

#[[ no need for uwp
set(FFMPEG_LOAD_DLLS avcodec-57.dll avdevice-57.dll avfilter-6.dll avformat-57.dll 
                     avutil-55.dll postproc-54.dll swresample-2.dll swscale-4.dll)
# DELAYLOAD option
set(_delayloadlibs ${FFMPEG_LOAD_DLLS} libmysql.dll libxslt.dll dnssd.dll dwmapi.dll 
                                       zlib1.dll ssh.dll sqlite3.dll)
foreach(_lib ${_delayloadlibs})
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DELAYLOAD:\"${_lib}\"")
endforeach()
]]

file(GLOB _KODI_UWP_DEPENDS_DLLS "${VCPKG_ROOT_DIR}/bin/*.dll")
foreach(_dll_file ${_KODI_UWP_DEPENDS_DLLS})
  get_filename_component(_dll_file_name ${_dll_file} NAME)
  list(APPEND KODI_UWP_DEPENDS_DLLS ${_dll_file_name})
endforeach()

# Make the Release version create a PDB
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
# Minimize the size or the resulting DLLs
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF")


# -------- Visual Studio options ---------

if(CMAKE_GENERATOR MATCHES "Visual Studio")
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  # Generate a batch file that opens Visual Studio with the necessary env variables set.
  file(WRITE ${CMAKE_BINARY_DIR}/kodi-sln.bat
             "@echo off\n"
             "set KODI_HOME=%~dp0\n"
             "set PATH=%~dp0\\system\n"
             "start %~dp0\\${PROJECT_NAME}.sln")
endif()
