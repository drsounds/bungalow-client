cmake_minimum_required(VERSION 3.10)
project(LibSpacify)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Boost REQUIRED)
find_packagce(Lua REQUIRED)

# Include directories
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${LUA_INCLUDE_DIR})

include_directories(include)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

add_library(space ${SOURCES} ${HEADERS}
        src/SPMessageType.cpp
        include/SPMessageType.h)
