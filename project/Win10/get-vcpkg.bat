@echo off

if not exist vcpkg (
  echo Cloning vcpkg...
  call git clone -b kodi-deps --single-branch https://github.com/afedchin/vcpkg.git
)

set arch=x86
if "%1" == "x64" (
  set arch=x64
)
if "%1" == "arm" (
  set arch=arm
)

echo Building and installing vcpkg...
pushd vcpkg
  if not exist vcpkg.exe (
    echo A | ./bootstrap-vcpkg.bat
  )
  echo Building and installing kodi-deps:%arch%-uwp
  rem need to install windows version first to create include files with win32 tools
  call vcpkg install libfribidi
  call vcpkg install kodi-deps:%arch%-uwp
popd
echo kodi-deps:%arch%-uwp installed
