# kcenon-thread-system portfile
# High-performance C++20 multithreading framework

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO kcenon/thread_system
    REF "v${VERSION}"
    SHA512 9e1bc834b3f523b55f948bc62d1496a0cfb61babe8258f6041e27b790ce8be51cbc8d495e9bdafb2d5bbc1148f05fca69542a1b97c36f030b688e8a022227d51
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_WITH_COMMON_SYSTEM=ON
        -DTHREAD_BUILD_INTEGRATION_TESTS=OFF
        -DBUILD_DOCUMENTATION=OFF
        -DTHREAD_ENABLE_LOCKFREE_QUEUE=ON
        -DTHREAD_ENABLE_WORK_STEALING=OFF
        -DFETCHCONTENT_FULLY_DISCONNECTED=ON
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME thread_system
    CONFIG_PATH lib/cmake/thread_system
)

# Remove empty directories that cause vcpkg post-build validation errors
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/include/thread_system/interfaces")

# Fix absolute paths in pkgconfig files
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
