##################################################
# ThreadSystem Unit Tests CMakeLists.txt
# 
# This script configures and builds the test suite for validating 
# the ThreadSystem library functionality.
# Test are organized by component and run automatically during build if enabled.
##################################################

# Utility component tests
# Validates core utilities, data structures, and helper functions
add_subdirectory(utilities_test)

# Thread pool tests
# Validates standard thread pool operations, task queueing, and execution
add_subdirectory(thread_pool_test)

# Priority thread pool tests
# Validates priority-based scheduling, task ordering, and edge cases
add_subdirectory(priority_thread_pool_test)