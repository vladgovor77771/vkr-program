cmake_minimum_required(VERSION 3.10)

add_library(lib-cli
    read.cpp
    transform.cpp
    common.cpp
    dataset_generator.cpp
)

add_dependencies(lib-cli rapidjson)

target_include_directories(lib-cli PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RAPIDJSON_INCLUDE_DIR}
)

target_link_libraries(lib-cli PUBLIC
    lib-chunk-impl
)
