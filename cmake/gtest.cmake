cmake_minimum_required(VERSION 3.2)
project(gtest_builder C CXX)
include(ExternalProject)



execute_process(
  COMMAND "tar" "-xzf" "${CMAKE_SOURCE_DIR}/ext/googletest/googletest-release-1.12.1.tar.gz"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/ext/googletest"
  RESULT_VARIABLE tar_result
  )

message(STATUS "tar result: ${tar_result} ${CMAKE_SOURCE_DIR}/ext/googletest/googletest-release-1.12.1.tar.gz")

ExternalProject_Add(googletest
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/ext/googletest/googletest-release-1.12.1"
  CMAKE_ARGS -Dgtest_force_shared_crt=ON
  PREFIX "${CMAKE_CURRENT_BINARY_DIR}"
  
# Disable install step
  INSTALL_COMMAND ""
)

# Specify include dir
ExternalProject_Get_Property(googletest source_dir)
set(GTEST_INCLUDE_DIRS ${source_dir}/googletest/include CACHE PATH "" FORCE)

# Specify MainTest's link libraries
ExternalProject_Get_Property(googletest binary_dir)
set(GTEST_LIBS_DIR ${binary_dir}/lib CACHE PATH "" FORCE)
message( status "gtest libdir: " ${GTEST_LIBS_DIR})
