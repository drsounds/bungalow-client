cmake_minimum_required(VERSION 3.10)
project(LibSpacify)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Lua REQUIRED)
find_package(EXPAT 2.2.8 REQUIRED)

# Include directories
include_directories(${LUA_INCLUDE_DIR})

include_directories(include)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

add_library(space ${SOURCES} ${HEADERS}
        src/SPMessageType.cpp
        include/SPMessageType.h)
target_link_libraries(space PUBLIC nlohmann_json::nlohmann_json EXPAT::EXPAT)
