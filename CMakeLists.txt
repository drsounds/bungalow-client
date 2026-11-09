cmake_minimum_required(VERSION 3.16)

if(CMAKE_HOST_WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "vcpkg target triplet")
endif()

# Must be decided before project(), which runs the vcpkg manifest install.
option(BUNGALOW_ENABLE_CEF "Run local HTML5 app bundles in Chromium Embedded Framework (wtl backend only)" OFF)
if(BUNGALOW_ENABLE_CEF)
    list(APPEND VCPKG_MANIFEST_FEATURES "cef")
endif()
option(BUNGALOW_ENABLE_SWIMMING "Run \"AppType\": \"Swimming\" React app bundles natively with the swimming library" ON)
if(BUNGALOW_ENABLE_SWIMMING)
    list(APPEND VCPKG_MANIFEST_FEATURES "swimming")
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
    # net provides wxWebRequest, which WxWidgetsGraphicsContext::request uses.
    find_package(wxWidgets REQUIRED COMPONENTS core base xml net webview)
endif()

if(BUNGALOW_ENABLE_SWIMMING)
    add_subdirectory(swimming)
    option(BUNGALOW_BUILD_SWIMMING_VIEWER "Build the swimming viewer, which runs a swimming app in a window of the selected UI backend" ON)
endif()

add_library(elua STATIC
    elua/src/cstr.c
    elua/src/elua.c)
target_include_directories(elua PUBLIC
    elua/include
    ${LUA_INCLUDE_DIR})
target_link_libraries(elua PUBLIC ${LUA_LIBRARIES})

file(GLOB SPACE_SOURCES CONFIGURE_DEPENDS "space/src/*.cpp")
if(NOT BUNGALOW_ENABLE_SWIMMING)
    list(FILTER SPACE_SOURCES EXCLUDE REGEX "/SPSwimmingAppElement\\.cpp$")
endif()
file(GLOB BUNGALOW_SOURCES CONFIGURE_DEPENDS "bungalow/src/*.cpp")
file(GLOB SPACE_HEADERS CONFIGURE_DEPENDS "space/include/*.h")
file(GLOB BUNGALOW_HEADERS CONFIGURE_DEPENDS "bungalow/include/*.h")
# The client architecture, one directory per package: aspect, feature,
# concept, search, navigation, playlist, service, ui and web, plus the
# built-in contributions in app.
set(BUNGALOW_PACKAGES aspect app concept feature navigation playlist search service ui web)
set(BUNGALOW_PACKAGE_SOURCES)
set(BUNGALOW_PACKAGE_HEADERS)
foreach(package IN LISTS BUNGALOW_PACKAGES)
    file(GLOB_RECURSE package_sources CONFIGURE_DEPENDS "bungalow/src/${package}/*.cpp")
    file(GLOB_RECURSE package_headers CONFIGURE_DEPENDS "bungalow/include/${package}/*.h")
    list(APPEND BUNGALOW_PACKAGE_SOURCES ${package_sources})
    list(APPEND BUNGALOW_PACKAGE_HEADERS ${package_headers})
endforeach()
if(BUNGALOW_UI_BACKEND STREQUAL "wxwidgets")
    list(FILTER BUNGALOW_SOURCES EXCLUDE REGEX "/main\\.cpp$")
    list(APPEND BUNGALOW_SOURCES ${BUNGALOW_PACKAGE_SOURCES})
    list(APPEND BUNGALOW_HEADERS ${BUNGALOW_PACKAGE_HEADERS})
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
    if(NOT BUNGALOW_ENABLE_SWIMMING)
        list(FILTER BACKEND_SOURCES EXCLUDE REGEX "/SwimmingScheduler\\.cpp$")
    endif()
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
            wxwidgets/app/src/AppView.cpp
            wxwidgets/app/src/LibraryView.cpp
            wxwidgets/app/src/LocalFilesView.cpp
            wxwidgets/app/src/PlayQueueView.cpp
            wxwidgets/app/src/SPAppView.cpp)
        if(BUNGALOW_ENABLE_SWIMMING)
            list(APPEND BACKEND_SOURCES windows/src/SwimmingScheduler.cpp)
        endif()
        file(GLOB WTL_SHARED_HEADERS CONFIGURE_DEPENDS "wxwidgets/app/include/*.h")
        list(APPEND BACKEND_HEADERS ${WTL_SHARED_HEADERS})
    else()
        list(APPEND BACKEND_SOURCES windows/src/main.cpp)
    endif()
    set(PLATFORM_LIBRARIES winhttp msimg32 comctl32 gdi32 gdiplus user32 ole32 shell32 comdlg32)
    if(BUNGALOW_UI_BACKEND STREQUAL "wtl")
        list(APPEND PLATFORM_LIBRARIES winmm mfplay mfplat mf propsys shell32 ole32 SQLite3::SQLite3)
    endif()
endif()

if(BUNGALOW_ENABLE_CEF)
    if(NOT BUNGALOW_UI_BACKEND STREQUAL "wtl")
        message(FATAL_ERROR "BUNGALOW_ENABLE_CEF currently requires BUNGALOW_UI_BACKEND=wtl")
    endif()
    find_package(cef CONFIG REQUIRED)

    # CEF headers need C++20, so the CEF host is its own library and the
    # client only sees the CEF-free SPEmbeddedBrowser / CefRuntime headers.
    file(GLOB CEF_HOST_SOURCES CONFIGURE_DEPENDS "windows/src/web/cef/*.cpp" "windows/src/web/cef/*.h")
    file(GLOB CEF_HOST_HEADERS CONFIGURE_DEPENDS "windows/include/web/*.h")
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

if(BUNGALOW_ENABLE_SWIMMING)
    target_compile_definitions(bungalow-client PRIVATE BUNGALOW_SWIMMING)
    target_link_libraries(bungalow-client PRIVATE swimming-space)
endif()

install(TARGETS bungalow-client RUNTIME DESTINATION .)

# The swimming viewer: one swimming app in a Space window of the UI backend,
# or one frame rendered to a PNG with --out. Space is built without the
# client, so it gets its own copy of the Space sources.
if(BUNGALOW_ENABLE_SWIMMING AND BUNGALOW_BUILD_SWIMMING_VIEWER)
    if(BUNGALOW_UI_BACKEND STREQUAL "wxwidgets")
        add_executable(swimming-viewer
            wxwidgets/swimming/main.cpp
            wxwidgets/src/SwimmingScheduler.cpp
            wxwidgets/src/WxWidgetsGraphicsContext.cpp)
        target_include_directories(swimming-viewer PRIVATE wxwidgets/include ${wxWidgets_INCLUDE_DIRS})
        target_link_libraries(swimming-viewer PRIVATE ${wxWidgets_LIBRARIES})
    else()
        add_executable(swimming-viewer
            windows/swimming/main.cpp
            windows/src/SwimmingScheduler.cpp
            windows/src/Win32GraphicsContext.cpp)
        target_include_directories(swimming-viewer PRIVATE windows/include)
        target_compile_definitions(swimming-viewer PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0601)
        target_link_libraries(swimming-viewer PRIVATE winhttp msimg32 comctl32 gdi32 gdiplus user32 ole32 shell32 comdlg32)
    endif()
    target_sources(swimming-viewer PRIVATE ${SPACE_SOURCES} ${SPACE_HEADERS})
    target_include_directories(swimming-viewer PRIVATE
        ${LUA_INCLUDE_DIR}
        ${PNG_INCLUDE_DIRS}
        ${EXPAT_INCLUDE_DIRS}
        space/include
        bungalow/include) # for headers some Space sources include, as in the win32 backend
    target_link_libraries(swimming-viewer PRIVATE
        swimming-space
        nlohmann_json::nlohmann_json
        PNG::PNG
        EXPAT::EXPAT
        ${LUA_LIBRARIES}
        elua)
    set_target_properties(swimming-viewer PROPERTIES OUTPUT_NAME swimming)
    install(TARGETS swimming-viewer RUNTIME DESTINATION .)
endif()

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

    add_executable(bungalow-site-uri-test
        bungalow/tests/SiteUriTest.cpp
        bungalow/src/web/SiteUri.cpp)
    target_include_directories(bungalow-site-uri-test PRIVATE bungalow/include)
    add_test(NAME bungalow-site-uri-test COMMAND bungalow-site-uri-test)

    add_executable(bungalow-aspect-test
        bungalow/tests/AspectTest.cpp
        bungalow/src/search/SearchManager.cpp)
    target_include_directories(bungalow-aspect-test PRIVATE bungalow/include)
    add_test(NAME bungalow-aspect-test COMMAND bungalow-aspect-test)

    if(TARGET SQLite3::SQLite3)
        add_executable(bungalow-rootlist-test
            bungalow/tests/RootlistTest.cpp
            bungalow/src/Database.cpp
            bungalow/src/playlist/RootlistEvents.cpp
            bungalow/src/playlist/RootlistManager.cpp
            bungalow/src/playlist/Upsl.cpp)
        target_include_directories(bungalow-rootlist-test PRIVATE bungalow/include)
        target_link_libraries(bungalow-rootlist-test PRIVATE SQLite3::SQLite3)
        add_test(NAME bungalow-rootlist-test COMMAND bungalow-rootlist-test)

        add_executable(bungalow-service-test
            bungalow/tests/ServiceTest.cpp
            bungalow/src/Database.cpp
            bungalow/src/service/MediaService.cpp
            bungalow/src/service/MusicService.cpp
            bungalow/src/service/PlaybackCoordinator.cpp
            bungalow/src/service/ResolutionStore.cpp
            bungalow/src/service/ServiceManager.cpp)
        target_include_directories(bungalow-service-test PRIVATE bungalow/include)
        target_link_libraries(bungalow-service-test PRIVATE SQLite3::SQLite3)
        add_test(NAME bungalow-service-test COMMAND bungalow-service-test)
    endif()

    if(BUNGALOW_ENABLE_SWIMMING)
        add_executable(bungalow-swimming-app-test
            space/tests/SPSwimmingAppTest.cpp
            space/src/SPSwimmingApp.cpp)
        target_include_directories(bungalow-swimming-app-test PRIVATE space/include)
        target_compile_definitions(bungalow-swimming-app-test PRIVATE BUNGALOW_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
        target_link_libraries(bungalow-swimming-app-test PRIVATE swimming-bridge nlohmann_json::nlohmann_json)
        add_test(NAME bungalow-swimming-app-test COMMAND bungalow-swimming-app-test)
    endif()
endif()
