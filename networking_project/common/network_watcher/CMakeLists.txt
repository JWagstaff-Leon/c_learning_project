add_library(network_watcher OBJECT)

target_include_directories(network_watcher
  PUBLIC
    api

  PRIVATE
    inc

    Common::message_queue
    Common::types
)

target_sources(network_watcher
  PRIVATE
    src/network_watcher.c
    src/network_watcher_fsm.c
)

add_library(Common::network_watcher ALIAS network_watcher)
