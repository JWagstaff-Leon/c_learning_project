add_library(message_queue OBJECT)

target_include_directories(message_queue
  PUBLIC
    api

  PRIVATE
    inc
)

target_sources(message_queue
  PRIVATE
    src/message_queue.c
)

target_link_libraries(message_queue
  PUBLIC
    Common::types
)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(message_queue
      PUBLIC
        -g
    )

    target_link_options(message_queue
      PUBLIC
        -fsanitize=address
    )
endif()

add_library(Common::message_queue ALIAS message_queue)
