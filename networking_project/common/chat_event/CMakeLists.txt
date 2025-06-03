add_library(chat_event OBJECT)

target_include_directories(chat_event
  PUBLIC
    api

  PRIVATE
    Common::chat_user
    Common::types
)

target_sources(chat_event
  PRIVATE
    src/chat_event.c
)

add_library(Common::chat_event ALIAS chat_event)
