# Real-time Monitoring Sample

This directory contains sample applications demonstrating the real-time monitoring capabilities of the Thread System library.

## 📊 Samples Overview

### 1. `simple_monitoring_sample`
A basic demonstration of the monitoring system that shows:
- System metrics collection (memory usage, active threads)
- Thread pool metrics simulation 
- Real-time metric display at 2-second intervals
- Clean startup and shutdown procedures

**Key Features Demonstrated:**
- Cross-platform system metrics collection
- Thread-safe metric registration and updates
- Memory-efficient ring buffer storage
- Easy-to-use global collector API

### 2. `monitoring_sample` 
An advanced example showing integration with the thread pool system:
- Real thread pool creation and job submission
- Work simulation with varying complexity
- Live metrics updates during high load
- Integration with the existing logger system

**Key Features Demonstrated:**
- Thread pool monitoring integration
- High-frequency job processing metrics
- Concurrent metric collection and job processing
- Production-like workload simulation

## 🚀 Building and Running

### Prerequisites
- CMake 3.16 or later
- C++20 capable compiler
- Thread System library dependencies

### Build Instructions
```bash
# From the thread_system root directory
./build.sh

# Or build just the monitoring samples
cd build
ninja simple_monitoring_sample monitoring_sample
```

### Running the Samples
```bash
# Run the simple demo
./bin/simple_monitoring_sample

# Run the advanced demo  
./bin/monitoring_sample
```

## 📈 Sample Output

### Simple Monitoring Sample
```
============================================================
         MONITORING MODULE DEMO
============================================================

🚀 Starting logger...
📈 Starting monitoring system...
⚡ Starting simulation...
   Monitoring for 10 seconds with 2-second intervals

📊 Snapshot 1 at 802s
   Memory: 6416449536 bytes | Threads: 8
   Pool Jobs: 40 completed | 10 pending
------------------------------------------------------------
📊 Snapshot 2 at 804s  
   Memory: 6433718272 bytes | Threads: 8
   Pool Jobs: 80 completed | 0 pending
------------------------------------------------------------

✅ Monitoring demo completed!

Features Demonstrated:
  ✓ Real-time metric collection
  ✓ Thread-safe metric updates
  ✓ Cross-platform compatibility
  ✓ Memory-efficient storage
  ✓ Easy integration API
```

## 🔧 Integration Guide

### Basic Usage
```cpp
#include "monitoring/core/metrics_collector.h"

using namespace monitoring_module;

// 1. Start monitoring
monitoring_config config;
config.collection_interval = std::chrono::milliseconds(100);
metrics::start_global_monitoring(config);

// 2. Register metrics
auto collector = global_metrics_collector::instance().get_collector();
auto system_metrics = std::make_shared<system_metrics>();
collector->register_system_metrics(system_metrics);

// 3. Get real-time data
auto snapshot = metrics::get_current_metrics();
std::cout << "Memory: " << snapshot.system.memory_usage_bytes.load() << "\n";

// 4. Cleanup
metrics::stop_global_monitoring();
```

### Advanced Configuration
```cpp
monitoring_config config;
config.collection_interval = std::chrono::milliseconds(50);  // High frequency
config.buffer_size = 1200;                                   // 1 minute of data
config.enable_system_metrics = true;
config.enable_thread_pool_metrics = true; 
config.low_overhead_mode = false;                           // Full feature set

metrics::start_global_monitoring(config);
```

## 📚 Key Concepts

### Metric Types
- **System Metrics**: CPU usage, memory consumption, active thread count
- **Thread Pool Metrics**: Job completion rates, queue depths, worker utilization
- **Worker Metrics**: Individual thread performance, processing times

### Collection Strategy
- **Lock-free Design**: Minimal performance impact during collection
- **Ring Buffer Storage**: Fixed memory footprint with configurable retention
- **Atomic Operations**: Thread-safe updates without mutex overhead
- **Configurable Intervals**: Balance between granularity and performance

### Performance Characteristics
- **Collection Overhead**: < 5% CPU impact at 100ms intervals
- **Memory Usage**: ~1-10MB depending on buffer size and metrics enabled
- **Thread Safety**: All operations are thread-safe by design
- **Platform Support**: Windows, Linux, macOS with unified API

## 🛠️ Customization

### Custom Metrics
You can extend the monitoring system with your own metrics:

```cpp
struct custom_metrics {
    std::atomic<std::uint64_t> business_metric{0};
    std::atomic<std::uint64_t> performance_counter{0};
    time_point timestamp;
    
    // Add copy constructor and assignment operator for atomic members
    custom_metrics() = default;
    custom_metrics(const custom_metrics& other) { /* ... */ }
    custom_metrics& operator=(const custom_metrics& other) { /* ... */ }
};
```

### Collection Intervals
- **High Frequency** (50-100ms): Real-time dashboards, critical monitoring
- **Medium Frequency** (500ms-1s): General purpose monitoring, alerts
- **Low Frequency** (5-10s): Long-term trends, resource planning

## 🔍 Troubleshooting

### Common Issues
1. **High CPU Usage**: Increase collection interval or enable low_overhead_mode
2. **Memory Growth**: Reduce buffer_size or disable unused metric types
3. **Missing Metrics**: Ensure proper metric registration before collection starts
4. **Platform Differences**: Some system metrics may vary between platforms

### Debug Tips
- Enable detailed logging to trace metric collection
- Monitor collection statistics via `get_collection_stats()`
- Use the simple sample first to verify basic functionality
- Check system permissions for reading system metrics

## 📖 Related Documentation

- [Monitoring Module Architecture](../../docs/architecture.md#monitoring-system)
- [Performance Guide](../../docs/performance.md#monitoring-overhead)
- [API Reference](../../docs/api-reference.md#monitoring-api)
- [Thread System Overview](../../README.md#real-time-monitoring-system)