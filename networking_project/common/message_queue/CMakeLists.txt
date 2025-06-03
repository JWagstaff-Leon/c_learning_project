add_library(message_queue OBJECT)

target_include_directories(message_queue
  PUBLIC
    api

  PRIVATE
    inc

    Common::types
)

target_sources(message_queue
  PRIVATE
    src/message_queue.c
)

add_library(Common::message_queue ALIAS message_queue)
