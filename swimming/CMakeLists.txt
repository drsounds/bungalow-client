cmake_minimum_required(VERSION 3.16)
project(Swimming LANGUAGES C CXX)

# swimming: runs React (TSX) apps built with vite-plugin-swimming natively,
# with no browser engine: QuickJS runs the JS, Yoga lays out the shadow tree
# React's reconciler builds, and the app is painted by the host.
#
#   swimming-core    shadow tree, inline CSS, HTML defaults and parser, Yoga layout,
#                    software painter
#   swimming-js      engine-neutral JsEngine interface and its QuickJS backend
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

add_library(swimming-js STATIC
    src/QuickJsEngine.cpp
    include/swimming/JsEngine.h
    include/swimming/QuickJsEngine.h)
target_include_directories(swimming-js PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(swimming-js PRIVATE qjs)

add_library(swimming-bridge STATIC
    src/Bridge.cpp
    src/Cli.cpp
    src/Loader.cpp
    include/swimming/Bridge.h
    include/swimming/Cli.h
    include/swimming/Loader.h)
target_link_libraries(swimming-bridge PUBLIC swimming-core swimming-js)

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
    add_test(NAME swimming-bridge-test COMMAND swimming-bridge-test)
endif()
