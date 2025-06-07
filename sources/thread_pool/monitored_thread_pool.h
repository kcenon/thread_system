#pragma once

#include "thread_pool.h"
#include "../metrics/thread_pool_metrics.h"
#include <memory>
#include <optional>

namespace thread_pool_module
{
    /**
     * @class monitored_thread_pool
     * @brief Thread pool with integrated metrics collection
     * 
     * Extends the basic thread_pool with comprehensive metrics tracking
     * for monitoring performance, resource usage, and health.
     */
    class monitored_thread_pool : public thread_pool
    {
    public:
        /**
         * @brief Construct a monitored thread pool
         * @param thread_title Pool identifier for logging and metrics
         * @param enable_metrics Whether to enable metrics collection (default: true)
         */
        explicit monitored_thread_pool(
            const std::string& thread_title = "monitored_thread_pool",
            bool enable_metrics = true);
        
        /**
         * @brief Destructor ensures clean shutdown
         */
        virtual ~monitored_thread_pool();
        
        /**
         * @brief Start the thread pool with metrics tracking
         * @return Error message if start fails, nullopt on success
         */
        auto start() -> std::optional<std::string>;
        
        /**
         * @brief Enqueue a job with automatic metrics tracking
         * @param job The job to execute
         * @return Error message if enqueue fails, nullopt on success
         */
        auto enqueue(std::unique_ptr<job>&& job) -> std::optional<std::string>;
        
        /**
         * @brief Enqueue multiple jobs with batch metrics tracking
         * @param jobs Vector of jobs to execute
         * @return Error message if enqueue fails, nullopt on success
         */
        auto enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) 
            -> std::optional<std::string>;
            
        // Make worker enqueue methods visible
        using thread_pool::enqueue;  // For thread_worker overload
        
        /**
         * @brief Stop the thread pool and finalize metrics
         * @param immediately_stop If true, cancel pending jobs
         */
        auto stop(const bool& immediately_stop = false) -> void;
        
        /**
         * @brief Get the metrics object for this pool
         * @return Shared pointer to metrics object, or nullptr if metrics disabled
         */
        [[nodiscard]] auto get_metrics() const -> std::shared_ptr<metrics::thread_pool_metrics>;
        
        /**
         * @brief Update worker and queue statistics
         * 
         * Should be called periodically to update gauge metrics
         */
        auto update_stats() -> void;
        
        /**
         * @brief Get current queue depth
         * @return Number of jobs waiting in queue
         */
        [[nodiscard]] auto get_queue_depth() const -> size_t;
        
        /**
         * @brief Get number of active workers
         * @return Count of workers currently processing jobs
         */
        [[nodiscard]] auto get_active_worker_count() const -> size_t;
        
        /**
         * @brief Get total number of workers
         * @return Total worker count
         */
        [[nodiscard]] auto get_worker_count() const -> size_t;
        
        /**
         * @brief Extended string representation including metrics summary
         * @return String describing pool state and metrics
         */
        [[nodiscard]] auto to_string() const -> std::string;
        
    protected:
        /**
         * @brief Wrap a job with metrics tracking
         * @param original_job The job to wrap
         * @return Job wrapped with metrics tracking
         */
        auto wrap_job_with_metrics(std::unique_ptr<job>&& original_job) 
            -> std::unique_ptr<job>;
        
    private:
        bool metrics_enabled_;
        std::shared_ptr<metrics::thread_pool_metrics> metrics_;
        mutable std::shared_mutex stats_mutex_;
        
        // Cached stats for performance
        std::atomic<size_t> active_workers_{0};
        std::chrono::steady_clock::time_point last_stats_update_;
    };
    
    /**
     * @brief Factory function to create a monitored thread pool with workers
     * @param worker_count Number of worker threads to create
     * @param thread_title Pool identifier
     * @param enable_metrics Whether to enable metrics
     * @return Shared pointer to configured thread pool
     */
    auto make_monitored_thread_pool(
        size_t worker_count,
        const std::string& thread_title = "monitored_pool",
        bool enable_metrics = true) -> std::shared_ptr<monitored_thread_pool>;

} // namespace thread_pool_module