set(LIBRARY_NAME logger)

file(GLOB LOGGER_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB LOGGER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${LIBRARY_NAME} STATIC ${LOGGER_HEADERS} ${LOGGER_SOURCES})

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${LIBRARY_NAME} PUBLIC thread_base)

if(NOT HAS_STD_FORMAT)
find_package(fmt CONFIG REQUIRED)
target_link_libraries(${LIBRARY_NAME} PUBLIC fmt::fmt-header-only)
endif()