# cmake/HyCANFindlibnl3.cmake
include(FetchContent)

set(LIBNL3_PROJECT_CACHE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.cache/libnl3_dependency" CACHE PATH "Persistent root cache directory for libnl3")
set(LIBNL3_SOURCE_DIR_CACHED "${LIBNL3_PROJECT_CACHE_ROOT}/source")
set(LIBNL3_INSTALL_DIR_CACHED "${LIBNL3_PROJECT_CACHE_ROOT}/install")

FetchContent_Declare(
        libnl3
        GIT_REPOSITORY https://github.com/thom311/libnl.git
        GIT_TAG libnl3_11_0
        SOURCE_DIR ${LIBNL3_SOURCE_DIR_CACHED}
)

FetchContent_GetProperties(libnl3)

set(LIBNL3_INSTALL_STAMP_FILE "${LIBNL3_INSTALL_DIR_CACHED}/hycan_libnl3_install_complete.stamp")

if (NOT EXISTS "${LIBNL3_INSTALL_STAMP_FILE}")
    message(STATUS "libnl3 not yet installed in persistent cache: ${LIBNL3_INSTALL_DIR_CACHED}. Building and installing.")

    # Ensure source code is available via FetchContent
    if (NOT libnl3_POPULATED)
        FetchContent_Populate(libnl3) # Downloads to LIBNL3_SOURCE_DIR_CACHED if not already there
        if (NOT libnl3_POPULATED) # Re-check after populate
            message(FATAL_ERROR "Failed to populate libnl3 sources using FetchContent into ${libnl3_SOURCE_DIR}.")
        endif ()
    else ()
        message(STATUS "libnl3 sources already available in ${libnl3_SOURCE_DIR}.")
    endif ()

    # Clean previous install directory if we are about to reinstall, to ensure freshness
    if (EXISTS "${LIBNL3_INSTALL_DIR_CACHED}")
        message(STATUS "Cleaning previous libnl3 install directory: ${LIBNL3_INSTALL_DIR_CACHED}")
        file(REMOVE_RECURSE "${LIBNL3_INSTALL_DIR_CACHED}")
    endif ()
    file(MAKE_DIRECTORY "${LIBNL3_INSTALL_DIR_CACHED}") # Ensure install directory exists

    # Build and install libnl3 (using in-source build as per previous script logic)
    # libnl3_SOURCE_DIR is set by FetchContent_GetProperties to LIBNL3_SOURCE_DIR_CACHED
    message(STATUS "Running autogen.sh for libnl3 in ${libnl3_SOURCE_DIR}...")
    execute_process(COMMAND ./autogen.sh WORKING_DIRECTORY ${libnl3_SOURCE_DIR} RESULT_VARIABLE _res OUTPUT_QUIET ERROR_QUIET)
    if (NOT _res EQUAL 0)
        message(FATAL_ERROR "libnl3 autogen.sh failed. Check logs. (Source: ${libnl3_SOURCE_DIR})")
    endif ()

    message(STATUS "Configuring libnl3 (install prefix: ${LIBNL3_INSTALL_DIR_CACHED})...")
    execute_process(COMMAND ./configure --prefix=${LIBNL3_INSTALL_DIR_CACHED} WORKING_DIRECTORY ${libnl3_SOURCE_DIR} RESULT_VARIABLE _res OUTPUT_QUIET ERROR_QUIET)
    if (NOT _res EQUAL 0)
        message(FATAL_ERROR "libnl3 configure failed. Check logs. (Source: ${libnl3_SOURCE_DIR}, Prefix: ${LIBNL3_INSTALL_DIR_CACHED})")
    endif ()

    message(STATUS "Building libnl3 (make -j)...")
    execute_process(COMMAND make -j WORKING_DIRECTORY ${libnl3_SOURCE_DIR} RESULT_VARIABLE _res OUTPUT_QUIET ERROR_QUIET)
    if (NOT _res EQUAL 0)
        message(FATAL_ERROR "libnl3 make failed. Check logs. (Source: ${libnl3_SOURCE_DIR})")
    endif ()

    message(STATUS "Installing libnl3 to ${LIBNL3_INSTALL_DIR_CACHED}...")
    execute_process(COMMAND make install WORKING_DIRECTORY ${libnl3_SOURCE_DIR} RESULT_VARIABLE _res OUTPUT_QUIET ERROR_QUIET)
    if (NOT _res EQUAL 0)
        message(FATAL_ERROR "libnl3 make install failed. Check logs. (Source: ${libnl3_SOURCE_DIR}, Prefix: ${LIBNL3_INSTALL_DIR_CACHED})")
    endif ()

    file(WRITE "${LIBNL3_INSTALL_STAMP_FILE}" "Successfully installed libnl3 by HyCANFindlibnl3.cmake")
    message(STATUS "libnl3 successfully built and installed to ${LIBNL3_INSTALL_DIR_CACHED}.")
else ()
    message(STATUS "libnl3 already installed in persistent cache: ${LIBNL3_INSTALL_DIR_CACHED} (stamp file found).")
endif ()

set(LIBNL3_INCLUDE_DIR "${LIBNL3_INSTALL_DIR_CACHED}/include/libnl3" CACHE INTERNAL "libnl3 include directory")

find_library(LIBNL_CORE_LIBRARY_RELEASE NAMES nl-3 nl-3-200
        PATHS "${LIBNL3_INSTALL_DIR_CACHED}/lib" "${LIBNL3_INSTALL_DIR_CACHED}/lib64" NO_DEFAULT_PATH)
find_library(LIBNL_CORE_LIBRARY_DEBUG NAMES nl-3d nl-3-200d
        PATHS "${LIBNL3_INSTALL_DIR_CACHED}/lib" "${LIBNL3_INSTALL_DIR_CACHED}/lib64" NO_DEFAULT_PATH)

find_library(LIBNL_ROUTE_LIBRARY_RELEASE NAMES nl-route-3 nl-route-3-200
        PATHS "${LIBNL3_INSTALL_DIR_CACHED}/lib" "${LIBNL3_INSTALL_DIR_CACHED}/lib64" NO_DEFAULT_PATH)
find_library(LIBNL_ROUTE_LIBRARY_DEBUG NAMES nl-route-3d nl-route-3-200d
        PATHS "${LIBNL3_INSTALL_DIR_CACHED}/lib" "${LIBNL3_INSTALL_DIR_CACHED}/lib64" NO_DEFAULT_PATH)

set(TEMP_LIBRARIES "")
if (CMAKE_BUILD_TYPE MATCHES "Release|MinSizeRel|RelWithDebInfo")
    if (LIBNL_CORE_LIBRARY_RELEASE)
        list(APPEND TEMP_LIBRARIES ${LIBNL_CORE_LIBRARY_RELEASE})
    else ()
        message(WARNING "libnl-3 release library not found in cache: ${LIBNL3_INSTALL_DIR_CACHED}/lib")
    endif ()
    if (LIBNL_ROUTE_LIBRARY_RELEASE)
        list(APPEND TEMP_LIBRARIES ${LIBNL_ROUTE_LIBRARY_RELEASE})
    else ()
        message(WARNING "libnl-route-3 release library not found in cache: ${LIBNL3_INSTALL_DIR_CACHED}/lib")
    endif ()
elseif (CMAKE_BUILD_TYPE MATCHES "Debug")
    if (LIBNL_CORE_LIBRARY_DEBUG)
        list(APPEND TEMP_LIBRARIES ${LIBNL_CORE_LIBRARY_DEBUG})
    elseif (LIBNL_CORE_LIBRARY_RELEASE) # Fallback to release
        list(APPEND TEMP_LIBRARIES ${LIBNL_CORE_LIBRARY_RELEASE})
    else ()
        message(WARNING "libnl-3 debug/release library not found in cache: ${LIBNL3_INSTALL_DIR_CACHED}/lib")
    endif ()

    if (LIBNL_ROUTE_LIBRARY_DEBUG)
        list(APPEND TEMP_LIBRARIES ${LIBNL_ROUTE_LIBRARY_DEBUG})
    elseif (LIBNL_ROUTE_LIBRARY_RELEASE) # Fallback to release
        list(APPEND TEMP_LIBRARIES ${LIBNL_ROUTE_LIBRARY_RELEASE})
    else ()
        message(WARNING "libnl-route-3 debug/release library not found in cache: ${LIBNL3_INSTALL_DIR_CACHED}/lib")
    endif ()
else () # Default to release if no specific build type or other match
    if (LIBNL_CORE_LIBRARY_RELEASE)
        list(APPEND TEMP_LIBRARIES ${LIBNL_CORE_LIBRARY_RELEASE})
    else ()
        message(WARNING "libnl-3 release library not found in cache: ${LIBNL3_INSTALL_DIR_CACHED}/lib (defaulting to release build type context)")
    endif ()
    if (LIBNL_ROUTE_LIBRARY_RELEASE)
        list(APPEND TEMP_LIBRARIES ${LIBNL_ROUTE_LIBRARY_RELEASE})
    else ()
        message(WARNING "libnl-route-3 release library not found in cache: ${LIBNL3_INSTALL_DIR_CACHED}/lib (defaulting to release build type context)")
    endif ()
endif ()

if (TEMP_LIBRARIES AND (EXISTS "${LIBNL3_INCLUDE_DIR}" OR (DEFINED LIBNL3_INCLUDE_DIR AND NOT "${LIBNL3_INCLUDE_DIR}" STREQUAL "")))
    set(LIBNL3_LIBRARIES ${TEMP_LIBRARIES} CACHE INTERNAL "libnl3 libraries")
    set(LIBNL3_FOUND TRUE CACHE BOOL "libnl3 found")
    message(STATUS "Using libnl3 libraries from cache: ${LIBNL3_LIBRARIES}")
    message(STATUS "Using libnl3 include dir from cache: ${LIBNL3_INCLUDE_DIR}")
else ()
    set(LIBNL3_LIBRARIES "" CACHE INTERNAL "libnl3 libraries") # Clear if not found
    set(LIBNL3_FOUND FALSE CACHE BOOL "libnl3 found")
    if (NOT TEMP_LIBRARIES)
        message(WARNING "libnl3 core or route libraries NOT found in cache: ${LIBNL3_INSTALL_DIR_CACHED}/lib")
    endif ()
    if (NOT (EXISTS "${LIBNL3_INCLUDE_DIR}" OR (DEFINED LIBNL3_INCLUDE_DIR AND NOT "${LIBNL3_INCLUDE_DIR}" STREQUAL "")))
        message(WARNING "libnl3 include directory NOT found or empty in cache: ${LIBNL3_INSTALL_DIR_CACHED}/include/libnl3")
    endif ()
    message(WARNING "libnl3 was not properly found or configured from the cache. Please check the cache directory: ${LIBNL3_PROJECT_CACHE_ROOT} or remove it to force a rebuild.")
endif ()

if (NOT TARGET libnl::nl-3)
    if (NOT LIBNL3_FOUND)
        message(WARNING "libnl3 was fetched/built, but LIBNL3_FOUND is false. Check find_library steps and paths in HyCANFindlibnl3.cmake, and the contents of ${LIBNL3_INSTALL_DIR_CACHED}.")
    endif ()
endif ()