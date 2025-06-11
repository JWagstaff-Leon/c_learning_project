add_library(chat_clients OBJECT)

target_include_directories(chat_clients
  PUBLIC
    api

  PRIVATE
    inc
)

target_sources(chat_clients
  PRIVATE
    src/chat_clients.c
    src/chat_clients_cback.c
    src/chat_clients_fsm.c
    src/chat_clients_internal.c
)

target_link_libraries(chat_clients
  PUBLIC
    Common::chat_event
    Common::chat_user
    Common::types
    Common::network_watcher
    Common::shared_ptr

  PRIVATE
    chat_auth
    chat_connection

    Common::chat_event_io
    Common::message_queue
)
