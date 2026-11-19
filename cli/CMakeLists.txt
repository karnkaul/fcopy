add_executable(${package_name})

target_link_libraries(${package_name} PRIVATE
  ${package_name}::${package_name}-lib
  clap::clap
)

target_sources(${package_name} PRIVATE FILE_SET CXX_MODULES FILES src/app.cppm)
target_sources(${package_name} PRIVATE src/main.cpp)
