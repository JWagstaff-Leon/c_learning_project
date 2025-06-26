add_library(network_watcher OBJECT)

target_include_directories(network_watcher
  PUBLIC
    api

  PRIVATE
    inc
)

target_sources(network_watcher
  PRIVATE
    src/network_watcher.c
    src/network_watcher_fsm.c
)

target_link_libraries(network_watcher
  PUBLIC
    Common::types

  PRIVATE
    Common::message_queue
)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(network_watcher
      PUBLIC
        -O3
    )
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(network_watcher
      PUBLIC
        -g
    )

    target_link_options(network_watcher
      PUBLIC
        -fsanitize=address
    )
endif()

add_library(Common::network_watcher ALIAS network_watcher)
