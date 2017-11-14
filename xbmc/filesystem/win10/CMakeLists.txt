set(SOURCES WinLibraryDirectory.cpp
            WinLibraryFile.cpp
            Win10SMBDirectory.cpp
            Win10SMBFile.cpp)

set(HEADERS WinLibraryDirectory.h
            WinLibraryFile.h
            Win10SMBDirectory.h
            Win10SMBFile.h)

core_add_library(filesystem_win10)
