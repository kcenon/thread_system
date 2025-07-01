##################################################
# ThreadSystem Samples CMakeLists.txt
# 
# This script builds sample applications demonstrating the ThreadSystem library usage.
# Each sample focuses on a specific component or feature of the library.
##################################################

# Basic logger usage sample
# Demonstrates configuration and usage of the logging system
add_subdirectory(logger_sample)

# Thread pool usage sample
# Shows how to create and use thread pools for parallel task execution
add_subdirectory(thread_pool_sample)


# Typed thread pool sample
# Demonstrates type-based task scheduling in thread pools
add_subdirectory(typed_thread_pool_sample)

# Advanced typed thread pool sample
# Shows more complex usage patterns of typed thread pools
add_subdirectory(typed_thread_pool_sample_2)

# MPMC queue sample
# Demonstrates lock-free MPMC queue usage patterns
add_subdirectory(mpmc_queue_sample)

# Adaptive queue sample
# Shows adaptive queue strategy selection and usage
add_subdirectory(adaptive_queue_sample)

# Typed job queue sample
# Demonstrates typed job queue usage patterns
add_subdirectory(typed_job_queue_sample)

# Hazard pointer sample
# Shows hazard pointer usage for thread-safe memory reclamation
add_subdirectory(hazard_pointer_sample)

# Node pool sample
# Demonstrates high-performance memory pool usage
add_subdirectory(node_pool_sample)

# Real-time monitoring sample
# Shows real-time performance monitoring and metrics collection
add_subdirectory(monitoring_sample)