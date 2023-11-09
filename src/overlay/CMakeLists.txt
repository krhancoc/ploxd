add_library(overlay SHARED interposition.cc)
target_include_directories(overlay PRIVATE ../../include)
target_include_directories(overlay PRIVATE .)

if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
	target_link_libraries(overlay /usr/local/lib/libunwind.so)
else()
  message(FATAL_ERROR "Lambda requires to be built and run on FreeBSD")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_compile_options(overlay PRIVATE "-fpic")
target_link_options(overlay PRIVATE "-rdynamic")
