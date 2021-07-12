project(tiny)

include_directories(include)

aux_source_directory(src TINY_SRCS)

add_library(tiny ${TINY_SRCS})