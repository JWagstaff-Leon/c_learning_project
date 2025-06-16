add_library(chat_client OBJECT)

target_include_directories(chat_client
  PUBLIC
    api

  PRIVATE
    inc
)

target_sources(chat_client
  PRIVATE
    src/chat_client.c
    src/chat_client_cback.c
    src/chat_client_fsm.c
    src/chat_client_internal.c
)

target_link_libraries(chat_client
  PUBLIC
    Common::chat_event
    Common::types

  PRIVATE
    Common::chat_connection
    Common::chat_user
    Common::message_queue
)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(chat_client
      PUBLIC
        -g
    )

    target_link_options(chat_client
      PUBLIC
        -fsanitize=address
    )
endif()
