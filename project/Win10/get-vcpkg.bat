@echo off
set repo=https://github.com/afedchin/vcpkg
set version=v0.0.2
set zip=%cd%\..\Win32BuildSetup\tools\7z\7za.exe
set wget=%cd%\..\BuildDependencies\bin\wget.exe
set download=yes

if not exist vcpkg (
  echo Cloning vcpkg...
  call git clone -b kodi-deps --single-branch %repo%.git
)

set arch=x86
if "%1" == "x64" (
  set arch=x64
)
if "%1" == "arm" (
  set arch=arm
)

pushd vcpkg
  if not exist vcpkg.exe (
    echo Building and installing vcpkg...
    echo A | .\bootstrap-vcpkg.bat
  )
  echo Installing common tools
  call vcpkg install libfribidi swig
popd

if not "%download%" == "yes" goto install
set archive=vcpkg-export-%arch%-uwp-%version%.7z
set URL=%repo%/releases/download/%version%/%archive%

if exist %archive% goto extract
echo Downloading pre-built kodi-deps:%arch%-uwp
%wget% --tries=5 --retry-connrefused --waitretry=2 --no-check-certificate -c %URL%

:extract
if not exist %archive% goto install

echo Extracting pre-built kodi-deps:%arch%-uwp
%zip% x %archive% -o.\temp -y
for /D %%G in ("%cd%\temp\*") do (
  xcopy .\temp\%%~nxG %cd%\vcpkg /iycqs
  goto forend
)
:forend

if exist temp (
  rd /S /Q .\temp
)

if exist vcpkg\installed\%arch%-uwp\share\kodi-deps goto end

:install
pushd vcpkg
  echo Building and installing kodi-deps:%arch%-uwp
  rem need to install windows version first to create include files with win32 tools
  call vcpkg install kodi-deps:%arch%-uwp
popd

:end
echo kodi-deps:%arch%-uwp installed
