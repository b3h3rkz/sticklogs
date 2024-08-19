cmake_minimum_required(VERSION 3.10)
project(stickylogs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set RocksDB paths
set(ROCKSDB_ROOT "/usr/local/Cellar/rocksdb/9.4.0")
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${ROCKSDB_ROOT}")
include_directories("${ROCKSDB_ROOT}/include")
link_directories("${ROCKSDB_ROOT}/lib")

# Find RocksDB package
find_package(RocksDB REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(nlohmann_json 3.2.0 REQUIRED)

# Define common source files
set(COMMON_SOURCES
    src/transaction.cpp
    src/db_wrapper.cpp
)

# Add main executable
add_executable(${PROJECT_NAME} 
    src/main.cpp
    ${COMMON_SOURCES}
)

# Include directories for main executable
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link the target libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    RocksDB::rocksdb 
    Boost::system 
    nlohmann_json::nlohmann_json
    pthread
)
# Add compile options for main executable
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic)

# Benchmarking Section
add_executable(${PROJECT_NAME}_benchmark
    src/benchmark.cpp
    src/benchmark_main.cpp
    ${COMMON_SOURCES}
)

# Include directories for benchmark
target_include_directories(${PROJECT_NAME}_benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link RocksDB for benchmark
target_link_libraries(${PROJECT_NAME}_benchmark PRIVATE RocksDB::rocksdb)

# Add compile options for benchmark
target_compile_options(${PROJECT_NAME}_benchmark PRIVATE -Wall -Wextra -pedantic)