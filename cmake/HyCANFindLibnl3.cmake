find_package(PkgConfig REQUIRED)

pkg_check_modules(PC_LIBNL_CORE REQUIRED libnl-3.0)
pkg_check_modules(PC_LIBNL_ROUTE REQUIRED libnl-route-3.0)

if (PC_LIBNL_CORE_FOUND AND PC_LIBNL_ROUTE_FOUND)
    message(STATUS "Found libnl-3.0 and libnl-route-3.0 via pkg-config.")

    set(_libnl3_include_dirs "")
    if (PC_LIBNL_CORE_INCLUDE_DIRS)
        list(APPEND _libnl3_include_dirs ${PC_LIBNL_CORE_INCLUDE_DIRS})
    endif ()
    if (PC_LIBNL_ROUTE_INCLUDE_DIRS) # 通常 libnl-route 不会添加新的 include 目录
        list(APPEND _libnl3_include_dirs ${PC_LIBNL_ROUTE_INCLUDE_DIRS})
    endif ()
    if (_libnl3_include_dirs)
        list(REMOVE_DUPLICATES _libnl3_include_dirs)
    endif ()
    set(LIBNL3_INCLUDE_DIR ${_libnl3_include_dirs} CACHE INTERNAL "libnl3 include directories from pkg-config")

    set(_libnl3_libraries "")
    if (PC_LIBNL_CORE_LIBRARIES)
        list(APPEND _libnl3_libraries ${PC_LIBNL_CORE_LIBRARIES})
    endif ()
    if (PC_LIBNL_ROUTE_LIBRARIES)
        list(APPEND _libnl3_libraries ${PC_LIBNL_ROUTE_LIBRARIES})
    endif ()
    if (_libnl3_libraries)
        list(REMOVE_DUPLICATES _libnl3_libraries)
    endif ()
    set(LIBNL3_LIBRARIES ${_libnl3_libraries} CACHE INTERNAL "libnl3 libraries from pkg-config")

    set(LIBNL3_FOUND TRUE CACHE BOOL "libnl3 found via pkg-config")
    mark_as_advanced(LIBNL3_INCLUDE_DIR LIBNL3_LIBRARIES LIBNL3_FOUND)

    message(STATUS "Using libnl3 include dirs from pkg-config: ${LIBNL3_INCLUDE_DIR}")
    message(STATUS "Using libnl3 libraries from pkg-config: ${LIBNL3_LIBRARIES}")
else ()
    message(FATAL_ERROR "libnl-3.0 (PC_LIBNL_CORE_FOUND: ${PC_LIBNL_CORE_FOUND}) and/or libnl-route-3.0 (PC_LIBNL_ROUTE_FOUND: ${PC_LIBNL_ROUTE_FOUND}) not found via pkg-config. Please install libnl3 development packages (e.g., libnl-3-dev, libnl-route-3-dev on Debian/Ubuntu, or libnl3-devel on Fedora/CentOS).")
endif ()

if (NOT LIBNL3_FOUND)
    message(FATAL_ERROR "LIBNL3_FOUND is false after pkg-config check. This should not happen if pkg_check_modules used REQUIRED.")
endif ()