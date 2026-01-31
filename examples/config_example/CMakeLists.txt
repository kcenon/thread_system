# Configuration example
add_executable(config_example config_example.cpp)

# Link against the appropriate library
if(TARGET ThreadSystem)
    target_link_libraries(config_example PRIVATE ThreadSystem)
elseif(TARGET thread_base)
    target_link_libraries(config_example PRIVATE thread_base)
endif()

target_compile_features(config_example PRIVATE cxx_std_20)
