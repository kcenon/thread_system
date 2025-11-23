/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
All rights reserved.
*****************************************************************************/

#include <gtest/gtest.h>
#include <kcenon/thread/utils/platform_detection.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <future>

#if defined(THREAD_SYSTEM_WINDOWS)
    #include <windows.h>
#elif defined(THREAD_SYSTEM_MACOS)
    #include <mach/mach.h>
    #include <pthread.h>
    #include <sys/sysctl.h>
#elif defined(THREAD_SYSTEM_LINUX)
    #ifndef _GNU_SOURCE
    #define _GNU_SOURCE
    #endif
    #include <pthread.h>
    #include <sched.h>
    #include <unistd.h>
    #include <fstream>
#endif

namespace platform_edge_case_test {

using namespace kcenon::thread::platform;

class PlatformEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        info_ = get_system_info();
    }

    system_info info_;
};

// =============================================================================
// Common Platform Tests
// =============================================================================

TEST_F(PlatformEdgeCaseTest, SystemInfoAvailable) {
    EXPECT_NE(info_.os, os_type::unknown);
    EXPECT_NE(info_.arch, cpu_architecture::unknown);
    EXPECT_GT(info_.logical_cores, 0u);
    EXPECT_GT(info_.physical_cores, 0u);
}

TEST_F(PlatformEdgeCaseTest, ArchitectureConsistency) {
    auto arch = get_architecture();
    auto arch_name = get_arch_name();

    EXPECT_NE(arch_name, nullptr);
    EXPECT_NE(std::string(arch_name), "unknown");

    if (arch == cpu_architecture::arm64) {
        EXPECT_TRUE(is_arm64());
        EXPECT_STREQ(arch_name, "arm64");
    } else if (arch == cpu_architecture::x86_64) {
        EXPECT_FALSE(is_arm64());
        EXPECT_STREQ(arch_name, "x86_64");
    }
}

TEST_F(PlatformEdgeCaseTest, PlatformNameConsistency) {
    auto os = get_os();
    auto platform_name = get_platform_name();

    EXPECT_NE(platform_name, nullptr);

    switch (os) {
        case os_type::windows:
            EXPECT_STREQ(platform_name, "windows");
            break;
        case os_type::macos:
            EXPECT_STREQ(platform_name, "macos");
            break;
        case os_type::linux_os:
            EXPECT_STREQ(platform_name, "linux");
            break;
        default:
            break;
    }
}

TEST_F(PlatformEdgeCaseTest, ThreadHardwareConcurrency) {
    unsigned int hw_concurrency = std::thread::hardware_concurrency();
    EXPECT_GT(hw_concurrency, 0u);
    EXPECT_EQ(info_.logical_cores, hw_concurrency);
}

// =============================================================================
// macOS-Specific Tests
// =============================================================================

#if defined(THREAD_SYSTEM_MACOS)

TEST_F(PlatformEdgeCaseTest, MacOS_ThreadNaming) {
    std::atomic<bool> name_set{false};
    std::string captured_name;

    std::thread t([&name_set, &captured_name]() {
        const char* test_name = "test-thread";
        int result = pthread_setname_np(test_name);
        name_set = (result == 0);

        // Verify name was set (macOS-specific)
        char name_buf[64] = {0};
        pthread_getname_np(pthread_self(), name_buf, sizeof(name_buf));
        captured_name = name_buf;
    });

    t.join();
    EXPECT_TRUE(name_set.load());
    EXPECT_EQ(captured_name, "test-thread");
}

#if defined(THREAD_SYSTEM_ARCH_ARM64)
TEST_F(PlatformEdgeCaseTest, MacOS_AppleSilicon_EfficiencyCores) {
    EXPECT_TRUE(has_efficiency_cores());

    // Check for Apple Silicon specific features
    int has_amx = 0;
    size_t size = sizeof(has_amx);
    // Apple Silicon has AMX (Apple Matrix coprocessor)
    // This is just a detection test, not a feature test
    EXPECT_TRUE(is_arm64());
}

TEST_F(PlatformEdgeCaseTest, MacOS_AppleSilicon_CoreCount) {
    // Apple Silicon Macs have both performance and efficiency cores
    size_t perf_cores = 0;
    size_t eff_cores = 0;
    size_t size = sizeof(size_t);

    // Try to get P-core and E-core counts
    int ret1 = sysctlbyname("hw.perflevel0.physicalcpu", &perf_cores, &size, nullptr, 0);
    int ret2 = sysctlbyname("hw.perflevel1.physicalcpu", &eff_cores, &size, nullptr, 0);

    if (ret1 == 0 && ret2 == 0) {
        EXPECT_GT(perf_cores, 0u);
        // M1/M2/M3 have efficiency cores, but count varies
        std::cout << "Performance cores: " << perf_cores << ", Efficiency cores: " << eff_cores << std::endl;
    }
}
#endif // ARM64

#endif // MACOS

// =============================================================================
// Linux-Specific Tests
// =============================================================================

#if defined(THREAD_SYSTEM_LINUX)

TEST_F(PlatformEdgeCaseTest, Linux_ThreadNaming) {
    std::atomic<bool> name_set{false};

    std::thread t([&name_set]() {
        const char* test_name = "test-thread";
        int result = pthread_setname_np(pthread_self(), test_name);
        name_set = (result == 0);
    });

    t.join();
    EXPECT_TRUE(name_set.load());
}

TEST_F(PlatformEdgeCaseTest, Linux_CPUAffinityAvailable) {
    // Test that CPU affinity APIs are available
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    // Should be able to get current affinity
    int result = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    EXPECT_EQ(result, 0);

    // Count set CPUs
    int count = CPU_COUNT(&cpuset);
    EXPECT_GT(count, 0);
}

TEST_F(PlatformEdgeCaseTest, Linux_ContainerDetection) {
    // Check if we can detect container environment
    bool in_container = is_container_environment();

    // In CI, we might be in a container
    // This test just verifies the function doesn't crash
    (void)in_container;  // Suppress unused warning
    SUCCEED();
}

TEST_F(PlatformEdgeCaseTest, Linux_CgroupsAccessible) {
    // Check if cgroups info is accessible (for container CPU limits)
    std::ifstream cgroup_file("/proc/self/cgroup");

    // File should be readable on Linux
    if (cgroup_file.is_open()) {
        std::string line;
        while (std::getline(cgroup_file, line)) {
            // Just verify we can read it
            EXPECT_FALSE(line.empty() || line[0] == '\0');
            break;
        }
    }
    // Not a failure if file doesn't exist (some systems)
    SUCCEED();
}

#endif // LINUX

// =============================================================================
// Windows-Specific Tests
// =============================================================================

#if defined(THREAD_SYSTEM_WINDOWS)

TEST_F(PlatformEdgeCaseTest, Windows_ThreadNaming) {
    std::atomic<bool> name_set{false};

    std::thread t([&name_set]() {
        HRESULT hr = SetThreadDescription(GetCurrentThread(), L"test-thread");
        name_set = SUCCEEDED(hr);
    });

    t.join();
    EXPECT_TRUE(name_set.load());
}

TEST_F(PlatformEdgeCaseTest, Windows_ProcessorCount) {
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    EXPECT_GT(sysinfo.dwNumberOfProcessors, 0u);
    EXPECT_EQ(info_.logical_cores, sysinfo.dwNumberOfProcessors);
}

TEST_F(PlatformEdgeCaseTest, Windows_ProcessorGroups) {
    // Test processor group support (important for >64 core systems)
    DWORD active_groups = GetActiveProcessorGroupCount();
    EXPECT_GE(active_groups, 1u);

    for (WORD group = 0; group < active_groups; ++group) {
        DWORD processors_in_group = GetActiveProcessorCount(group);
        EXPECT_GT(processors_in_group, 0u);
    }
}

#if defined(THREAD_SYSTEM_ARCH_ARM64)
TEST_F(PlatformEdgeCaseTest, Windows_ARM64_Detection) {
    EXPECT_TRUE(is_arm64());

    // Verify we're running on Windows ARM64
    SYSTEM_INFO sysinfo;
    GetNativeSystemInfo(&sysinfo);

    // ARM64 processor architecture value
    EXPECT_EQ(sysinfo.wProcessorArchitecture, PROCESSOR_ARCHITECTURE_ARM64);
}
#endif // ARM64

#endif // WINDOWS

// =============================================================================
// Cross-Platform Thread Behavior Tests
// =============================================================================

TEST_F(PlatformEdgeCaseTest, MultipleThreadCreation) {
    const int num_threads = info_.logical_cores * 2;
    std::vector<std::thread> threads;
    std::atomic<int> counter{0};

    threads.reserve(num_threads);
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&counter]() {
            counter.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(counter.load(), num_threads);
}

TEST_F(PlatformEdgeCaseTest, ThreadYieldBehavior) {
    std::atomic<int> counter{0};
    const int iterations = 1000;

    auto worker = [&counter, iterations]() {
        for (int i = 0; i < iterations; ++i) {
            counter.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::yield();  // Platform-specific yield
        }
    };

    std::thread t1(worker);
    std::thread t2(worker);

    t1.join();
    t2.join();

    EXPECT_EQ(counter.load(), iterations * 2);
}

TEST_F(PlatformEdgeCaseTest, HighPrecisionSleep) {
    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Allow some variance due to OS scheduling
    EXPECT_GE(duration.count(), 9);   // At least 9ms
    EXPECT_LE(duration.count(), 50);  // No more than 50ms (generous for CI)
}

} // namespace platform_edge_case_test
