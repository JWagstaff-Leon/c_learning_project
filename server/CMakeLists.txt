add_subdirectory(chat_auth ./chat_auth)
add_subdirectory(chat_clients ./chat_clients)
add_subdirectory(chat_server ./chat_server)

add_executable(server)

set_target_properties(server PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ./lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

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

if(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(server
      PUBLIC
        -O3
    )
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(server
      PUBLIC
        -g
    )

    target_link_options(server
      PUBLIC
        -fsanitize=address
    )
endif()
