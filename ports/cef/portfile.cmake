# Chromium Embedded Framework, matching the source checkout in cef/
# (chromiumembedded-cef-2384915b7b1f, Chromium 151.0.7922.174).
#
# libcef.dll cannot practically be built here (it requires a full Chromium
# build), so this port installs the official prebuilt "minimal" binary
# distribution and builds only the libcef_dll_wrapper static library from it.

set(CEF_VERSION "151.3.24+g2384915+chromium-151.0.7922.174")
string(REPLACE "+" "%2B" CEF_VERSION_URL "${CEF_VERSION}")

vcpkg_download_distfile(ARCHIVE
    URLS "https://cef-builds.spotifycdn.com/cef_binary_${CEF_VERSION_URL}_windows64_minimal.tar.bz2"
    FILENAME "cef_binary_151.3.24_g2384915_windows64_minimal.tar.bz2"
    SHA512 be90b802fa1f12bf12f24fc479105a4e15146c07b4d34a09021a743b1d52d6864565ffb6918bc6dcba013d81c059757c77e54a307cdf8a6402f429d509ec4146
)

vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}")

# CEF builds its wrapper with /WX; newer MSVC releases add warnings that would
# otherwise break the build.
vcpkg_replace_string("${SOURCE_PATH}/cmake/cef_variables.cmake"
    "/WX           # Treat warnings as errors"
    "# /WX disabled by the vcpkg port")

# The wrapper is linked into the client, so it must use the same CRT as the
# triplet (/MD for x64-windows-static-md). No sandbox: the client exe doubles
# as the CEF subprocess.
if(VCPKG_CRT_LINKAGE STREQUAL "dynamic")
    set(CEF_RUNTIME_FLAG "/MD")
else()
    set(CEF_RUNTIME_FLAG "/MT")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DUSE_SANDBOX=OFF
        -DUSE_ATL=OFF
        "-DCEF_RUNTIME_LIBRARY_FLAG=${CEF_RUNTIME_FLAG}"
        -DCEF_DEBUG_INFO_FLAG=/Z7
)
vcpkg_cmake_build(TARGET libcef_dll_wrapper)

# The CEF project has no install rules; lay the files out by hand.
file(INSTALL "${SOURCE_PATH}/include" DESTINATION "${CURRENT_PACKAGES_DIR}/include/cef")

foreach(build_type IN ITEMS rel dbg)
    if(build_type STREQUAL "rel")
        set(prefix "${CURRENT_PACKAGES_DIR}")
    else()
        set(prefix "${CURRENT_PACKAGES_DIR}/debug")
    endif()
    file(GLOB_RECURSE wrapper_lib "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-${build_type}/libcef_dll_wrapper.lib")
    if(NOT wrapper_lib)
        message(FATAL_ERROR "libcef_dll_wrapper.lib was not built for ${build_type}")
    endif()
    list(GET wrapper_lib 0 wrapper_lib)
    file(INSTALL "${wrapper_lib}" DESTINATION "${prefix}/lib")

    # Only Release binaries ship in the minimal distribution; the C API boundary
    # makes them safe to use from Debug clients as well.
    file(INSTALL "${SOURCE_PATH}/Release/libcef.lib" DESTINATION "${prefix}/lib")
    file(INSTALL
        "${SOURCE_PATH}/Release/chrome_elf.dll"
        "${SOURCE_PATH}/Release/d3dcompiler_47.dll"
        "${SOURCE_PATH}/Release/dxcompiler.dll"
        "${SOURCE_PATH}/Release/dxil.dll"
        "${SOURCE_PATH}/Release/libcef.dll"
        "${SOURCE_PATH}/Release/libEGL.dll"
        "${SOURCE_PATH}/Release/libGLESv2.dll"
        "${SOURCE_PATH}/Release/v8_context_snapshot.bin"
        "${SOURCE_PATH}/Release/vk_swiftshader.dll"
        "${SOURCE_PATH}/Release/vk_swiftshader_icd.json"
        "${SOURCE_PATH}/Release/vulkan-1.dll"
        DESTINATION "${prefix}/bin")
endforeach()

file(INSTALL "${SOURCE_PATH}/Resources/" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}/Resources")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/cefConfig.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")

# libcef.dll is a prebuilt shared library with its own statically linked CRT,
# shipped alongside the static wrapper even on static triplets.
set(VCPKG_POLICY_DLLS_IN_STATIC_LIBRARY enabled)
set(VCPKG_POLICY_SKIP_CRT_LINKAGE_CHECK enabled)
