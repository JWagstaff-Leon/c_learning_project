add_library(chat_auth OBJECT)

target_include_directories(chat_auth
  PUBLIC
    api

  PRIVATE
    inc

    ../../sqlite
)

target_sources(chat_auth
  PRIVATE
    src/chat_auth.c
    src/chat_auth_fsm.c
    src/chat_auth_internal.c

    ../../sqlite/sqlite3.c
)

target_link_libraries(chat_auth
  PRIVATE
    Common::chat_event
    Common::chat_user
    Common::message_queue
    Common::types
)
