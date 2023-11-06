add_library(shim SHARED interposition.cc)
target_include_directories(shim PRIVATE ../../include)
target_include_directories(shim PRIVATE .)

if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  target_link_libraries(shim)
else()
  message(FATAL_ERROR "Lambda requires to be built and run on FreeBSD")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_compile_options(shim PRIVATE "-fpic")
target_link_options(shim PRIVATE "-rdynamic")
