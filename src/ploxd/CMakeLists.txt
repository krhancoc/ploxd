find_package(fmt REQUIRED)

add_executable(ploxd ploxd.cc policy.cc)
add_executable(plox plox.cc)
target_include_directories(ploxd PRIVATE ../../include)
target_include_directories(ploxd PRIVATE ../../kplox/include)
target_include_directories(ploxd PRIVATE .)


target_link_libraries(ploxd fmt::fmt)

target_include_directories(ploxd PUBLIC /usr/include/)
target_link_directories(ploxd PUBLIC /usr/lib/)
target_link_libraries(ploxd /usr/lib/libplox.a)


# add_executable(client client.cc)
# target_include_directories(client PRIVATE ../include)
# target_include_directories(client PRIVATE .)
#
# target_link_libraries(client ssl fmt::fmt md crypto pthread python3.7m ploxclient)
#
