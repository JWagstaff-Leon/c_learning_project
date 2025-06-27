cmake_minimum_required(VERSION 3.22.1)

project(c-chat C)

add_compile_definitions(_GNU_SOURCE)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    add_compile_options(
        -O3
    )
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options(
        -g
    )

    add_link_options(
        -fsanitize=address
    )
endif()

add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(common)
