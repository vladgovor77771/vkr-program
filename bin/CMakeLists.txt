cmake_minimum_required(VERSION 3.10)

add_executable(cli
    cli.cpp
    read.cpp
    transform.cpp
    common.cpp
)

target_include_directories(cli PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cli PRIVATE 
    CLI11::CLI11
    lib-chunk-impl
)
