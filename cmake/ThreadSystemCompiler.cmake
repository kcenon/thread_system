##################################################
# ThreadSystemCompiler.cmake
#
# Compiler-specific configuration for ThreadSystem
# Handles compiler flags, sanitizers, and static analysis
##################################################

##################################################
# Configure compiler-specific flags
##################################################
function(setup_thread_system_compiler_flags)
  # Compiler-specific configuration
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # For GCC and Clang compilers
    # Enable deprecated warnings to catch usage of deprecated APIs early
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -Wdeprecated-declarations" PARENT_SCOPE)

    # Add additional flags for macOS
    if(APPLE)
      # Suppress duplicate library warnings on macOS
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-no_warn_duplicate_libraries" PARENT_SCOPE)
    endif()
  elseif(MSVC)
    # For Microsoft Visual C++ compiler
    # Enable deprecated warnings (removed /wd4996) to catch usage of deprecated APIs early
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++latest /Zc:__cplusplus" PARENT_SCOPE)
  endif()

  message(STATUS "Compiler flags configured for ${CMAKE_CXX_COMPILER_ID}")
endfunction()

##################################################
# Sanitizer support
##################################################
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

function(apply_sanitizers target)
  if(ENABLE_ASAN)
    target_compile_options(${target} PRIVATE -fsanitize=address)
    target_link_libraries(${target} PRIVATE -fsanitize=address)
    message(STATUS "AddressSanitizer enabled for ${target}")
  endif()

  if(ENABLE_UBSAN)
    target_compile_options(${target} PRIVATE -fsanitize=undefined)
    target_link_libraries(${target} PRIVATE -fsanitize=undefined)
    message(STATUS "UndefinedBehaviorSanitizer enabled for ${target}")
  endif()

  if(ENABLE_TSAN)
    target_compile_options(${target} PRIVATE -fsanitize=thread)
    target_link_libraries(${target} PRIVATE -fsanitize=thread)
    message(STATUS "ThreadSanitizer enabled for ${target}")
  endif()
endfunction()

##################################################
# Static analysis support
##################################################
option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis" OFF)

function(setup_static_analysis)
  if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
      message(STATUS "Enabling clang-tidy: ${CLANG_TIDY_EXE}")
      set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE} PARENT_SCOPE)
    else()
      message(WARNING "clang-tidy not found; ENABLE_CLANG_TIDY ignored")
    endif()
  endif()
endfunction()

##################################################
# Output compiler information
##################################################
function(print_compiler_info)
  message(STATUS "========================================")
  message(STATUS "Compiler Configuration:")
  message(STATUS "  ID: ${CMAKE_CXX_COMPILER_ID}")
  message(STATUS "  Version: ${CMAKE_CXX_COMPILER_VERSION}")
  message(STATUS "  Path: ${CMAKE_CXX_COMPILER}")
  message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
  message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
  message(STATUS "  Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
  message(STATUS "========================================")
endfunction()
