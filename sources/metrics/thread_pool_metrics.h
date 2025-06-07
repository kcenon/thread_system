#pragma once

#include "metric_types.h"
#include "metric_registry.h"
#include <chrono>
#include <string>
#include <memory>

namespace metrics {

/**
 * @brief Comprehensive metrics collection for thread pools
 * 
 * Provides detailed metrics for monitoring thread pool performance,
 * including job processing, queue status, worker utilization, and more.
 */
class thread_pool_metrics {
private:
    // Job metrics
    std::shared_ptr<counter<uint64_t>> jobs_submitted_;
    std::shared_ptr<counter<uint64_t>> jobs_completed_;
    std::shared_ptr<counter<uint64_t>> jobs_failed_;
    std::shared_ptr<counter<uint64_t>> jobs_cancelled_;
    std::shared_ptr<counter<uint64_t>> jobs_rejected_;
    
    // Queue metrics
    std::shared_ptr<gauge<size_t>> queue_depth_;
    std::shared_ptr<gauge<size_t>> queue_capacity_;
    std::shared_ptr<histogram<double>> queue_wait_time_;
    
    // Worker metrics
    std::shared_ptr<gauge<size_t>> workers_active_;
    std::shared_ptr<gauge<size_t>> workers_idle_;
    std::shared_ptr<gauge<size_t>> workers_total_;
    std::shared_ptr<histogram<double>> worker_busy_time_;
    
    // Performance metrics
    std::shared_ptr<histogram<double>> job_duration_;
    std::shared_ptr<summary<double>> throughput_;
    std::shared_ptr<histogram<double>> job_size_bytes_;
    
    // Resource metrics
    std::shared_ptr<gauge<size_t>> memory_usage_;
    std::shared_ptr<gauge<double>> cpu_usage_;
    
    // Anomaly metrics
    std::shared_ptr<counter<uint64_t>> deadlock_detected_;
    std::shared_ptr<counter<uint64_t>> starvation_events_;
    
    const std::string pool_name_;
    
public:
    /**
     * @brief Construct metrics for a named thread pool
     * @param pool_name Name to identify this thread pool
     */
    explicit thread_pool_metrics(const std::string& pool_name = "default")
        : pool_name_(pool_name) {
        
        auto& registry = metric_registry::instance();
        
        // Initialize job metrics
        jobs_submitted_ = registry.register_metric<counter<uint64_t>>(
            pool_name_ + ".jobs.submitted",
            "Total number of jobs submitted to the pool"
        );
        
        jobs_completed_ = registry.register_metric<counter<uint64_t>>(
            pool_name_ + ".jobs.completed",
            "Total number of jobs successfully completed"
        );
        
        jobs_failed_ = registry.register_metric<counter<uint64_t>>(
            pool_name_ + ".jobs.failed",
            "Total number of jobs that failed with errors"
        );
        
        jobs_cancelled_ = registry.register_metric<counter<uint64_t>>(
            pool_name_ + ".jobs.cancelled",
            "Total number of jobs that were cancelled"
        );
        
        jobs_rejected_ = registry.register_metric<counter<uint64_t>>(
            pool_name_ + ".jobs.rejected",
            "Total number of jobs rejected due to queue overflow"
        );
        
        // Initialize queue metrics
        queue_depth_ = registry.register_metric<gauge<size_t>>(
            pool_name_ + ".queue.depth",
            "Current number of jobs waiting in queue"
        );
        
        queue_capacity_ = registry.register_metric<gauge<size_t>>(
            pool_name_ + ".queue.capacity",
            "Maximum queue capacity"
        );
        
        queue_wait_time_ = registry.register_metric<histogram<double>>(
            pool_name_ + ".queue.wait_time_seconds",
            "Time jobs spend waiting in queue before execution"
        );
        
        // Initialize worker metrics
        workers_active_ = registry.register_metric<gauge<size_t>>(
            pool_name_ + ".workers.active",
            "Number of workers currently processing jobs"
        );
        
        workers_idle_ = registry.register_metric<gauge<size_t>>(
            pool_name_ + ".workers.idle",
            "Number of workers waiting for jobs"
        );
        
        workers_total_ = registry.register_metric<gauge<size_t>>(
            pool_name_ + ".workers.total",
            "Total number of worker threads"
        );
        
        worker_busy_time_ = registry.register_metric<histogram<double>>(
            pool_name_ + ".workers.busy_time_seconds",
            "Time workers spend processing jobs"
        );
        
        // Initialize performance metrics
        job_duration_ = registry.register_metric<histogram<double>>(
            pool_name_ + ".job.duration_seconds",
            "Job execution duration"
        );
        
        throughput_ = registry.register_metric<summary<double>>(
            pool_name_ + ".throughput.jobs_per_second",
            "Jobs processed per second",
            std::chrono::seconds(60)  // 1-minute window
        );
        
        job_size_bytes_ = registry.register_metric<histogram<double>>(
            pool_name_ + ".job.size_bytes",
            "Size of job data in bytes"
        );
        
        // Initialize resource metrics
        memory_usage_ = registry.register_metric<gauge<size_t>>(
            pool_name_ + ".resources.memory_bytes",
            "Memory usage in bytes"
        );
        
        cpu_usage_ = registry.register_metric<gauge<double>>(
            pool_name_ + ".resources.cpu_percent",
            "CPU usage percentage"
        );
        
        // Initialize anomaly metrics
        deadlock_detected_ = registry.register_metric<counter<uint64_t>>(
            pool_name_ + ".anomalies.deadlock_detected",
            "Number of potential deadlocks detected"
        );
        
        starvation_events_ = registry.register_metric<counter<uint64_t>>(
            pool_name_ + ".anomalies.starvation_events",
            "Number of thread starvation events"
        );
    }
    
    /**
     * @brief Job tracker for automatic metric collection
     * 
     * RAII class that tracks job lifecycle and updates metrics accordingly
     */
    class job_tracker {
    private:
        thread_pool_metrics& metrics_;
        std::chrono::steady_clock::time_point enqueue_time_;
        std::chrono::steady_clock::time_point start_time_;
        bool started_{false};
        bool completed_{false};
        
    public:
        explicit job_tracker(thread_pool_metrics& metrics)
            : metrics_(metrics)
            , enqueue_time_(std::chrono::steady_clock::now()) {
            
            metrics_.jobs_submitted_->increment();
            metrics_.queue_depth_->increment();
        }
        
        /**
         * @brief Call when job is dequeued and starts execution
         */
        void on_start() {
            if (started_) return;
            started_ = true;
            
            start_time_ = std::chrono::steady_clock::now();
            auto wait_duration = std::chrono::duration<double>(
                start_time_ - enqueue_time_
            ).count();
            
            metrics_.queue_wait_time_->observe(wait_duration);
            metrics_.queue_depth_->decrement();
            metrics_.workers_active_->increment();
            metrics_.workers_idle_->decrement();
        }
        
        /**
         * @brief Call when job completes successfully
         */
        void on_complete() {
            if (completed_) return;
            completed_ = true;
            
            if (started_) {
                auto duration = std::chrono::duration<double>(
                    std::chrono::steady_clock::now() - start_time_
                ).count();
                
                metrics_.job_duration_->observe(duration);
                metrics_.jobs_completed_->increment();
                metrics_.workers_active_->decrement();
                metrics_.workers_idle_->increment();
                
                // Update throughput
                static thread_local auto last_throughput_update = 
                    std::chrono::steady_clock::now();
                static thread_local uint64_t jobs_in_window = 0;
                
                jobs_in_window++;
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration<double>(
                    now - last_throughput_update
                ).count();
                
                if (elapsed >= 1.0) {
                    metrics_.throughput_->observe(jobs_in_window / elapsed);
                    jobs_in_window = 0;
                    last_throughput_update = now;
                }
            }
        }
        
        /**
         * @brief Call when job fails with error
         */
        void on_error() {
            if (completed_) return;
            completed_ = true;
            
            metrics_.jobs_failed_->increment();
            
            if (started_) {
                metrics_.workers_active_->decrement();
                metrics_.workers_idle_->increment();
            } else {
                metrics_.queue_depth_->decrement();
            }
        }
        
        /**
         * @brief Call when job is cancelled
         */
        void on_cancel() {
            if (completed_) return;
            completed_ = true;
            
            metrics_.jobs_cancelled_->increment();
            
            if (!started_) {
                metrics_.queue_depth_->decrement();
            }
        }
        
        ~job_tracker() {
            if (!completed_) {
                on_error();  // Treat incomplete tracking as error
            }
        }
    };
    
    /**
     * @brief Update worker count metrics
     * @param total Total number of workers
     * @param active Number of active workers
     */
    void update_worker_stats(size_t total, size_t active) {
        workers_total_->set(total);
        workers_active_->set(active);
        workers_idle_->set(total - active);
    }
    
    /**
     * @brief Update queue status metrics
     * @param depth Current queue depth
     * @param capacity Maximum queue capacity
     */
    void update_queue_stats(size_t depth, size_t capacity) {
        queue_depth_->set(depth);
        queue_capacity_->set(capacity);
    }
    
    /**
     * @brief Update resource usage metrics
     * @param memory_bytes Memory usage in bytes
     * @param cpu_percent CPU usage percentage
     */
    void update_resource_stats(size_t memory_bytes, double cpu_percent) {
        memory_usage_->set(memory_bytes);
        cpu_usage_->set(cpu_percent);
    }
    
    /**
     * @brief Record a potential deadlock
     */
    void record_deadlock() {
        deadlock_detected_->increment();
    }
    
    /**
     * @brief Record a starvation event
     */
    void record_starvation() {
        starvation_events_->increment();
    }
    
    /**
     * @brief Record job rejection due to queue overflow
     */
    void record_rejection() {
        jobs_rejected_->increment();
    }
    
    /**
     * @brief Get all metrics as JSON
     * @return JSON object containing all thread pool metrics
     */
    json to_json() const {
        return {
            {"pool_name", pool_name_},
            {"jobs", {
                {"submitted", jobs_submitted_->to_json()},
                {"completed", jobs_completed_->to_json()},
                {"failed", jobs_failed_->to_json()},
                {"cancelled", jobs_cancelled_->to_json()},
                {"rejected", jobs_rejected_->to_json()}
            }},
            {"queue", {
                {"depth", queue_depth_->to_json()},
                {"capacity", queue_capacity_->to_json()},
                {"wait_time", queue_wait_time_->to_json()}
            }},
            {"workers", {
                {"active", workers_active_->to_json()},
                {"idle", workers_idle_->to_json()},
                {"total", workers_total_->to_json()},
                {"busy_time", worker_busy_time_->to_json()}
            }},
            {"performance", {
                {"job_duration", job_duration_->to_json()},
                {"throughput", throughput_->to_json()},
                {"job_size", job_size_bytes_->to_json()}
            }},
            {"resources", {
                {"memory", memory_usage_->to_json()},
                {"cpu", cpu_usage_->to_json()}
            }},
            {"anomalies", {
                {"deadlocks", deadlock_detected_->to_json()},
                {"starvation", starvation_events_->to_json()}
            }}
        };
    }
};

}  // namespace metrics