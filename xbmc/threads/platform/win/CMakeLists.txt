if(NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
	set(SOURCES Win32Exception.cpp)
endif()

set(HEADERS Condition.h
            CriticalSection.h
            ThreadImpl.h
            ThreadLocal.h
            Win32Exception.h)

core_add_library(threads_win)
