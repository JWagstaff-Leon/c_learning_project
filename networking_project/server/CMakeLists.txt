cmake_minimum_required(VERSION 3.22.1)


add_subdirectory(chat_auth chat_auth)
add_subdirectory(chat_clients chat_clients)
add_subdirectory(chat_connection chat_connection)
add_subdirectory(chat_server chat_server)
add_subdirectory(../common ../common)

add_executable(server)

target_link_libraries(server
    chat_auth
    chat_clients
    chat_connection
    chat_server

    Common::chat_event
    Common::chat_event_io
    Common::message_queue
    Common::network_watcher
    Common::types
)

target_include_directories(server
  PRIVATE
    chat_auth
    chat_clients
    chat_connection
    chat_server

    Common::chat_event
    Common::chat_event_io
    Common::chat_user
    Common::message_queue
    Common::network_watcher
)

target_sources(server
  PRIVATE
    server.c
)
