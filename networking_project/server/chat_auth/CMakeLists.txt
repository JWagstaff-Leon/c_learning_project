add_library(chat_auth OBJECT)

target_include_directories(chat_auth
  PUBLIC
    api

  PRIVATE
    inc

    Common::chat_event
    Common::chat_user
    Common::types

    ../../sqlite
)

target_sources(chat_auth
  PRIVATE
    src/chat_auth.c
    src/chat_auth_fsm.c
    src/chat_auth_internal.c

    ../../sqlite/sqlite3.c
)
