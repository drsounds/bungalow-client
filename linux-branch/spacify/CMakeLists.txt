cmake_minimum_required(VERSION 3.10)
project(LibSpacify)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find dependencies
find_package(Boost REQUIRED)
find_package(Lua REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Development)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

add_library(LibSpacify ${SOURCES} ${HEADERS})

target_include_directories(LibSpacify PRIVATE ${Python3_INCLUDE_DIRS})

target_link_libraries(LibSpacify ${Boost_LIBRARIES} ${LUA_LIBRARIES} ${Python3_LIBRARIES})