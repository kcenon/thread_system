# Monitoring Module CMakeLists.txt

# Define the monitoring library
add_library(monitoring STATIC
    core/metrics_collector.cpp
    core/metrics_collector.h
    core/monitoring_types.h
    storage/ring_buffer.h
)

# Set target properties
set_target_properties(monitoring PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(monitoring 
    PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Link dependencies
target_link_libraries(monitoring 
    PUBLIC 
        utilities
        thread_base
    PRIVATE
        ${CMAKE_THREAD_LIBS_INIT}
)

# Platform-specific libraries
if(WIN32)
    target_link_libraries(monitoring PRIVATE psapi)
elseif(UNIX AND NOT APPLE)
    # Linux specific libraries if needed
    target_link_libraries(monitoring PRIVATE pthread)
elseif(APPLE)
    # macOS specific libraries if needed
    target_link_libraries(monitoring PRIVATE pthread)
endif()

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(monitoring PRIVATE 
        -Wall 
        -Wextra 
        -Wpedantic
        -Wno-unused-parameter
    )
elseif(MSVC)
    target_compile_options(monitoring PRIVATE 
        /W4
        /wd4100  # unreferenced formal parameter
    )
endif()

# Add to the list of ThreadSystem libraries
if(NOT TARGET ThreadSystem::monitoring)
    add_library(ThreadSystem::monitoring ALIAS monitoring)
endif()

# Export target
set_target_properties(monitoring PROPERTIES EXPORT_NAME monitoring)

message(STATUS "✅ Monitoring module configured successfully")
message(STATUS "   - Real-time metrics collection")
message(STATUS "   - Lock-free ring buffer storage") 
message(STATUS "   - Cross-platform system metrics")
message(STATUS "   - Thread-safe metric registration")