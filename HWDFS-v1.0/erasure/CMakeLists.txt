project(erasure)

include_directories(include)

aux_source_directory(src ERASURE_SRCS)

add_library(erasure ${ERASURE_SRCS})