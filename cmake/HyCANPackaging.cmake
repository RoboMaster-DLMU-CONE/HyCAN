message(STATUS "Packaging HyCAN")
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VENDOR "C-One-Studio")
set(CPACK_PACKAGE_CONTACT "MoonFeather <moonfeather120@outlook.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_PACKAGE_NAME "hycan")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "HyCAN - Modern High-Performance Linux C++ CAN Communication Protocol Library")
configure_file(
        "${PROJECT_SOURCE_DIR}/LICENSE"
        "${PROJECT_BINARY_DIR}/LICENSE.txt"
        COPYONLY
)
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_BINARY_DIR}/LICENSE.txt")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")

set(CPACK_GENERATOR "TGZ;DEB;RPM")

# Debian settings for executable package
set(CPACK_DEBIAN_PACKAGE_NAME "libhycan")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "libs")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libnl-3-dev (>= 3.7.0), libnl-route-3-dev (>= 3.7.0), libexpected-dev (>= 1.0.0)")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_SOURCE_DIR}/cmake/packaging/debian/postinst;${PROJECT_SOURCE_DIR}/cmake/packaging/debian/prerm")

# RPM settings for executable package
set(CPACK_RPM_PACKAGE_NAME "hycan-devel")
set(CPACK_RPM_PACKAGE_LICENSE "ISC")
set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
set(CPACK_RPM_PACKAGE_REQUIRES "libnl3-devel >= 3.9.0, expected-devel >= 1.0.0")
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/cmake/packaging/rpm/postinstall.sh")
set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${PROJECT_SOURCE_DIR}/cmake/packaging/rpm/preuninstall.sh")

include(CPack)
