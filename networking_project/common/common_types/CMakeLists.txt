add_library(common_types OBJECT)

target_include_directories(common_types
  PUBLIC
    api
)

target_sources(common_types
  PRIVATE
    src/common_types.c
)

add_library(Common::types ALIAS common_types)
