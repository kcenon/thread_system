cmake_minimum_required(VERSION 3.14)
project(adaptive_queue_sample)

# Create executable
add_executable(adaptive_queue_sample adaptive_queue_sample.cpp)

# Link required libraries
target_link_libraries(adaptive_queue_sample
    thread_base
    thread_pool
    logger
    formatter
)

# Set compile features
target_compile_features(adaptive_queue_sample PRIVATE cxx_std_20)

# Install the executable
install(TARGETS adaptive_queue_sample DESTINATION bin)