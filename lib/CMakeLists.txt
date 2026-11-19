add_library(${package_name}-lib)
add_library(${package_name}::${package_name}-lib ALIAS ${package_name}-lib)

target_compile_features(${package_name}-lib PUBLIC cxx_std_23)

set(version_module public/build_version.cppm)
configure_file(public/build_version.cppm.in ${version_module})

file(GLOB_RECURSE modules LIST_DIRECTORIES false "public/*.cppm" "private/*.cppm")
file(GLOB_RECURSE sources LIST_DIRECTORIES false "src/*.cpp")

target_sources(${package_name}-lib PUBLIC FILE_SET CXX_MODULES
  BASE_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_BINARY_DIR}"
  
  FILES
  ${modules}
  "${CMAKE_CURRENT_BINARY_DIR}/${version_module}"
)

target_sources(${package_name}-lib PRIVATE
  ${sources}
)
