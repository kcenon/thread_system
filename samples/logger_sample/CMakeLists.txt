project(logger_sample)

file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE logger "-framework CoreFoundation")
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE logger)
endif()