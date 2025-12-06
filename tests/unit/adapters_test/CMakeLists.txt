project(adapters_unit)

file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../build/vcpkg_installed")
    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../build/vcpkg_installed/arm64-osx/include)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../build/vcpkg_installed/x64-osx/include)
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../build/vcpkg_installed/x64-linux/include)
    endif()
endif()

find_package(GTest CONFIG)
if(NOT GTest_FOUND)
    find_package(GTest)
endif()
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    thread_base
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Link logger_system if available
if(BUILD_WITH_LOGGER_SYSTEM AND TARGET logger_base)
    target_link_libraries(${PROJECT_NAME} PRIVATE logger_base)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_WITH_LOGGER_SYSTEM)
endif()

# Link common_system if available
if(BUILD_WITH_COMMON_SYSTEM AND TARGET common_system)
    target_link_libraries(${PROJECT_NAME} PRIVATE common_system)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_WITH_COMMON_SYSTEM)
endif()

# Tests require C++20 for consistency
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

# Apply sanitizers if enabled
if(COMMAND apply_sanitizers)
    apply_sanitizers(${PROJECT_NAME})
endif()
