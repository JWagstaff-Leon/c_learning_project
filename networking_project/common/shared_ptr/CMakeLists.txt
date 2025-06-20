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

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(shared_ptr
      PUBLIC
        -g
    )

    target_link_options(shared_ptr
      PUBLIC
        -fsanitize=address
    )
endif()

add_library(Common::shared_ptr ALIAS shared_ptr)

option(SHARED_PTR_DEBUG "Extra printouts on stderr for debugging shared pointer functionality" OFF)

if (SHARED_PTR_DEBUG)
    target_compile_definitions(shared_ptr
      PUBLIC
        -DSHARED_PTR_DEBUG
    )
endif()
