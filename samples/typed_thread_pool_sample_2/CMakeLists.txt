project(typed_thread_pool_sample_2)

file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE typed_thread_pool "-framework CoreFoundation")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE typed_thread_pool)
endif()