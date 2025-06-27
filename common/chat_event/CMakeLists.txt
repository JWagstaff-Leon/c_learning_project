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
  PUBLIC
    Common::chat_user
    Common::types
)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(chat_event
      PUBLIC
        -O3
    )
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(chat_event
      PUBLIC
        -g
    )

    target_link_options(chat_event
      PUBLIC
        -fsanitize=address
    )
endif()

add_library(Common::chat_event ALIAS chat_event)
