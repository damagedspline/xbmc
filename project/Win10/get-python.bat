@echo off

if not exist cpython (
  echo Cloning cpython...
  call git clone -b 2.7-winrt --single-branch https://github.com/afedchin/cpython.git
)

set platform=Win32
set arch=x86
if "%1" == "x64" (
  set platform=x64
  set arch=x64
)
if "%1" == "arm" (
  set platform=ARM
  set arch=arm
)
set triplet=%arch%-uwp
set dependsdir=BuildDependencies\%triplet%

if exist %dependsdir%\system\python (
  rd /S /Q %dependsdir%\system\python
)

if exist %dependsdir%\include\python (
  rd /S /Q %dependsdir%\include\python
)

if exist %dependsdir%\lib\python27_d.lib (
  rd /S /Q %dependsdir%\lib\python27_d.lib
)

if exist %dependsdir%\lib\python27.lib (
  rd /S /Q %dependsdir%\lib\python27.lib
)

if exist %dependsdir%\bin\python27_d.dll (
  rd /S /Q %dependsdir%\bin\python27_d.dll
)

if exist %dependsdir%\bin\python27.dll (
  rd /S /Q %dependsdir%\bin\python27.dll
)

if not exist %dependsdir%\system\python do mkdir %dependsdir%\system\python
if not exist %dependsdir%\lib do mkdir %dependsdir%\%subdir%\lib
if not exist %dependsdir%\bin do mkdir %dependsdir%\%subdir%\bin
if not exist %dependsdir%\%subdir%\include\python do mkdir %dependsdir%\include\python

rem goto:install

set buildpath=%cd%\cpython\%triplet%

pushd cpython\PCBuild
  echo Building and installing cpython
  call "%VS140COMNTOOLS%vsvars32.bat"
  call msbuild pcbuild.sln /p:Configuration="Release" /p:Platform="%platform%" /p:BuildPath="%buildpath%" /m
popd


:install 

echo Copying python include files...
pushd cpython
  xcopy Include ..\%dependsdir%\include\python\ /iycqs  
  xcopy PC\pyconfig.h ..\%dependsdir%\include\python\ /iycq  
popd

echo Copying python binaries...
pushd cpython\%triplet%
  xcopy python27.lib ..\..\%dependsdir%\lib\ /iycq  
  xcopy python27.dll ..\..\%dependsdir%\bin\ /iycq  
  
  xcopy _ctypes.pyd ..\..\%dependsdir%\system\python\DLLs\ /iycq  
  xcopy _elementtree.pyd ..\..\%dependsdir%\system\python\DLLs\ /iycq  
  xcopy _hashlib.pyd ..\..\%dependsdir%\system\python\DLLs\ /iycq  
  xcopy _multiprocessing.pyd ..\..\%dependsdir%\system\python\DLLs\ /iycq  
  xcopy _socket.pyd ..\..\%dependsdir%\system\python\DLLs\ /iycq  
  xcopy _sqlite3.pyd ..\..\%dependsdir%\system\python\DLLs\ /iycq  
  xcopy _ssl.pyd ..\..\%dependsdir%\system\python\DLLs\ /iycq  
  xcopy bz2.pyd ..\..\%dependsdir%\system\python\DLLs\ /iycq  
  xcopy pyexpat.pyd ..\..\%dependsdir%\system\python\DLLs\ /iycq  
  xcopy select.pyd ..\..\%dependsdir%\system\python\DLLs\ /iycq  
  xcopy unicodedata.pyd ..\..\%dependsdir%\system\python\DLLs\ /iycq  
popd

echo Copying python LIB files...
pushd cpython
  xcopy Lib ..\%dependsdir%\system\python\Lib /iycqs  
popd

echo Removing unused python LIB files...
pushd %dependsdir%\system\python\Lib
  rd /S /Q plat-aix3
  rd /S /Q plat-aix4
  rd /S /Q plat-atheos
  rd /S /Q plat-beos5
  rd /S /Q plat-darwin
  rd /S /Q plat-freebsd4
  rd /S /Q plat-freebsd5
  rd /S /Q plat-freebsd6
  rd /S /Q plat-freebsd7
  rd /S /Q plat-freebsd8
  rd /S /Q plat-generic
  rd /S /Q plat-irix5
  rd /S /Q plat-irix6
  rd /S /Q plat-linux2
  rd /S /Q plat-mac
  rd /S /Q plat-netbsd1
  rd /S /Q plat-next3
  rd /S /Q plat-os2emx
  rd /S /Q plat-riscos
  rd /S /Q plat-sunos5
  rd /S /Q plat-unixware7
  rd /S /Q test
  del /S /F /Q *.exe
popd


echo cpython installed

:exit
