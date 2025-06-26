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
  PUBLIC
    Common::chat_user
    Common::shared_ptr
    Common::types

  PRIVATE
    Common::chat_event
    Common::message_queue
)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(chat_auth
      PUBLIC
        -O3
    )
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(chat_auth
      PUBLIC
        -g
    )

    target_link_options(chat_auth
      PUBLIC
        -fsanitize=address
    )
endif()
