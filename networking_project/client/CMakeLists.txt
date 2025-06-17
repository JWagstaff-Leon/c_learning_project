cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_definitions(_GNU_SOURCE)

add_subdirectory(../common ${CMAKE_BINARY_DIR}/common)

add_executable(client)

target_link_libraries(client
  PRIVATE
    Common::chat_connection
    Common::chat_event
    Common::chat_event_io
    Common::chat_user
    Common::message_queue
    Common::network_watcher
    Common::types
)

target_include_directories(client
  PRIVATE
    src
)

target_sources(client
  PRIVATE
    client.c
)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(client
      PUBLIC
        -g
    )

    target_link_options(client
      PUBLIC
        -fsanitize=address
    )
endif()
