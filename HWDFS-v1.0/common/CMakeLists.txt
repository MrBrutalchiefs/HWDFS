project(common)

include_directories(include)

aux_source_directory(src COMMON_SRCS)

add_library(common ${COMMON_SRCS})
target_link_libraries(common tsl::robin_map)
target_link_libraries(common erasure)
target_link_libraries(common tiny)
target_link_libraries(common ${Boost_LIBRARIES})
target_link_libraries(common StoreService)