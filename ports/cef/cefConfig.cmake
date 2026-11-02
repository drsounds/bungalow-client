# Imported targets for the vcpkg cef port.
#
#   cef::cef               libcef_dll_wrapper + libcef (link this)
#   cef_copy_runtime(<tgt>) copies libcef.dll, its companions and the CEF
#                           resources next to <tgt> after each build.
#   cef_install_runtime(<dir>) installs the same files into <dir>.

get_filename_component(_CEF_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

if(NOT TARGET cef::cef)
    add_library(cef::libcef SHARED IMPORTED)
    set_target_properties(cef::libcef PROPERTIES
        IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
        IMPORTED_IMPLIB_RELEASE "${_CEF_PREFIX}/lib/libcef.lib"
        IMPORTED_LOCATION_RELEASE "${_CEF_PREFIX}/bin/libcef.dll"
        IMPORTED_IMPLIB_DEBUG "${_CEF_PREFIX}/debug/lib/libcef.lib"
        IMPORTED_LOCATION_DEBUG "${_CEF_PREFIX}/debug/bin/libcef.dll"
        MAP_IMPORTED_CONFIG_MINSIZEREL Release
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        INTERFACE_INCLUDE_DIRECTORIES "${_CEF_PREFIX}/include/cef")

    add_library(cef::libcef_dll_wrapper STATIC IMPORTED)
    set_target_properties(cef::libcef_dll_wrapper PROPERTIES
        IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
        IMPORTED_LOCATION_RELEASE "${_CEF_PREFIX}/lib/libcef_dll_wrapper.lib"
        IMPORTED_LOCATION_DEBUG "${_CEF_PREFIX}/debug/lib/libcef_dll_wrapper.lib"
        MAP_IMPORTED_CONFIG_MINSIZEREL Release
        MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
        INTERFACE_INCLUDE_DIRECTORIES "${_CEF_PREFIX}/include/cef"
        INTERFACE_LINK_LIBRARIES "cef::libcef;comctl32;crypt32;delayimp;gdi32;rpcrt4;shlwapi;wintrust;ws2_32")

    add_library(cef::cef INTERFACE IMPORTED)
    set_target_properties(cef::cef PROPERTIES
        INTERFACE_LINK_LIBRARIES cef::libcef_dll_wrapper)
endif()

set(CEF_RESOURCE_DIR "${_CEF_PREFIX}/share/cef/Resources")
set(CEF_RUNTIME_FILES
    chrome_elf.dll d3dcompiler_47.dll dxcompiler.dll dxil.dll libcef.dll
    libEGL.dll libGLESv2.dll v8_context_snapshot.bin vk_swiftshader.dll
    vk_swiftshader_icd.json vulkan-1.dll)

function(cef_copy_runtime target)
    set(_bin "${_CEF_PREFIX}/$<$<CONFIG:Debug>:debug/>bin")
    set(_files)
    foreach(_file IN LISTS CEF_RUNTIME_FILES)
        list(APPEND _files "${_bin}/${_file}")
    endforeach()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${_files} "$<TARGET_FILE_DIR:${target}>"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CEF_RESOURCE_DIR}" "$<TARGET_FILE_DIR:${target}>"
        COMMENT "Copying CEF runtime next to ${target}"
        VERBATIM)
endfunction()

function(cef_install_runtime destination)
    foreach(_file IN LISTS CEF_RUNTIME_FILES)
        install(FILES "${_CEF_PREFIX}/bin/${_file}" DESTINATION "${destination}")
    endforeach()
    install(DIRECTORY "${CEF_RESOURCE_DIR}/" DESTINATION "${destination}")
endfunction()

set(cef_FOUND TRUE)
