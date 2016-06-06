include_directories(common)

add_subdirectory(common)
add_subdirectory(lldb)
if(NOT WIN32)
    add_subdirectory(gdb)
endif()
