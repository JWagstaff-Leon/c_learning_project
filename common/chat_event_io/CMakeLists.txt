add_library(chat_event_io OBJECT)

target_include_directories(chat_event_io
  PUBLIC
    api

  PRIVATE
    inc
)

target_sources(chat_event_io
  PRIVATE
    src/chat_event_io.c
    src/chat_event_io_fsm.c
    src/chat_event_io_fsm_reader.c
    src/chat_event_io_fsm_writer.c
)

target_link_libraries(chat_event_io
  PUBLIC
    Common::chat_event
    Common::types

  PRIVATE
    Common::message_queue
)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(chat_event_io
      PUBLIC
        -O3
    )
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(chat_event_io
      PUBLIC
        -g
    )

    target_link_options(chat_event_io
      PUBLIC
        -fsanitize=address
    )
endif()

add_library(Common::chat_event_io ALIAS chat_event_io)
