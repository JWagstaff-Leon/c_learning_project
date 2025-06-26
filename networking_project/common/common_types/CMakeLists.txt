add_library(common_types OBJECT)

target_include_directories(common_types
  PUBLIC
    api
)

target_sources(common_types
  PRIVATE
    src/common_types.c
)

if(CMAKE_BUILD_TYPE MATCHES "Release")
    target_compile_options(common_types
      PUBLIC
        -O3
    )
elseif(CMAKE_BUILD_TYPE MATCHES "Debug")
    target_compile_options(common_types
      PUBLIC
        -g
    )

    target_link_options(common_types
      PUBLIC
        -fsanitize=address
    )
endif()

add_library(Common::types ALIAS common_types)
