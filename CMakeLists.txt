cmake_minimum_required(VERSION 3.10)
project(stickylogs)

set(CMAKE_CXX_STANDARD 17)

find_package(Boost REQUIRED COMPONENTS system)
find_package(RocksDB REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(stickylogs
    src/main.cpp
    src/log.cpp
    src/db_wrapper.cpp
)

target_include_directories(stickylogs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(stickylogs
    PRIVATE
    Boost::system
    RocksDB::rocksdb
    nlohmann_json::nlohmann_json
)