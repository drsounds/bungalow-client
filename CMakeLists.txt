cmake_minimum_required(VERSION 3.16)
project(BungalowNative LANGUAGES C CXX)
include(CTest)

if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUNGALOW_UI_BACKEND "wxwidgets" CACHE STRING "UI backend: wxwidgets, win32, or wtl")
set_property(CACHE BUNGALOW_UI_BACKEND PROPERTY STRINGS wxwidgets win32 wtl)
string(TOLOWER "${BUNGALOW_UI_BACKEND}" BUNGALOW_UI_BACKEND)
if(NOT BUNGALOW_UI_BACKEND MATCHES "^(wxwidgets|win32|wtl)$")
    message(FATAL_ERROR "BUNGALOW_UI_BACKEND must be wxwidgets, win32, or wtl")
endif()
if(NOT BUNGALOW_UI_BACKEND STREQUAL "wxwidgets" AND NOT WIN32)
    message(FATAL_ERROR "The ${BUNGALOW_UI_BACKEND} backend is only supported on Windows")
endif()

find_package(Boost REQUIRED COMPONENTS json)
find_package(Lua REQUIRED)
find_package(PNG REQUIRED)
find_package(EXPAT 2.2.8 REQUIRED)

if(BUNGALOW_UI_BACKEND STREQUAL "wxwidgets")
    find_package(wxWidgets REQUIRED COMPONENTS core base xml webview)
endif()

add_library(elua STATIC
    elua/src/cstr.c
    elua/src/elua.c)
target_include_directories(elua PUBLIC
    elua/include
    ${LUA_INCLUDE_DIR})
target_link_libraries(elua PUBLIC ${LUA_LIBRARIES})

file(GLOB SPACE_SOURCES CONFIGURE_DEPENDS "space/src/*.cpp")
file(GLOB SPACIFY_SOURCES CONFIGURE_DEPENDS "spacify/src/*.cpp")
file(GLOB SPACE_HEADERS CONFIGURE_DEPENDS "space/include/*.h")
file(GLOB SPACIFY_HEADERS CONFIGURE_DEPENDS "spacify/include/*.h")
list(FILTER SPACIFY_SOURCES EXCLUDE REGEX "/main\\.cpp$")

if(BUNGALOW_UI_BACKEND STREQUAL "wxwidgets")
    file(GLOB BACKEND_SOURCES CONFIGURE_DEPENDS "wxwidgets/src/*.cpp")
    file(GLOB BACKEND_HEADERS CONFIGURE_DEPENDS "wxwidgets/include/*.h")
    set(PLATFORM_LIBRARIES ${wxWidgets_LIBRARIES})
else()
    list(FILTER SPACIFY_SOURCES EXCLUDE REGEX "/SpacifyApp\\.cpp$")
    file(GLOB BACKEND_HEADERS CONFIGURE_DEPENDS "windows/include/*.h")
    set(BACKEND_SOURCES
        windows/src/NativeApplication.cpp
        windows/src/Win32GraphicsContext.cpp)
    if(BUNGALOW_UI_BACKEND STREQUAL "wtl")
        find_path(WTL_INCLUDE_DIR atlapp.h DOC "Directory containing the WTL headers")
        if(NOT WTL_INCLUDE_DIR)
            message(FATAL_ERROR "WTL was not found. Set WTL_INCLUDE_DIR to its include directory.")
        endif()
        list(APPEND BACKEND_SOURCES windows/src/wtl_main.cpp)
    else()
        list(APPEND BACKEND_SOURCES windows/src/main.cpp)
    endif()
    set(PLATFORM_LIBRARIES winhttp msimg32 comctl32 gdi32 user32)
endif()

if(BUNGALOW_UI_BACKEND STREQUAL "wxwidgets" AND wxWidgets_USE_FILE)
    include(${wxWidgets_USE_FILE})
endif()

add_executable(spacify-client
    ${SPACE_SOURCES}
    ${SPACIFY_SOURCES}
    ${BACKEND_SOURCES}
    ${SPACE_HEADERS}
    ${SPACIFY_HEADERS}
    ${BACKEND_HEADERS})

target_include_directories(spacify-client PRIVATE
    ${Boost_INCLUDE_DIRS}
    ${LUA_INCLUDE_DIR}
    ${PNG_INCLUDE_DIRS}
    ${EXPAT_INCLUDE_DIRS}
    space/include
    spacify/include)

if(BUNGALOW_UI_BACKEND STREQUAL "wxwidgets")
    target_include_directories(spacify-client PRIVATE wxwidgets/include ${wxWidgets_INCLUDE_DIRS})
else()
    target_include_directories(spacify-client PRIVATE windows/include)
    if(BUNGALOW_UI_BACKEND STREQUAL "wtl")
        target_include_directories(spacify-client PRIVATE "${WTL_INCLUDE_DIR}")
    endif()
    target_compile_definitions(spacify-client PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
endif()

target_link_libraries(spacify-client PRIVATE
    ${PLATFORM_LIBRARIES}
    Boost::json
    PNG::PNG
    EXPAT::EXPAT
    ${LUA_LIBRARIES}
    elua)

if(WIN32)
    set_target_properties(spacify-client PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS spacify-client RUNTIME DESTINATION .)

if(BUILD_TESTING)
    add_executable(spacify-template-test
        space/tests/SPTemplateTest.cpp
        space/src/SPTemplate.cpp)
    target_include_directories(spacify-template-test PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${LUA_INCLUDE_DIR}
        space/include
        elua/include)
    target_link_libraries(spacify-template-test PRIVATE Boost::json elua)
    add_test(NAME spacify-template-test COMMAND spacify-template-test)
endif()
