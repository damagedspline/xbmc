@ECHO OFF
SETLOCAL

:: batch file to compile mingw libs via BuildSetup
PUSHD %~dp0\..\..\..
SET WORKDIR=%CD%
POPD

SET PROMPTLEVEL=prompt
SET BUILDMODE=clean
SET opt=mintty
SET build32=yes
SET build64=no
SET vcarch=x86
SET msys2=msys64
SET tools=mingw
SET winstore=no
SET UniversalSDKVer=""
FOR %%b in (%1, %2, %3, %4) DO (
  IF %%b==noprompt SET PROMPTLEVEL=noprompt
  IF %%b==clean SET BUILDMODE=clean
  IF %%b==noclean SET BUILDMODE=noclean
  IF %%b==sh SET opt=sh
  IF %%b==build64 ( 
    SET build64=yes 
    SET build32=no
    SET vcarch=x64
    )
  IF %%b==msvc SET tools=msvc
  IF %%b==store (
    SET tools=msvc
    SET winstore=yes
  )
)
:: Export full current PATH from environment into MSYS2
set MSYS2_PATH_TYPE=inherit

:: setup MSVC env
SET vcstore=""
IF %winstore%==yes (
  SET vcstore=store
  SET UniversalSDKVer=10.0.14393.0
)
call "%VS140COMNTOOLS%..\..\VC\vcvarsall.bat" %vcarch% %vcstore% %UniversalSDKVer% || exit /b 1

:: Prepend the msys and mingw paths onto %PATH%
SET MSYS_INSTALL_PATH=%WORKDIR%\project\BuildDependencies\msys
SET PATH=%MSYS_INSTALL_PATH%\mingw\bin;%MSYS_INSTALL_PATH%\bin;%PATH%

SET ERRORFILE=%WORKDIR%\project\Win32BuildSetup\errormingw

SET BS_DIR=%WORKDIR%\project\Win32BuildSetup

IF EXIST %ERRORFILE% del %ERRORFILE% > NUL

:: compiles a bunch of mingw libs and not more
IF %opt%==sh (
  IF EXIST %WORKDIR%\project\BuildDependencies\%msys2%\usr\bin\sh.exe (
    ECHO starting sh shell
    %WORKDIR%\project\BuildDependencies\%msys2%\usr\bin\sh.exe --login -i /xbmc/tools/buildsteps/windows/make-mingwlibs.sh --prompt=%PROMPTLEVEL% --mode=%BUILDMODE% --build32=%build32% --build64=%build64% --tools=%tools% --win10=%winstore%
    GOTO END
  ) ELSE (
    GOTO ENDWITHERROR
  )
)
IF EXIST %WORKDIR%\project\BuildDependencies\%msys2%\usr\bin\mintty.exe (
  ECHO starting mintty shell
  %WORKDIR%\project\BuildDependencies\%msys2%\usr\bin\mintty.exe -d -i /msys2.ico /usr/bin/bash --login /xbmc/tools/buildsteps/windows/make-mingwlibs.sh --prompt=%PROMPTLEVEL% --mode=%BUILDMODE% --build32=%build32% --build64=%build64% --tools=%tools% --win10=%winstore%
  GOTO END
)
GOTO ENDWITHERROR

:ENDWITHERROR
  ECHO msys environment not found
  ECHO bla>%ERRORFILE%
  EXIT /B 1
  
:END
  ECHO exiting msys environment
  IF EXIST %ERRORFILE% (
    ECHO failed to build mingw libs
    EXIT /B 1
  )
  EXIT /B 0

ENDLOCAL
