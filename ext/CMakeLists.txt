file(ARCHIVE_EXTRACT INPUT "${CMAKE_CURRENT_SOURCE_DIR}/src.zip" DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}")

if(FCOPY_BUILD_CLI AND NOT TARGET clap::clap)
  message(STATUS "[clap]")
  add_subdirectory(src/clap)
endif()
