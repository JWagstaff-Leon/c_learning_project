add_library(chat_event OBJECT)

target_include_directories(chat_event
  PUBLIC
    api

  PRIVATE
    inc
)

target_sources(chat_event
  PRIVATE
    src/chat_event.c
)

target_link_libraries(chat_event
  PRIVATE
    Common::chat_user
    Common::types
)

add_library(Common::chat_event ALIAS chat_event)
