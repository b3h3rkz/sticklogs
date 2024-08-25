cmake_minimum_required(VERSION 3.10)
project(stickylogs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the path to yaml-cpp
set(YAML_CPP_DIR "/lib/x86_64-linux-gnu")

# Find yaml-cpp
find_library(YAML_CPP_LIBRARIES
    NAMES yaml-cpp
    PATHS ${YAML_CPP_DIR}
    NO_DEFAULT_PATH
)

if(NOT YAML_CPP_LIBRARIES)
    message(FATAL_ERROR "yaml-cpp library not found")
endif()

# Set the include directory for yaml-cpp
set(YAML_CPP_INCLUDE_DIR "/usr/include/yaml-cpp")

find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(RocksDB REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(stickylogs
    src/main.cpp
    src/db_wrapper.cpp
    src/config_reader.cpp
    src/log.cpp
)

target_include_directories(stickylogs PRIVATE 
    ${YAML_CPP_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(stickylogs
    PRIVATE
    ${YAML_CPP_LIBRARIES}
    Boost::system
    Boost::filesystem
    RocksDB::rocksdb
    nlohmann_json::nlohmann_json
)