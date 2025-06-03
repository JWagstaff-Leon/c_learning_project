add_library(chat_connection OBJECT)

target_include_directories(chat_connection
  PUBLIC
    api

  PRIVATE
    inc

    Common::chat_event
    Common::chat_event_io
    Common::chat_user
    Common::message_queue
    Common::network_watcher
    Common::types
)

target_sources(chat_connection
  PRIVATE
    src/chat_connection.c
    src/chat_connection_cback.c
    src/chat_connection_fsm.c
)
