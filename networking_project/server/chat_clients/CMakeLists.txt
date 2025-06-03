add_library(chat_clients OBJECT)

target_include_directories(chat_clients
  PUBLIC
    api

  PRIVATE
    inc

    Common::chat_event
    Common::message_queue
    Common::types
)

target_sources(chat_clients
  PRIVATE
    src/chat_clients.c
    src/chat_clients_cback.c
    src/chat_clients_fsm.c
    src/chat_clients_internal.c
)
