# Performance Regression Detection CMake Module
# Provides targets for performance testing and regression detection

# Performance baseline directory
set(PERF_BASELINE_DIR "${CMAKE_SOURCE_DIR}/baselines" CACHE PATH "Performance baseline directory")

# Create baseline directory if it doesn't exist
file(MAKE_DIRECTORY ${PERF_BASELINE_DIR})

# Function to add a performance test
function(add_perf_test TARGET_NAME SOURCE_FILE)
    add_executable(${TARGET_NAME} ${SOURCE_FILE})
    target_link_libraries(${TARGET_NAME} PRIVATE
        thread_system
        benchmark::benchmark
        benchmark::benchmark_main
    )

    # Add custom target for running with regression check
    add_custom_target(perf_check_${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E echo "Running performance test: ${TARGET_NAME}"
        COMMAND $<TARGET_FILE:${TARGET_NAME}>
            --benchmark_out=${CMAKE_BINARY_DIR}/perf_${TARGET_NAME}.json
            --benchmark_out_format=json
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/check_performance_regression.py
            --current=${CMAKE_BINARY_DIR}/perf_${TARGET_NAME}.json
            --baseline=${PERF_BASELINE_DIR}/${TARGET_NAME}.json
            --threshold=10
        DEPENDS ${TARGET_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Checking performance regression for ${TARGET_NAME}"
    )
endfunction()

# Convenience target to run all performance checks
add_custom_target(perf_check_all
    COMMENT "Running all performance regression checks"
)

# Function to update baseline
function(add_baseline_update TARGET_NAME)
    add_custom_target(update_baseline_${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_BINARY_DIR}/perf_${TARGET_NAME}.json
            ${PERF_BASELINE_DIR}/${TARGET_NAME}.json
        DEPENDS perf_check_${TARGET_NAME}
        COMMENT "Updating baseline for ${TARGET_NAME}"
    )
endfunction()
