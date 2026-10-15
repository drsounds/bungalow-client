cmake_minimum_required(VERSION 3.10)
project(LibSpacify)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find dependencies
find_package(Boost REQUIRED COMPONENTS json)
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
target_link_libraries(space PUBLIC Boost::headers Boost::json EXPAT::EXPAT)
