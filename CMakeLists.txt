cmake_minimum_required(VERSION 3.5.0)
project(epharma VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(CTest)
enable_testing()

add_library(epharma epharma.cpp)
target_include_directories(epharma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(epharma PUBLIC sqlite3)

add_executable(epharma_test epharma_test.cpp epharma.cpp)
target_link_libraries(epharma_test epharma gtest gtest_main)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
