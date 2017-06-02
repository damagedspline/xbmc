set(SOURCES WinEventsWin10.cpp
            WinSystemWin10.cpp
            ../windows/VideoSyncD3D.cpp
            WinSystemWin10DX.cpp)

set(HEADERS WinEventsWin10.h
            WinSystemWin10.h
            ../windows/VideoSyncD3D.h
            WinSystemWin10DX.h)

core_add_library(windowing_windowsstore)
