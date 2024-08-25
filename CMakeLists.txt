cmake_minimum_required(VERSION 3.10)
project(stickylogs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the path to yaml-cpp
# set(YAML_CPP_DIR "/usr/local/Cellar/yaml-cpp/0.8.0")

# use this for ubuntu
set(YAML_CPP_DIR "/usr/include/yaml-cpp/yaml.h")

# Find yaml-cpp
set(yaml-cpp_DIR "${YAML_CPP_DIR}/lib/cmake/yaml-cpp")
find_package(yaml-cpp REQUIRED)

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
    ${YAML_CPP_DIR}/include 
    ${Boost_INCLUDE_DIRS}
)

# Explicitly add the yaml-cpp library path
link_directories(${YAML_CPP_DIR}/lib)

target_link_libraries(stickylogs
    PRIVATE
    ${YAML_CPP_DIR}/lib/libyaml-cpp.dylib
    Boost::system
    Boost::filesystem
    RocksDB::rocksdb
    nlohmann_json::nlohmann_json
)