cmake_minimum_required(VERSION 3.10)

add_library(lib-chunk-impl
    json.cpp
    bson.cpp
    get_stream.cpp
    common.cpp
    columnar.cpp
)

add_dependencies(lib-chunk-impl rapidjson)

target_include_directories(lib-chunk-impl PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${RAPIDJSON_INCLUDE_DIR}
)

target_link_libraries(lib-chunk-impl PUBLIC
    lib-document
    lib-dremel
)

add_subdirectory(dremel)
