add_library(shared_ptr OBJECT)

target_include_directories(shared_ptr
  PUBLIC
    api

  PRIVATE
    inc
)

target_sources(shared_ptr
  PRIVATE
    src/shared_ptr.c
)

target_link_libraries(shared_ptr
  PRIVATE
    Common::types
)

add_library(Common::shared_ptr ALIAS shared_ptr)
