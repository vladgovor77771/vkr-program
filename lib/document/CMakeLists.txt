cmake_minimum_required(VERSION 3.10)

add_library(lib-document
    document.cpp
)

target_include_directories(lib-document PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
