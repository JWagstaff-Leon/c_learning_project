add_library(chat_user INTERFACE)

target_include_directories(chat_user
  INTERFACE
    api
)

add_library(Common::chat_user ALIAS chat_user)
