cmake_minimum_required(VERSION 3.10)

project(DocumentsProgram)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

include(ExternalProject)
include(FetchContent)
include("${CMAKE_SOURCE_DIR}/vendor/rapidjson.cmake")
include("${CMAKE_SOURCE_DIR}/vendor/cli11.cmake")

add_subdirectory(lib)
add_subdirectory(bin)
