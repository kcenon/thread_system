cmake_minimum_required(VERSION 3.14)
project(mpmc_queue_sample)

# Create executable
add_executable(mpmc_queue_sample mpmc_queue_sample.cpp)

# Link required libraries
target_link_libraries(mpmc_queue_sample
    thread_base
    thread_pool
    logger
    formatter
)

# Set compile features
target_compile_features(mpmc_queue_sample PRIVATE cxx_std_20)

# Install the executable
install(TARGETS mpmc_queue_sample DESTINATION bin)