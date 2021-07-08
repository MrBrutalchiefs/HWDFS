project(scheme)

include_directories(include)

aux_source_directory(src SCHEME_SRCS)

add_library(scheme ${SCHEME_SRCS})
target_link_libraries(scheme common)