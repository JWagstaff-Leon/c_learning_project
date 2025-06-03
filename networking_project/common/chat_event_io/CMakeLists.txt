add_library(chat_event_io OBJECT)

target_include_directories(chat_event_io
  PUBLIC
    api

  PRIVATE
    inc

    Common::chat_event
    Common::message_queue
    Common::types
)

target_sources(chat_event_io
  PRIVATE
    src/chat_event_io.c
    src/chat_event_io_fsm.c
    src/chat_event_io_fsm_reader.c
    src/chat_event_io_fsm_writer.c
)

add_library(Common::chat_event_io ALIAS chat_event_io)
