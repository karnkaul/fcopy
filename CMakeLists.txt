cmake_minimum_required(VERSION 4.4)

if (CMAKE_VERSION VERSION_LESS "4.5.0")
  set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "f35a9ac6-8463-4d38-8eec-5d6008153e7d")
  set(CMAKE_CXX_MODULE_STD ON)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_DEBUG_POSTFIX "-d")

set(package_name fcopy)
project(${package_name} VERSION 0.1.2)

option(FCOPY_BUILD_CLI "Build fcopy cli" ${PROJECT_IS_TOP_LEVEL})
option(FCOPY_INSTALL "Setup fcopy cli install" ${PROJECT_IS_TOP_LEVEL})

add_subdirectory(ext)
add_subdirectory(lib)

if (FCOPY_BUILD_CLI)
  add_subdirectory(cli)
endif()
