cmake_minimum_required(VERSION 4.5)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

set(package_name fcopy)
project(${package_name} VERSION 0.1.0)

option(FCOPY_BUILD_CLI "Build fcopy cli" ON)

add_subdirectory(ext)
add_subdirectory(lib)

if (FCOPY_BUILD_CLI)
  add_subdirectory(cli)
endif()
