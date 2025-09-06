##################################################
# Utilities Component (new layout)
##################################################

project(utilities)

file(GLOB_RECURSE UTIL_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE UTIL_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_library(${PROJECT_NAME} STATIC ${UTIL_HEADERS} ${UTIL_SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/thread_system/utilities>
)

if(COMMAND set_compiler_warnings)
    set_compiler_warnings(${PROJECT_NAME})
endif()

# Fallback to fmt when std::format is not used
if(NOT USE_STD_FORMAT)
    find_package(fmt CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt-header-only)
endif()

# Iconv for string conversions
find_package(Iconv REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Iconv::Iconv)

