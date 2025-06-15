# Monitoring Sample CMakeLists.txt

# Create the monitoring sample executable
add_executable(monitoring_sample
    monitoring_sample.cpp
)

# Create a simple monitoring sample
add_executable(simple_monitoring_sample
    simple_monitoring_sample.cpp
)

# Set target properties for both executables
set_target_properties(monitoring_sample simple_monitoring_sample PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Link required libraries for both executables
target_link_libraries(monitoring_sample PRIVATE
    utilities
    thread_base
    thread_pool
    logger
    monitoring
    ${CMAKE_THREAD_LIBS_INIT}
)

target_link_libraries(simple_monitoring_sample PRIVATE
    utilities
    thread_base
    logger
    monitoring
    ${CMAKE_THREAD_LIBS_INIT}
)

# Include directories for both executables
target_include_directories(monitoring_sample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sources
)

target_include_directories(simple_monitoring_sample PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../sources
)

# Platform-specific linking for both executables
if(WIN32)
    target_link_libraries(monitoring_sample PRIVATE psapi)
    target_link_libraries(simple_monitoring_sample PRIVATE psapi)
elseif(UNIX)
    target_link_libraries(monitoring_sample PRIVATE pthread)
    target_link_libraries(simple_monitoring_sample PRIVATE pthread)
endif()

# Compiler-specific options for both executables
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(monitoring_sample PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(simple_monitoring_sample PRIVATE -Wall -Wextra -Wpedantic)
elseif(MSVC)
    target_compile_options(monitoring_sample PRIVATE /W4)
    target_compile_options(simple_monitoring_sample PRIVATE /W4)
endif()

# Set output directory for both executables
set_target_properties(monitoring_sample simple_monitoring_sample PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "✅ Monitoring sample configured")
message(STATUS "   Target: monitoring_sample")
message(STATUS "   Demonstrates: Real-time metrics collection")
message(STATUS "   Features: Thread pool monitoring, system metrics, live dashboard")