cmake_minimum_required(VERSION 3.18)
project(ploxd VERSION 1.0 DESCRIPTION "Lambda Container Library")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_COMPILER clang15)
set(CMAKE_CPP_COMPILER clang++15)

link_directories(/usr/local/lib)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})

if (CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DACTIVE_LEVEL=LEVEL_DEBUG)
elseif(CMAKE_BUILD_TYPE MATCHES Perf)
  add_definitions(-DACTIVE_LEVEL=LEVEL_ERROR)
else()
  add_definitions(-DACTIVE_LEVEL=LEVEL_INFO)
endif()


add_subdirectory(src)
add_subdirectory(tests)
