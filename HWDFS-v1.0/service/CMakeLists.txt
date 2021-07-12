project(service)

include_directories(include)

aux_source_directory(src DIR_LIB_SRCS)

add_library(StoreService ${DIR_LIB_SRCS})
target_link_libraries(StoreService thrift)