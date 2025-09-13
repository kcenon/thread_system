project(thread_pool_unit)

file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Direct linking for macOS
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

# vcpkg toolchain automatically handles include paths when CMAKE_TOOLCHAIN_FILE is set
# No manual include directories needed for vcpkg packages

# Try to find GTest (vcpkg first, then system)
find_package(GTest CONFIG REQUIRED)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    thread_pool
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Include new headers for thread_pool and core
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../implementations/thread_pool/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/base/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/jobs/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../core/sync/include
)

enable_testing()

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

add_custom_target(${PROJECT_NAME}_test ALL
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
    DEPENDS ${PROJECT_NAME}
)

# Apply sanitizers if enabled
if(COMMAND apply_sanitizers)
    apply_sanitizers(${PROJECT_NAME})
endif()
