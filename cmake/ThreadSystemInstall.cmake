##################################################
# ThreadSystemInstall.cmake
#
# Installation configuration for ThreadSystem
# Handles header and library installation, config files
##################################################

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

##################################################
# Install headers
##################################################
function(install_thread_system_headers)
  # Install headers from new structure
  install(DIRECTORY include/
          DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          COMPONENT Development
          FILES_MATCHING PATTERN "*.h")

  # Install legacy interfaces for backward compatibility
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/interfaces)
    install(DIRECTORY interfaces/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thread_system/interfaces
            COMPONENT Development
            FILES_MATCHING PATTERN "*.h")
  endif()

  # Utility headers
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/utilities/include)
    install(DIRECTORY utilities/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thread_system/utilities
            COMPONENT Development
            FILES_MATCHING PATTERN "*.h")
  endif()

  # Implementation headers
  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/implementations/thread_pool/include)
    install(DIRECTORY implementations/thread_pool/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thread_system/implementations/thread_pool
            COMPONENT Implementation
            FILES_MATCHING PATTERN "*.h")
  endif()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/implementations/typed_thread_pool/include)
    install(DIRECTORY implementations/typed_thread_pool/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thread_system/implementations/typed_thread_pool
            COMPONENT Implementation
            FILES_MATCHING PATTERN "*.h" PATTERN "*.tpp")
  endif()

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/implementations/lockfree/include)
    install(DIRECTORY implementations/lockfree/include/
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thread_system/implementations/lockfree
            COMPONENT Implementation
            FILES_MATCHING PATTERN "*.h")
  endif()

  message(STATUS "Configured header installation")
endfunction()

##################################################
# Install libraries
##################################################
function(install_thread_system_libraries)
  if(USE_LEGACY_BUILD)
    # Legacy targets
    set(INSTALL_TARGETS "")
    if(TARGET utilities)
      list(APPEND INSTALL_TARGETS utilities)
    endif()
    if(TARGET interfaces)
      list(APPEND INSTALL_TARGETS interfaces)
    endif()
    if(TARGET thread_base)
      list(APPEND INSTALL_TARGETS thread_base)
    endif()
    if(TARGET thread_pool)
      list(APPEND INSTALL_TARGETS thread_pool)
    endif()
    if(TARGET typed_thread_pool)
      list(APPEND INSTALL_TARGETS typed_thread_pool)
    endif()
    if(TARGET lockfree)
      list(APPEND INSTALL_TARGETS lockfree)
    endif()

    if(INSTALL_TARGETS)
      install(TARGETS ${INSTALL_TARGETS}
              EXPORT ThreadSystemTargets
              ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
              LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
              RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
      message(STATUS "Configured library installation (legacy targets)")
    endif()
  else()
    # New structure - install ThreadSystem library
    if(TARGET ThreadSystem)
      install(TARGETS ThreadSystem
              EXPORT ThreadSystemTargets
              ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
              LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
              RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
      message(STATUS "Configured library installation (ThreadSystem)")
    endif()
  endif()
endfunction()

##################################################
# Install CMake config files
##################################################
function(install_cmake_config_files)
  # Install targets with standardized namespace
  install(EXPORT ThreadSystemTargets
          FILE thread_system-targets.cmake
          NAMESPACE thread_system::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/thread_system)

  # Legacy compatibility
  install(EXPORT ThreadSystemTargets
          FILE ThreadSystemTargets.cmake
          NAMESPACE ThreadSystem::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ThreadSystem)

  # Generate standardized config files
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/thread_system-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/thread_system-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/thread_system
  )

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/thread_system-config-version.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/thread_system-config-version.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/thread_system
  )

  # Legacy config files
  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ThreadSystemConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ThreadSystemConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ThreadSystem
  )

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/ThreadSystemConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  # Install config files
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/thread_system-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/thread_system-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/thread_system
  )

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/ThreadSystemConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/ThreadSystemConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ThreadSystem
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

  # Get Iconv libraries for pkg-config
  if(TARGET Iconv::Iconv)
    get_target_property(ICONV_LIBRARIES Iconv::Iconv INTERFACE_LINK_LIBRARIES)
    if(NOT ICONV_LIBRARIES)
      set(ICONV_LIBRARIES "")
    endif()
  else()
    set(ICONV_LIBRARIES "")
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
  # Install adapter headers if BUILD_WITH_COMMON_SYSTEM is enabled
  if(BUILD_WITH_COMMON_SYSTEM)
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/thread_system/adapters/common_executor_adapter.h)
      install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/include/thread_system/adapters/common_executor_adapter.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/thread_system/adapters
        COMPONENT Development
      )
    endif()
  endif()

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
