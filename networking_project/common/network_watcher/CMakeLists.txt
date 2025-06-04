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
  PRIVATE
    Common::message_queue
    Common::types
)

add_library(Common::network_watcher ALIAS network_watcher)
