# V8 from rusty_v8 v152.2.0 (V8 15.2.124.1), the version the swimming Rust
# crates use through the v8 crate.
#
# V8 cannot practically be built here (it needs a Chromium toolchain
# checkout), so this port installs rusty_v8's prebuilt static library. It
# bundles Chromium's libc++ (in namespace std::__Cr) and is compiled with /Zl,
# so it links with either CRT; callers use its extern "C" binding layer
# (v8__* functions) rather than V8's C++ headers, whose std types would not
# match MSVC's. The library also expects symbols from the v8 crate's Rust
# code and the temporal_rs crate, which callers define (see swimming's
# V8RustStubs.cpp).

set(RUSTY_V8_TAG "v${VERSION}")
set(RUSTY_V8_V8_COMMIT "c4ca1eccb90c5464d826b7713dc27178a53b0cfe")

vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/denoland/rusty_v8/releases/download/${RUSTY_V8_TAG}/rusty_v8_release_x86_64-pc-windows-msvc.lib.gz"
    FILENAME "rusty_v8_release_x86_64-pc-windows-msvc-${VERSION}.lib.gz"
    SHA512 6850dcd8f5b7cfe4908d2c40918927b7bf977cab5141285894f10a57a931ce0115d0e0eff2bc2bc94dd0209a92efe86fa539949b2e80f47f30f9f36eb887afe2
)
vcpkg_download_distfile(RUSTY_V8_LICENSE
    URLS "https://raw.githubusercontent.com/denoland/rusty_v8/${RUSTY_V8_TAG}/LICENSE"
    FILENAME "rusty_v8-${VERSION}-LICENSE"
    SHA512 5debf8c98469501187341bd878e7ae49c34ad9e601a3be1d6ffa06a56d2adf8055ba04cc62f3d91d0965e38dc3883ac9d4c92b0f832e1d9b1288dea674960968
)
vcpkg_download_distfile(V8_LICENSE
    URLS "https://raw.githubusercontent.com/denoland/v8/${RUSTY_V8_V8_COMMIT}/LICENSE"
    FILENAME "v8-${RUSTY_V8_V8_COMMIT}-LICENSE"
    SHA512 157e52b212a706e8f639d08197185b5383a0c50cdfc1dab0c4985c83363db352545d81ff2bdd4bcbb0414924c54c967bbec46f88cbbc0b5b69169d785a514f9c
)

# The library is a single gzip-compressed file, which 7-Zip unpacks.
vcpkg_find_acquire_program(7Z)
set(UNPACKED "${CURRENT_BUILDTREES_DIR}/unpacked")
file(REMOVE_RECURSE "${UNPACKED}")
file(MAKE_DIRECTORY "${UNPACKED}")
vcpkg_execute_required_process(
    COMMAND "${7Z}" e "${ARCHIVE}" "-o${UNPACKED}" -y
    WORKING_DIRECTORY "${UNPACKED}"
    LOGNAME "unpack-${TARGET_TRIPLET}"
)
file(GLOB unpacked_lib "${UNPACKED}/*")
list(GET unpacked_lib 0 unpacked_lib)

# Only a release build is published; the C binding boundary makes it safe to
# link into Debug builds too.
file(INSTALL "${unpacked_lib}" DESTINATION "${CURRENT_PACKAGES_DIR}/lib" RENAME "rusty_v8.lib")
if(NOT VCPKG_BUILD_TYPE)
    file(INSTALL "${unpacked_lib}" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib" RENAME "rusty_v8.lib")
endif()

if(VCPKG_CRT_LINKAGE STREQUAL "dynamic")
    set(RUSTY_V8_LINK_OPTIONS "/NODEFAULTLIB:libcmt.lib")
endif()
configure_file("${CMAKE_CURRENT_LIST_DIR}/rusty-v8Config.cmake.in" "${CURRENT_PACKAGES_DIR}/share/${PORT}/rusty-v8Config.cmake" @ONLY)
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${RUSTY_V8_LICENSE}" "${V8_LICENSE}")

# A prebuilt static library, built with /Zl for either CRT (the config
# drops the static CRT a few of its objects still name).
set(VCPKG_POLICY_SKIP_CRT_LINKAGE_CHECK enabled)
# Callers declare the v8__* bindings they use; V8's headers are not installed.
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
set(VCPKG_POLICY_SKIP_MISPLACED_CMAKE_FILES_CHECK enabled)
set(VCPKG_POLICY_SKIP_LIB_CMAKE_MERGE_CHECK enabled)
