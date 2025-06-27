add_subdirectory(chat_client ./chat_client)
add_subdirectory(client_ui ./client_ui)

add_executable(client)

set_target_properties(client PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ./lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_link_libraries(client
  PRIVATE
    chat_client
    client_ui

    Common::chat_connection
    Common::chat_event
    Common::chat_event_io
    Common::chat_user
    Common::message_queue
    Common::network_watcher
    Common::types

    ncurses
)

target_include_directories(client
  PRIVATE
    inc
)

target_sources(client
  PRIVATE
    client.c

    src/client_fsm.c
    src/client_cback.c
)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(client
      PUBLIC
        -O3
    )
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(client
      PUBLIC
        -g
    )

    target_link_options(client
      PUBLIC
        -fsanitize=address
    )
endif()
