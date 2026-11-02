cmake_minimum_required(VERSION 3.16)
project(Swimming LANGUAGES C CXX)

# swimming: runs React (TSX) apps built with vite-plugin-swimming natively,
# with no browser engine: QuickJS runs the JS, Yoga lays out the shadow tree
# React's reconciler builds, and the app is painted by the host.
#
#   swimming-core    shadow tree, inline CSS, HTML defaults and parser, Yoga layout,
#                    software painter
#   swimming-js      engine-neutral JsEngine interface and its QuickJS and V8 backends
#   swimming-bridge  __SWIMMING_NATIVE__, timers, pointer events, HTML/script loading
#   swimming-space   SPSwimmingElement, which shows an app in a Space window; an
#                    interface library whose sources build with the consumer's
#                    Space headers and flags

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(CTest)
endif()
if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

find_package(nlohmann_json CONFIG REQUIRED)
find_package(yoga CONFIG REQUIRED)
find_package(qjs CONFIG REQUIRED)
find_package(PNG REQUIRED)

add_library(swimming-core STATIC
    src/Engine.cpp
    src/Html.cpp
    src/HtmlDocument.cpp
    src/Layout.cpp
    src/Png.cpp
    src/Render.cpp
    src/Style.cpp
    include/swimming/Engine.h
    include/swimming/Html.h
    include/swimming/HtmlDocument.h
    include/swimming/Layout.h
    include/swimming/Render.h
    include/swimming/Style.h)
target_include_directories(swimming-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(swimming-core PUBLIC nlohmann_json::nlohmann_json PRIVATE yoga::yogacore PNG::PNG)

# JavaScript engines: QuickJS is always built; V8 (rusty_v8's prebuilt
# library, Windows x64) optionally. SWIMMING_DEFAULT_ENGINE picks the one
# createDefaultEngine() returns.
option(SWIMMING_ENABLE_V8 "Build the V8 engine backend (needs the rusty-v8 package)" OFF)
set(SWIMMING_DEFAULT_ENGINE "quickjs" CACHE STRING "Engine createDefaultEngine() returns: quickjs or v8")
set_property(CACHE SWIMMING_DEFAULT_ENGINE PROPERTY STRINGS quickjs v8)
if(SWIMMING_DEFAULT_ENGINE STREQUAL "v8" AND NOT SWIMMING_ENABLE_V8)
    message(FATAL_ERROR "SWIMMING_DEFAULT_ENGINE=v8 needs SWIMMING_ENABLE_V8")
endif()

add_library(swimming-js STATIC
    src/JsEngine.cpp
    src/QuickJsEngine.cpp
    src/StackLimit.cpp
    src/StackLimit.h
    include/swimming/JsEngine.h
    include/swimming/QuickJsEngine.h)
target_include_directories(swimming-js PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(swimming-js PRIVATE qjs)
if(SWIMMING_ENABLE_V8)
    find_package(rusty-v8 CONFIG REQUIRED)
    target_sources(swimming-js PRIVATE src/V8Engine.cpp src/V8RustStubs.cpp include/swimming/V8Engine.h)
    target_link_libraries(swimming-js PRIVATE rusty_v8::rusty_v8)
    target_compile_definitions(swimming-js PUBLIC SWIMMING_HAS_V8)
endif()
if(SWIMMING_DEFAULT_ENGINE STREQUAL "v8")
    target_compile_definitions(swimming-js PRIVATE SWIMMING_DEFAULT_V8)
endif()

add_library(swimming-bridge STATIC
    src/Bridge.cpp
    src/Cli.cpp
    src/Loader.cpp
    include/swimming/Bridge.h
    include/swimming/Cli.h
    include/swimming/Loader.h)
target_link_libraries(swimming-bridge PUBLIC swimming-core swimming-js)

# Executables that run swimming apps need a deep main thread stack: React
# renders recursively, and the engines stop JS short of the thread's real
# stack end (src/StackLimit.cpp), throwing a RangeError there. Windows gives
# the main thread 1 MiB unless the executable asks for more; elsewhere it is
# usually 8 MiB already.
function(swimming_host_executable target)
    if(MSVC)
        target_link_options(${target} PRIVATE /STACK:0x800000)
    endif()
endfunction()

add_library(swimming-space INTERFACE)
target_sources(swimming-space INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/space/src/SPSwimmingElement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/space/include/SPSwimmingElement.h)
target_include_directories(swimming-space INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/space/include)
target_link_libraries(swimming-space INTERFACE swimming-bridge)

if(BUILD_TESTING)
    add_executable(swimming-core-test tests/CoreTest.cpp)
    target_link_libraries(swimming-core-test PRIVATE swimming-core)
    add_test(NAME swimming-core-test COMMAND swimming-core-test)

    add_executable(swimming-bridge-test tests/BridgeTest.cpp)
    target_link_libraries(swimming-bridge-test PRIVATE swimming-bridge)
    swimming_host_executable(swimming-bridge-test)
    add_test(NAME swimming-bridge-test COMMAND swimming-bridge-test)
endif()
