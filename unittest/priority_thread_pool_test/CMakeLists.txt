project(priority_thread_pool_unit)

file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Direct linking for macOS
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# Add GTest include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../build/vcpkg_installed/arm64-osx/include)

# Use vcpkg provided GTest
find_package(GTest CONFIG REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    priority_thread_pool
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

enable_testing()

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

add_custom_target(${PROJECT_NAME}_test ALL
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
    DEPENDS ${PROJECT_NAME}
)