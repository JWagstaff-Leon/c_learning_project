add_library(chat_connection OBJECT)

target_include_directories(chat_connection
  PUBLIC
    api

  PRIVATE
    inc
)

target_sources(chat_connection
  PRIVATE
    src/chat_connection.c
    src/chat_connection_cback.c
    src/chat_connection_fsm.c
)

target_link_libraries(chat_connection
  PUBLIC
    Common::chat_event
    Common::chat_user
    Common::types

  PRIVATE
    Common::chat_event_io
    Common::message_queue
    Common::network_watcher
)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(chat_connection
      PUBLIC
        -O3
    )
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(chat_connection
      PUBLIC
        -g
    )

    target_link_options(chat_connection
      PUBLIC
        -fsanitize=address
    )
endif()

add_library(Common::chat_connection ALIAS chat_connection)