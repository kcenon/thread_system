##################################################
# thread_system Fuzzing CMakeLists.txt
#
# libFuzzer-based fuzz targets, gated behind the BUILD_FUZZERS option
# (default OFF). Requires a Clang toolchain with the fuzzer and address
# sanitizers:
#
#   cmake -B build-fuzz -G Ninja \
#     -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
#     -DBUILD_FUZZERS=ON -DTHREAD_BUILD_INTEGRATION_TESTS=OFF \
#     -DBUILD_DOCUMENTATION=OFF
#   cmake --build build-fuzz
#   ./build-fuzz/bin/concurrent_queue_fuzzer -max_total_time=60 fuzz/corpus
#
# The targets are excluded from the default build and from the regular CI
# matrix; they are exercised by .github/workflows/fuzzing.yml on a weekly
# schedule. The harness here is header-only (it includes the queue headers
# directly), so it does not need the full thread_system library link or vcpkg.
##################################################

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(WARNING
        "BUILD_FUZZERS is ON but the compiler is '${CMAKE_CXX_COMPILER_ID}'. "
        "libFuzzer requires Clang; skipping fuzz targets.")
    return()
endif()

set(FUZZER_FLAGS -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer)

function(add_fuzz_target target_name source_file)
    add_executable(${target_name} ${source_file})
    target_include_directories(${target_name} PRIVATE
        ${CMAKE_SOURCE_DIR}/include)
    target_compile_features(${target_name} PRIVATE cxx_std_20)
    target_compile_options(${target_name} PRIVATE ${FUZZER_FLAGS})
    target_link_options(${target_name} PRIVATE ${FUZZER_FLAGS})
endfunction()

add_fuzz_target(concurrent_queue_fuzzer concurrent_queue_fuzzer.cpp)

message(STATUS "Fuzz targets enabled: concurrent_queue_fuzzer")
