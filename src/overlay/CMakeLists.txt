add_library(overlay SHARED interposition.cc)
target_include_directories(overlay PRIVATE ../../include)
target_include_directories(overlay PRIVATE .)
target_include_directories(overlay PRIVATE /usr/local/include)

if (${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
	link_directories(BEFORE /usr/local/lib)
	target_link_libraries(overlay /usr/local/lib/x86_64-unknown-freebsd13.2/libunwind.so fmt)
else()
  message(FATAL_ERROR "Lambda requires to be built and run on FreeBSD")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_compile_options(overlay PRIVATE "-fpic")
target_link_options(overlay PRIVATE "-rdynamic")
