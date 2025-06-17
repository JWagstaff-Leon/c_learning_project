add_library(client_ui OBJECT)

target_include_directories(client_ui
  PUBLIC
    api

  PRIVATE
    inc
)

target_sources(client_ui
  PRIVATE
    src/client_ui.c
)

target_link_libraries(client_ui
  PUBLIC
    Common::types

  PRIVATE
    Common::chat_event
)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(client_ui
      PUBLIC
        -g
    )

    target_link_options(client_ui
      PUBLIC
        -fsanitize=address
    )
endif()
