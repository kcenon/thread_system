project(thread_system_unittest)

add_executable(${PROJECT_NAME} convert_string_test.cpp argument_parser_test.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC utilities)

find_package(GTest CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE GTest::gtest GTest::gtest_main)

enable_testing()

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

add_custom_target(run_tests ALL
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${PROJECT_NAME}
)