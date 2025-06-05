add_library(chat_server OBJECT)

target_include_directories(chat_server
  PUBLIC
    api

  PRIVATE
    inc
)

target_sources(chat_server
  PRIVATE
    src/chat_server.c
    src/chat_server_cback.c
    src/chat_server_fsm.c
)

target_link_libraries(chat_server
  PUBLIC
    Common::types

  PRIVATE
    chat_auth
    chat_clients

    Common::message_queue
    Common::network_watcher
)
