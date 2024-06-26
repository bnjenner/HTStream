cmake_minimum_required (VERSION 3.2)
project (hts_ExtractUMI)

file(GLOB sources "src/*.cpp")

message( status "sources: " ${sources})

set(PROJECT_LINK_LIBS hts_common)
link_directories(${CMAKE_BINARY_DIR}/common)

FIND_PACKAGE( Boost 1.56 COMPONENTS system program_options iostreams filesystem REQUIRED )
find_package(Threads REQUIRED)
include_directories(${COMMON_INCLUDES} ${Boost_INCLUDE_DIR} )


add_executable(${PROJECT_NAME} ${sources})
if (BUILD_STATIC_BIN)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_SEARCH_START_STATIC 1)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_SEARCH_END_STATIC 1)
endif()
target_link_libraries(${PROJECT_NAME} ${EXTRA_BUILD_FLAGS} ${PROJECT_LINK_LIBS} ${Boost_LIBRARIES})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

## test
set(PROJECT_TEST_NAME ${PROJECT_NAME}_test)
find_package(Threads REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ${Boost_INCLUDE_DIR}
    ${COMMON_INCLUDES} ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/src)

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp)
link_directories(${GTEST_LIBS_DIR})
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}.cpp)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dprivate=public -lboost_serialization -lboost_unit_test_framework-mt -lboost_program_options")

add_executable(${PROJECT_TEST_NAME} ${TEST_SRC_FILES} ${sources})
#add_dependencies(${PROJECT_TEST_NAME} googletest)
target_link_libraries(${PROJECT_TEST_NAME} gtest gtest_main ${CMAKE_THREAD_LIBS_INIT} ${PROJECT_LINK_LIBS})
add_test(${PROJECT_TEST_NAME} ${PROJECT_TEST_NAME})
