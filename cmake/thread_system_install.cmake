##################################################
# thread_system_install.cmake
#
# Installation configuration for thread_system
# Handles header and library installation, config files
##################################################

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

##################################################
# Install headers
##################################################
function(install_thread_system_headers)
  # Canonical headers. The whole public API lives under
  # include/kcenon/thread/... in the current layout, so a single recursive
  # rule installs every header (core, interfaces, utils, lockfree,
  # implementation details, etc.) to <prefix>/include/kcenon/thread/...
  # *.tpp template implementation files are installed alongside the headers
  # they back (e.g. typed_thread_pool), which the previous per-component
  # rules only handled for one module.
  install(DIRECTORY include/
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          COMPONENT Development
          FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
            PATTERN "*.tpp")

  # Legacy forwarding-stub headers (EPIC #683 deprecation window). These are
  # thin shims such as core/base/include/thread_base.h that #include the
  # canonical <kcenon/thread/...> header, kept so that downstream code using
  # the old flat include names keeps compiling for one release. They are
  # installed flat into <prefix>/include so that #include "thread_base.h"
  # style usage still resolves. Each rule is guarded by EXISTS so it becomes
  # a no-op once the stubs are removed.
  #
  # NOTE: the previously referenced interfaces/ and
  # implementations/{thread_pool,typed_thread_pool,lockfree}/include/
  # directories do not exist in this tree. Those dangling install(DIRECTORY)
  # entries are removed here because their canonical headers are already
  # covered by the include/ rule above; leaving them broke the audit of the
  # install/export surface (issue #696).
  foreach(_legacy_inc
          core/base/include
          core/sync/include
          utilities/include)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_legacy_inc})
      install(DIRECTORY ${_legacy_inc}/
              DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
              COMPONENT Development
              FILES_MATCHING
                PATTERN "*.h"
                PATTERN "*.hpp")
    endif()
  endforeach()

  message(STATUS "Configured header installation")
endfunction()

##################################################
# Install libraries
##################################################
function(install_thread_system_libraries)
  if(TARGET thread_system)
    install(TARGETS thread_system
            EXPORT thread_system-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    message(STATUS "Configured library installation (thread_system)")
  endif()
endfunction()

##################################################
# Install CMake config files
##################################################
function(install_cmake_config_files)
  # Install targets with single standardized namespace
  install(EXPORT thread_system-targets
          FILE thread_system-targets.cmake
          NAMESPACE thread_system::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/thread_system)

  # Generate standardized config files
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/thread_system-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/thread_system-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/thread_system
  )

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/thread_system-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  # Install config files
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/thread_system-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/thread_system-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/thread_system
  )

  message(STATUS "Configured CMake config file installation")
endfunction()

##################################################
# Install pkg-config file
##################################################
function(install_pkgconfig_file)
  set(PKG_CONFIG_FEATURE_FLAGS "")
  if(USE_STD_FORMAT)
    set(PKG_CONFIG_FEATURE_FLAGS "${PKG_CONFIG_FEATURE_FLAGS} -DUSE_STD_FORMAT")
  endif()
  if(USE_STD_JTHREAD)
    set(PKG_CONFIG_FEATURE_FLAGS "${PKG_CONFIG_FEATURE_FLAGS} -DUSE_STD_JTHREAD")
  endif()
  if(USE_STD_CHRONO_CURRENT_ZONE)
    set(PKG_CONFIG_FEATURE_FLAGS "${PKG_CONFIG_FEATURE_FLAGS} -DUSE_STD_CHRONO_CURRENT_ZONE")
  endif()

  # Get simdutf libraries for pkg-config
  if(TARGET simdutf::simdutf)
    set(SIMDUTF_LIBRARIES "-lsimdutf")
  else()
    set(SIMDUTF_LIBRARIES "")
  endif()

  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/thread_system.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/thread_system.pc
    @ONLY
  )

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/thread_system.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )

  message(STATUS "Configured pkg-config file installation")
endfunction()

##################################################
# Install documentation files
##################################################
function(install_documentation_files)
  # Adapter headers are now installed from include/kcenon/thread/adapters/
  # via the main header installation in install_thread_system_headers()

  # Install dependency documentation
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/docs/dependency_compatibility_matrix.md)
    install(FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/docs/dependency_compatibility_matrix.md
      ${CMAKE_CURRENT_SOURCE_DIR}/docs/license_compatibility.md
      DESTINATION ${CMAKE_INSTALL_DOCDIR}/dependencies
      COMPONENT Documentation
    )
  endif()

  # Install main documentation
  set(DOC_FILES "")
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    list(APPEND DOC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
  endif()
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md)
    list(APPEND DOC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md)
  endif()
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
    list(APPEND DOC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE)
  endif()

  if(DOC_FILES)
    install(FILES ${DOC_FILES}
            DESTINATION ${CMAKE_INSTALL_DOCDIR}
            COMPONENT Documentation)
  endif()

  message(STATUS "Configured documentation installation")
endfunction()

##################################################
# Main installation setup
##################################################
function(setup_thread_system_install)
  install_thread_system_headers()
  install_thread_system_libraries()
  install_cmake_config_files()
  install_pkgconfig_file()
  install_documentation_files()

  message(STATUS "Installation configuration complete")
endfunction()
