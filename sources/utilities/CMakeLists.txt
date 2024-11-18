project(utilities)

file(GLOB HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB SOURCE_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADER_FILES} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(UNIX)
    if(APPLE)
        target_link_libraries(${PROJECT_NAME} PRIVATE -lc++)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE -lstdc++)
    endif()
endif()

if(NOT HAS_STD_FORMAT OR NOT SET_STD_FORMAT)
    find_package(fmt CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt-header-only)
endif()

find_package(Iconv REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Iconv::Iconv)