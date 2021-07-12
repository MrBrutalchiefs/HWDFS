cmake_minimum_required(VERSION 3.17)
project(HWDFS)
set(CMAKE_CXX_STANDARD 11)

# 引入 Boost.Thread组件
find_package(Boost REQUIRED COMPONENTS thread)
if(NOT Boost_FOUND)
    message("NOT found Boost")
endif()
include_directories(${Boost_INCLUDE_DIRS})

# 引入内部链接库
add_subdirectory(${PROJECT_SOURCE_DIR}/robin-map)
include_directories(${PROJECT_SOURCE_DIR}/robin-map/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/service)
include_directories(${PROJECT_SOURCE_DIR}/service/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/tiny)
include_directories(${PROJECT_SOURCE_DIR}/tiny/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/erasure)
include_directories(${PROJECT_SOURCE_DIR}/erasure/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/common)
include_directories(${PROJECT_SOURCE_DIR}/common/include)

add_subdirectory(${PROJECT_SOURCE_DIR}/scheme)
include_directories(${PROJECT_SOURCE_DIR}/scheme/include)

link_libraries(/usr/local/lib/libthrift-0.13.0.so)

add_executable(master_server MasterServer.cpp)
target_link_libraries(master_server common)
target_link_libraries(master_server scheme)

add_executable(data_server DataServer.cpp)
target_link_libraries(data_server common)
target_link_libraries(data_server scheme)

add_executable(parity_server ParityServer.cpp)
target_link_libraries(parity_server common)
target_link_libraries(parity_server scheme)

add_executable(client Client.cpp)
target_link_libraries(client common)
target_link_libraries(client scheme)
