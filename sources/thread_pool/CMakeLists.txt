set(LIBRARY_NAME thread_pool)

file(GLOB THREAD_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB THREAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${LIBRARY_NAME} STATIC ${THREAD_HEADERS} ${THREAD_SOURCES})

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${LIBRARY_NAME} PUBLIC thread_base logger utilities)