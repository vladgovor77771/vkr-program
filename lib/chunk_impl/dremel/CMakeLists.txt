cmake_minimum_required(VERSION 3.10)

add_library(lib-dremel
    field_descriptor.cpp
    field_writer.cpp
    field_reader.cpp
    assembly.cpp
)

add_dependencies(lib-dremel rapidjson)

target_include_directories(lib-dremel PUBLIC
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(lib-dremel PUBLIC
    lib-document
)
