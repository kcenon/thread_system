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

# Priority thread pool sample
# Demonstrates priority-based task scheduling in thread pools
add_subdirectory(priority_thread_pool_sample)

# Advanced priority thread pool sample
# Shows more complex usage patterns of priority thread pools
add_subdirectory(priority_thread_pool_sample_2)