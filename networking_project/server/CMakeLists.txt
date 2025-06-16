cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_definitions(_GNU_SOURCE)

add_subdirectory(chat_auth ${CMAKE_BINARY_DIR}/chat_auth)
add_subdirectory(chat_clients ${CMAKE_BINARY_DIR}/chat_clients)
add_subdirectory(chat_server ${CMAKE_BINARY_DIR}/chat_server)
add_subdirectory(../common ${CMAKE_BINARY_DIR}/common)

add_executable(server)

target_link_libraries(server
  PRIVATE
    chat_auth
    chat_clients
    chat_server
    
    Common::chat_connection
    Common::chat_event
    Common::chat_event_io
    Common::message_queue
    Common::network_watcher
    Common::shared_ptr
    Common::types
)

target_sources(server
  PRIVATE
    server.c
)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(server
      PUBLIC
        -g
    )

    target_link_options(server
      PUBLIC
        -fsanitize=address
    )
endif()
