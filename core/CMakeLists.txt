##################################################
# Core Component (thread_base + jobs + sync)
##################################################

project(thread_base)

# Explicit header file list (do not use GLOB - CMake won't detect new files automatically)
set(CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/callback_job.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/cancellable_job.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/cancellation_token.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/configuration_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/error_handling.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/event_bus.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/future_extensions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/hazard_pointer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/job.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/job_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/job_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/pool_traits.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/service_registry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/sync_primitives.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_base.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_conditions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_impl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_logger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_pool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_worker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/typed_thread_pool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/typed_thread_worker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/worker_policy.h
    # Queue module
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/queue/adaptive_job_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/queue/queue_factory.h
    # Diagnostics module
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/job_info.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/thread_info.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/bottleneck_report.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/health_status.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/execution_event.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/thread_pool_diagnostics.h
    # Priority aging
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/impl/typed_pool/priority_aging_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/impl/typed_pool/aging_typed_job.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/impl/typed_pool/aging_typed_job_queue.h
    # Work-stealing module
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/enhanced_steal_policy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/enhanced_work_stealing_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/numa_topology.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/numa_work_stealer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/steal_backoff_strategy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/work_affinity_tracker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/work_stealing_stats.h
    # Metrics module
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/metrics/enhanced_metrics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/metrics/latency_histogram.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/metrics/sliding_window_counter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/metrics/thread_pool_metrics.h
    # Resilience module
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/resilience/circuit_breaker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/resilience/circuit_breaker_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/resilience/failure_window.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/resilience/protected_job.h
    # Scaling module
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/scaling/autoscaler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/scaling/autoscaling_policy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/scaling/scaling_metrics.h
)

# Explicit source file list (do not use GLOB - CMake won't detect new files automatically)
set(CORE_SOURCES
    # Core implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/callback_job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/hazard_pointer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/job_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/thread_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/thread_logger_init.cpp
    # Lock-free implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/lockfree/lockfree_job_queue.cpp
    # Queue module implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/queue/adaptive_job_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/queue/queue_factory.cpp
    # Thread pool implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/thread_pool/thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/thread_pool/thread_worker.cpp
    # Typed pool implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/adaptive_typed_job_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/callback_typed_job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/typed_job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/typed_job_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/typed_thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/typed_thread_worker.cpp
    # Priority aging implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/aging_typed_job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/aging_typed_job_queue.cpp
    # Diagnostics implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/diagnostics/thread_pool_diagnostics.cpp
    # Work-stealing implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/stealing/numa_topology.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/stealing/numa_work_stealer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/stealing/work_affinity_tracker.cpp
    # Metrics implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/metrics/enhanced_metrics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/metrics/latency_histogram.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/metrics/sliding_window_counter.cpp
    # Resilience implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/resilience/circuit_breaker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/resilience/failure_window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/resilience/protected_job.cpp
    # Scaling implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/scaling/autoscaler.cpp
)

add_library(${PROJECT_NAME} STATIC
    ${CORE_HEADERS}
    ${CORE_SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../interfaces>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
    $<INSTALL_INTERFACE:include>
)

# Add common_system support if available
if(BUILD_WITH_COMMON_SYSTEM)
    # Search for common_system using environment variable or relative paths
    find_path(COMMON_SYSTEM_INCLUDE_DIR
        NAMES kcenon/common/patterns/result.h
        HINTS
            $ENV{COMMON_SYSTEM_ROOT}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../common_system/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../common_system/include
        DOC "Path to common_system include directory"
    )

    if(COMMON_SYSTEM_INCLUDE_DIR)
        target_include_directories(${PROJECT_NAME} PUBLIC
            $<BUILD_INTERFACE:${COMMON_SYSTEM_INCLUDE_DIR}>
        )
        target_compile_definitions(${PROJECT_NAME} PUBLIC BUILD_WITH_COMMON_SYSTEM)
        message(STATUS "ThreadSystem: common_system support enabled from ${COMMON_SYSTEM_INCLUDE_DIR}")
    else()
        message(WARNING "ThreadSystem: BUILD_WITH_COMMON_SYSTEM is ON but common_system not found")
        message(WARNING "  Set COMMON_SYSTEM_ROOT environment variable or place common_system as sibling directory")
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
    utilities
    interfaces
)

if(NOT APPLE)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
endif()

if(COMMAND set_compiler_warnings)
    set_compiler_warnings(${PROJECT_NAME})
endif()
