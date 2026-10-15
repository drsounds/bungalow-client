cmake_minimum_required(VERSION 3.10)
project(libspace)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Find dependencies
find_package(Boost REQUIRED)
find_package(Lua REQUIRED)
find_package(wxWidgets REQUIRED COMPONENTS xml base core)
include(${wxWidgets_USE_FILE})

# SQLite3
find_package(SQLite3 REQUIRED)


file(GLOB SOURCES "src/*.cpp")
file(GLOB HEADERS "include/*.h")

add_library(libspace ${SOURCES} ${HEADERS})

target_link_libraries(libspace ${Boost_LIBRARIES} ${LUA_LIBRARIES} ${wxWidgets_LIBRARIES})