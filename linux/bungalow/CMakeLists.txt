cmake_minimum_required(VERSION 3.10)
project(LibBungalow)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find dependencies
find_package(Boost REQUIRED)
find_package(Lua REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Development)

file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

add_library(LibBungalow ${SOURCES} ${HEADERS})

target_include_directories(LibBungalow PRIVATE ${Python3_INCLUDE_DIRS})

target_link_libraries(LibBungalow ${Boost_LIBRARIES} ${LUA_LIBRARIES} ${Python3_LIBRARIES})