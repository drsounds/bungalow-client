cmake_minimum_required(VERSION 3.16)

if(CMAKE_HOST_WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "vcpkg target triplet")
endif()

# Must be decided before project(), which runs the vcpkg manifest install.
option(BUNGALOW_ENABLE_CEF "Run local HTML5 app bundles in Chromium Embedded Framework (wtl backend only)" OFF)
if(BUNGALOW_ENABLE_CEF)
    list(APPEND VCPKG_MANIFEST_FEATURES "cef")
endif()

project(BungalowNative LANGUAGES C CXX)
include(CTest)

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

find_package(nlohmann_json CONFIG REQUIRED)
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
file(GLOB BUNGALOW_SOURCES CONFIGURE_DEPENDS "bungalow/src/*.cpp")
file(GLOB SPACE_HEADERS CONFIGURE_DEPENDS "space/include/*.h")
file(GLOB BUNGALOW_HEADERS CONFIGURE_DEPENDS "bungalow/include/*.h")
if(BUNGALOW_UI_BACKEND STREQUAL "wxwidgets")
    list(FILTER BUNGALOW_SOURCES EXCLUDE REGEX "/main\\.cpp$")
else()
    set(BUNGALOW_SOURCES)
    set(BUNGALOW_HEADERS)
    if(BUNGALOW_UI_BACKEND STREQUAL "wtl")
        find_package(SQLite3 REQUIRED)
        file(GLOB_RECURSE BUNGALOW_SOURCES CONFIGURE_DEPENDS "bungalow/src/*.cpp")
        file(GLOB_RECURSE BUNGALOW_HEADERS CONFIGURE_DEPENDS "bungalow/include/*.h")
        list(FILTER BUNGALOW_SOURCES EXCLUDE REGEX "/main\\.cpp$")
    endif()
endif()

if(BUNGALOW_UI_BACKEND STREQUAL "wxwidgets")
    file(GLOB_RECURSE BACKEND_SOURCES CONFIGURE_DEPENDS
        "wxwidgets/src/*.cpp"
        "wxwidgets/app/src/*.cpp")
    file(GLOB_RECURSE BACKEND_HEADERS CONFIGURE_DEPENDS
        "wxwidgets/include/*.h"
        "wxwidgets/app/include/*.h")
    set(PLATFORM_LIBRARIES ${wxWidgets_LIBRARIES})
else()
    file(GLOB BACKEND_HEADERS CONFIGURE_DEPENDS "windows/include/*.h")
    set(BACKEND_SOURCES
        windows/src/NativeApplication.cpp
        windows/src/Win32GraphicsContext.cpp)
    if(BUNGALOW_UI_BACKEND STREQUAL "wtl")
        find_path(WTL_INCLUDE_DIR atlapp.h DOC "Directory containing the WTL headers")
        if(NOT WTL_INCLUDE_DIR)
            message(FATAL_ERROR "WTL was not found. Set WTL_INCLUDE_DIR to its include directory.")
        endif()
        list(APPEND BACKEND_SOURCES
            windows/src/wtl_main.cpp
            windows/src/BungalowApp.cpp
            windows/src/SiteView.cpp
            windows/src/FaviconCache.cpp
            windows/src/DownloadManager.cpp
            windows/src/PlatformUi.cpp
            windows/src/WinAudio.cpp
            windows/src/WinHttpClient.cpp
            windows/src/SPPlayerCanvasElement.cpp
            windows/src/SPMusicVisualizerElement.cpp
            windows/src/UiDispatcher.cpp
            wxwidgets/app/src/MainElement.cpp
            wxwidgets/app/src/AppView.cpp
            wxwidgets/app/src/LibraryView.cpp
            wxwidgets/app/src/LocalFilesView.cpp
            wxwidgets/app/src/PlayQueueView.cpp
            wxwidgets/app/src/SPAppView.cpp)
        file(GLOB WTL_SHARED_HEADERS CONFIGURE_DEPENDS "wxwidgets/app/include/*.h")
        list(APPEND BACKEND_HEADERS ${WTL_SHARED_HEADERS})
    else()
        list(APPEND BACKEND_SOURCES windows/src/main.cpp)
    endif()
    set(PLATFORM_LIBRARIES winhttp msimg32 comctl32 gdi32 gdiplus user32 ole32 shell32 comdlg32)
    if(BUNGALOW_UI_BACKEND STREQUAL "wtl")
        list(APPEND PLATFORM_LIBRARIES winmm shell32 ole32 SQLite3::SQLite3)
    endif()
endif()

if(BUNGALOW_ENABLE_CEF)
    if(NOT BUNGALOW_UI_BACKEND STREQUAL "wtl")
        message(FATAL_ERROR "BUNGALOW_ENABLE_CEF currently requires BUNGALOW_UI_BACKEND=wtl")
    endif()
    find_package(cef CONFIG REQUIRED)

    # CEF headers need C++20, so the CEF host is its own library and the
    # client only sees the CEF-free SPEmbeddedBrowser / CefRuntime headers.
    file(GLOB CEF_HOST_SOURCES CONFIGURE_DEPENDS "windows/src/cefhost/*.cpp" "windows/src/cefhost/*.h")
    file(GLOB CEF_HOST_HEADERS CONFIGURE_DEPENDS "windows/include/cefhost/*.h")
    add_library(bungalow-cef STATIC ${CEF_HOST_SOURCES} ${CEF_HOST_HEADERS} space/include/SPEmbeddedBrowser.h)
    set_target_properties(bungalow-cef PROPERTIES CXX_STANDARD 20)
    target_include_directories(bungalow-cef PRIVATE
        space/include
        windows/include
        wxwidgets/app/include)
    target_compile_definitions(bungalow-cef PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00 UNICODE _UNICODE)
    target_link_libraries(bungalow-cef PUBLIC cef::cef PRIVATE nlohmann_json::nlohmann_json SQLite3::SQLite3)
endif()

if(BUNGALOW_UI_BACKEND STREQUAL "wxwidgets" AND wxWidgets_USE_FILE)
    include(${wxWidgets_USE_FILE})
endif()

add_executable(bungalow-client
    ${SPACE_SOURCES}
    ${BUNGALOW_SOURCES}
    ${BACKEND_SOURCES}
    ${SPACE_HEADERS}
    ${BUNGALOW_HEADERS}
    ${BACKEND_HEADERS}
    space/src/SPPageElement.h)

target_include_directories(bungalow-client PRIVATE
    ${LUA_INCLUDE_DIR}
    ${PNG_INCLUDE_DIRS}
    ${EXPAT_INCLUDE_DIRS}
    space/include
    bungalow/include)

if(BUNGALOW_UI_BACKEND STREQUAL "wxwidgets")
    target_include_directories(bungalow-client PRIVATE
        wxwidgets/include
        wxwidgets/app/include
        ${wxWidgets_INCLUDE_DIRS})
    target_compile_definitions(bungalow-client PRIVATE BUNGALOW_ENABLE_PLAYER_CANVAS)
else()
    target_include_directories(bungalow-client PRIVATE windows/include)
    if(BUNGALOW_UI_BACKEND STREQUAL "wtl")
        target_include_directories(bungalow-client PRIVATE "${WTL_INCLUDE_DIR}" wxwidgets/app/include)
        target_compile_definitions(bungalow-client PRIVATE BUNGALOW_WTL)
    endif()
    target_compile_definitions(bungalow-client PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
endif()

target_link_libraries(bungalow-client PRIVATE
    ${PLATFORM_LIBRARIES}
    nlohmann_json::nlohmann_json
    PNG::PNG
    EXPAT::EXPAT
    ${LUA_LIBRARIES}
    elua)

if(WIN32)
    set_target_properties(bungalow-client PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if(BUNGALOW_ENABLE_CEF)
    target_compile_definitions(bungalow-client PRIVATE BUNGALOW_CEF)
    target_link_libraries(bungalow-client PRIVATE bungalow-cef)
    # Chromium needs the Windows 10 compatibility manifest and, since this exe
    # also runs CEF's renderer processes, an 8 MiB main-thread stack.
    target_sources(bungalow-client PRIVATE windows/bungalow-client.manifest)
    target_link_options(bungalow-client PRIVATE /STACK:0x800000)
    cef_copy_runtime(bungalow-client)
    cef_install_runtime(.)
endif()

install(TARGETS bungalow-client RUNTIME DESTINATION .)

if(BUILD_TESTING)
    add_executable(bungalow-template-test
        space/tests/SPTemplateTest.cpp
        space/src/SPTemplate.cpp)
    target_include_directories(bungalow-template-test PRIVATE
        ${LUA_INCLUDE_DIR}
        space/include
        elua/include)
    target_link_libraries(bungalow-template-test PRIVATE nlohmann_json::nlohmann_json elua)
    add_test(NAME bungalow-template-test COMMAND bungalow-template-test)

    add_executable(bungalow-native-app-test
        space/tests/SPNativeAppTest.cpp
        space/src/SPNativeApp.cpp
        space/src/SPTemplate.cpp)
    target_include_directories(bungalow-native-app-test PRIVATE
        ${LUA_INCLUDE_DIR}
        space/include
        elua/include)
    target_link_libraries(bungalow-native-app-test PRIVATE nlohmann_json::nlohmann_json elua)
    add_test(NAME bungalow-native-app-test COMMAND bungalow-native-app-test)

    add_executable(bungalow-uri-store-test
        space/tests/SPUriStoreTest.cpp
        space/src/SPUriStore.cpp)
    target_include_directories(bungalow-uri-store-test PRIVATE space/include)
    target_link_libraries(bungalow-uri-store-test PRIVATE nlohmann_json::nlohmann_json)
    add_test(NAME bungalow-uri-store-test COMMAND bungalow-uri-store-test)

    add_executable(bungalow-string-test space/tests/SPStringTest.cpp)
    target_include_directories(bungalow-string-test PRIVATE space/include)
    add_test(NAME bungalow-string-test COMMAND bungalow-string-test)

    add_executable(bungalow-xml-parser-test
        space/tests/SPXmlParserTest.cpp
        space/src/SPXmlParser.cpp)
    target_include_directories(bungalow-xml-parser-test PRIVATE space/include)
    target_link_libraries(bungalow-xml-parser-test PRIVATE EXPAT::EXPAT)
    add_test(NAME bungalow-xml-parser-test COMMAND bungalow-xml-parser-test)
endif()
