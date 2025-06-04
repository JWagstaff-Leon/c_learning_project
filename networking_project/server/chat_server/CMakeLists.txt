add_library(chat_server OBJECT)

target_include_directories(chat_server
  PUBLIC
    api

  PRIVATE
    inc

    Common::chat_event
    Common::chat_event_io
    Common::message_queue
    Common::types
)

target_sources(chat_server
  PRIVATE
    src/chat_server.c
    src/chat_server_cback.c
    src/chat_server_fsm.c
)
