cmake_minimum_required(VERSION 3.10)
project(Laba3_OS VERSION 1.0)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(include)

add_library(crossplatform STATIC 
    src/crossplatform.c 
    src/stringutils.c
)

add_executable(parent src/parent.c)
add_executable(child src/child.c)

target_link_libraries(parent crossplatform)
target_link_libraries(child crossplatform)

if(WIN32)
    target_link_libraries(parent kernel32)
    target_link_libraries(child kernel32)
endif()

set_target_properties(parent child crossplatform
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)