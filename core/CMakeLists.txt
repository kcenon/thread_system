##################################################
# Core Component Build Configuration
#
# This file builds the ThreadSystem library with modular architecture:
# - thread_core: Essential components (job, thread_pool, job_queue)
# - Optional modules: stealing, resilience, scaling, metrics
##################################################

project(thread_base)

# Module build options
option(THREAD_BUILD_CORE_ONLY "Build only thread-core module (minimal)" OFF)
option(THREAD_BUILD_STEALING "Build work-stealing module" ON)
option(THREAD_BUILD_RESILIENCE "Build resilience module (circuit breaker)" ON)
option(THREAD_BUILD_SCALING "Build auto-scaling module" ON)
option(THREAD_BUILD_METRICS "Build metrics module" ON)
option(THREAD_BUILD_DIAGNOSTICS "Build diagnostics module" ON)

##################################################
# Thread Core Module (Essential Components)
##################################################

set(THREAD_CORE_HEADERS
    # Job fundamentals
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/job.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/job_builder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/job_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/job_types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/retry_policy.h
    # Deprecated job classes (for backward compatibility)
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/callback_job.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/cancellable_job.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/future_job.h
    # Cancellation support
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/cancellation_token.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/cancellation_exception.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/cancellation_reason.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/enhanced_cancellation_token.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/cancellable_future.h
    # Thread pool fundamentals
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_pool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_pool_impl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_pool_builder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_worker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/worker_policy.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/pool_traits.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/submit_options.h
    # Typed pool
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/typed_thread_pool.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/typed_thread_worker.h
    # Core infrastructure
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_base.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_impl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_conditions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/configuration_manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/lifecycle_controller.h
    # Error handling
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/error_handling.h
    # Synchronization primitives
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/sync_primitives.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/hazard_pointer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/safe_hazard_pointer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/atomic_shared_ptr.h
    # Logging
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_logger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/log_level.h
    # Extensions
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/future_extensions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/event_bus.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/service_registry.h
    # Backpressure
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/backpressure_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/backpressure_job_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/token_bucket.h
    # Formatter
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/thread_pool_fmt.h
    # Queue module (essential)
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/queue/adaptive_job_queue.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/queue/queue_factory.h
    # Priority aging (essential for job queue)
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/impl/typed_pool/priority_aging_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/impl/typed_pool/aging_typed_job.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/impl/typed_pool/aging_typed_job_queue.h
)

set(THREAD_CORE_SOURCES
    # Core implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/callback_job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/hazard_pointer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/job_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/lifecycle_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/thread_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core/thread_logger_init.cpp
    # Lock-free implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/lockfree/lockfree_job_queue.cpp
    # Queue module implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/queue/adaptive_job_queue.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/queue/queue_factory.cpp
    # Thread pool implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/thread_pool/thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/thread_pool/thread_pool_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/thread_pool/thread_worker.cpp
    # Typed pool implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/callback_typed_job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/typed_job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/typed_thread_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/typed_thread_worker.cpp
    # Priority aging implementation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/aging_typed_job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/typed_pool/aging_typed_job_queue.cpp
)

# Create thread_core library
add_library(thread_core STATIC
    ${THREAD_CORE_HEADERS}
    ${THREAD_CORE_SOURCES}
)

target_include_directories(thread_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../interfaces>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(thread_core PUBLIC THREAD_CORE_MODULE)

# Add common_system support
if(BUILD_WITH_COMMON_SYSTEM)
    find_path(COMMON_SYSTEM_INCLUDE_DIR
        NAMES kcenon/common/patterns/result.h
        HINTS
            $ENV{COMMON_SYSTEM_ROOT}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../../common_system/include
            ${CMAKE_CURRENT_SOURCE_DIR}/../common_system/include
        DOC "Path to common_system include directory"
    )

    if(COMMON_SYSTEM_INCLUDE_DIR)
        target_include_directories(thread_core PUBLIC
            $<BUILD_INTERFACE:${COMMON_SYSTEM_INCLUDE_DIR}>
        )
        target_compile_definitions(thread_core PUBLIC BUILD_WITH_COMMON_SYSTEM)
        message(STATUS "thread_core: common_system support enabled from ${COMMON_SYSTEM_INCLUDE_DIR}")
    endif()
endif()

target_link_libraries(thread_core PUBLIC
    utilities
    interfaces
)

if(NOT APPLE)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(thread_core PUBLIC Threads::Threads)
endif()

if(COMMAND set_compiler_warnings)
    set_compiler_warnings(thread_core)
endif()

message(STATUS "thread_core module: ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/ (~47 headers)")

##################################################
# Optional Modules (only if not THREAD_BUILD_CORE_ONLY)
##################################################

if(NOT THREAD_BUILD_CORE_ONLY)

    # Collect optional module headers and sources
    set(OPTIONAL_HEADERS "")
    set(OPTIONAL_SOURCES "")

    ##################################################
    # NUMA Thread Pool (depends on stealing)
    ##################################################
    if(THREAD_BUILD_STEALING)
        list(APPEND OPTIONAL_HEADERS
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/core/numa_thread_pool.h
        )
        list(APPEND OPTIONAL_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/impl/thread_pool/numa_thread_pool.cpp
        )
    endif()

    ##################################################
    # Work-Stealing Module
    ##################################################
    if(THREAD_BUILD_STEALING)
        list(APPEND OPTIONAL_HEADERS
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/enhanced_steal_policy.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/enhanced_work_stealing_config.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/numa_topology.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/numa_work_stealer.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/steal_backoff_strategy.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/work_affinity_tracker.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/stealing/work_stealing_stats.h
        )
        list(APPEND OPTIONAL_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/stealing/numa_topology.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/stealing/numa_work_stealer.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/stealing/work_affinity_tracker.cpp
        )
        message(STATUS "thread_stealing module: ENABLED")
    else()
        message(STATUS "thread_stealing module: DISABLED")
    endif()

    ##################################################
    # Metrics Module
    ##################################################
    if(THREAD_BUILD_METRICS)
        list(APPEND OPTIONAL_HEADERS
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/metrics/enhanced_metrics.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/metrics/latency_histogram.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/metrics/metrics_backend.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/metrics/metrics_base.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/metrics/metrics_service.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/metrics/sliding_window_counter.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/metrics/thread_pool_metrics.h
        )
        list(APPEND OPTIONAL_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/metrics/enhanced_metrics.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/metrics/latency_histogram.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/metrics/metrics_backend.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/metrics/metrics_service.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/metrics/sliding_window_counter.cpp
        )
        message(STATUS "thread_metrics module: ENABLED")
    else()
        message(STATUS "thread_metrics module: DISABLED")
    endif()

    ##################################################
    # Resilience Module
    ##################################################
    if(THREAD_BUILD_RESILIENCE)
        list(APPEND OPTIONAL_HEADERS
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/resilience/circuit_breaker.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/resilience/circuit_breaker_config.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/resilience/failure_window.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/resilience/protected_job.h
        )
        list(APPEND OPTIONAL_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/resilience/circuit_breaker.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/resilience/failure_window.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/resilience/protected_job.cpp
        )
        message(STATUS "thread_resilience module: ENABLED")
    else()
        message(STATUS "thread_resilience module: DISABLED")
    endif()

    ##################################################
    # Scaling Module
    ##################################################
    if(THREAD_BUILD_SCALING)
        list(APPEND OPTIONAL_HEADERS
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/scaling/autoscaler.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/scaling/autoscaling_policy.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/scaling/scaling_metrics.h
        )
        list(APPEND OPTIONAL_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/scaling/autoscaler.cpp
        )
        message(STATUS "thread_scaling module: ENABLED")
    else()
        message(STATUS "thread_scaling module: DISABLED")
    endif()

    ##################################################
    # Diagnostics Module
    ##################################################
    if(THREAD_BUILD_DIAGNOSTICS)
        list(APPEND OPTIONAL_HEADERS
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/job_info.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/thread_info.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/bottleneck_report.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/health_status.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/execution_event.h
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/diagnostics/thread_pool_diagnostics.h
        )
        list(APPEND OPTIONAL_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/diagnostics/thread_pool_diagnostics.cpp
        )
        message(STATUS "thread_diagnostics module: ENABLED")
    else()
        message(STATUS "thread_diagnostics module: DISABLED")
    endif()

    ##################################################
    # Pool Policies Module (depends on other modules)
    ##################################################
    list(APPEND OPTIONAL_HEADERS
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/pool_policies/pool_policy.h
    )

    if(THREAD_BUILD_RESILIENCE)
        list(APPEND OPTIONAL_HEADERS
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/pool_policies/circuit_breaker_policy.h
        )
        list(APPEND OPTIONAL_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/pool_policies/circuit_breaker_policy.cpp
        )
    endif()

    if(THREAD_BUILD_SCALING)
        list(APPEND OPTIONAL_HEADERS
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/pool_policies/autoscaling_pool_policy.h
        )
        list(APPEND OPTIONAL_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/pool_policies/autoscaling_pool_policy.cpp
        )
    endif()

    if(THREAD_BUILD_STEALING)
        list(APPEND OPTIONAL_HEADERS
            ${CMAKE_CURRENT_SOURCE_DIR}/../include/kcenon/thread/pool_policies/work_stealing_pool_policy.h
        )
        list(APPEND OPTIONAL_SOURCES
            ${CMAKE_CURRENT_SOURCE_DIR}/../src/pool_policies/work_stealing_pool_policy.cpp
        )
    endif()

endif()

##################################################
# Main Library Target (thread_base)
##################################################

if(THREAD_BUILD_CORE_ONLY)
    # Core-only build: thread_base is an alias to thread_core
    add_library(${PROJECT_NAME} ALIAS thread_core)
    message(STATUS "thread_base: Core-only build (alias to thread_core)")
else()
    # Full build: thread_base includes core + optional modules
    add_library(${PROJECT_NAME} STATIC
        ${OPTIONAL_HEADERS}
        ${OPTIONAL_SOURCES}
    )

    target_include_directories(${PROJECT_NAME} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../interfaces>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../src>
        $<INSTALL_INTERFACE:include>
    )

    # Link thread_core as the foundation
    target_link_libraries(${PROJECT_NAME} PUBLIC thread_core)

    # Add compile definitions for enabled modules
    if(THREAD_BUILD_STEALING)
        target_compile_definitions(${PROJECT_NAME} PUBLIC THREAD_STEALING_MODULE)
    endif()
    if(THREAD_BUILD_RESILIENCE)
        target_compile_definitions(${PROJECT_NAME} PUBLIC THREAD_RESILIENCE_MODULE)
    endif()
    if(THREAD_BUILD_SCALING)
        target_compile_definitions(${PROJECT_NAME} PUBLIC THREAD_SCALING_MODULE)
    endif()
    if(THREAD_BUILD_METRICS)
        target_compile_definitions(${PROJECT_NAME} PUBLIC THREAD_METRICS_MODULE)
    endif()
    if(THREAD_BUILD_DIAGNOSTICS)
        target_compile_definitions(${PROJECT_NAME} PUBLIC THREAD_DIAGNOSTICS_MODULE)
    endif()

    if(COMMAND set_compiler_warnings)
        set_compiler_warnings(${PROJECT_NAME})
    endif()

    message(STATUS "thread_base: Full build (thread_core + optional modules)")
endif()

##################################################
# Build Summary
##################################################

message(STATUS "----------------------------------------")
message(STATUS "ThreadSystem Module Configuration:")
message(STATUS "  THREAD_BUILD_CORE_ONLY: ${THREAD_BUILD_CORE_ONLY}")
if(NOT THREAD_BUILD_CORE_ONLY)
    message(STATUS "  THREAD_BUILD_STEALING: ${THREAD_BUILD_STEALING}")
    message(STATUS "  THREAD_BUILD_RESILIENCE: ${THREAD_BUILD_RESILIENCE}")
    message(STATUS "  THREAD_BUILD_SCALING: ${THREAD_BUILD_SCALING}")
    message(STATUS "  THREAD_BUILD_METRICS: ${THREAD_BUILD_METRICS}")
    message(STATUS "  THREAD_BUILD_DIAGNOSTICS: ${THREAD_BUILD_DIAGNOSTICS}")
endif()
message(STATUS "----------------------------------------")
