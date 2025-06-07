#pragma once

#include "typed_thread_pool.h"
#include "job_types.h"
#include "callback_typed_job.h"
#include "../metrics/thread_pool_metrics.h"
#include "../metrics/metric_registry.h"
#include <memory>
#include <unordered_map>
#include <string>
#include "formatter.h"
#include "logger.h"

using json = nlohmann::json;
using error = thread_module::error;

namespace typed_thread_pool_module
{
    /**
     * @class monitored_typed_thread_pool_t
     * @brief Type-based thread pool with integrated metrics collection
     * 
     * Extends typed_thread_pool_t with comprehensive metrics tracking
     * including per-type metrics for detailed performance analysis.
     * 
     * @tparam job_type The job type enum (e.g., job_types)
     */
    template <typename job_type = job_types>
    class monitored_typed_thread_pool_t : public typed_thread_pool_t<job_type>
    {
    public:
        /**
         * @brief Construct a monitored typed thread pool
         * @param thread_title Pool identifier for logging and metrics
         * @param enable_metrics Whether to enable metrics collection (default: true)
         */
        explicit monitored_typed_thread_pool_t(
            const std::string& thread_title = "monitored_typed_thread_pool",
            bool enable_metrics = true)
            : typed_thread_pool_t<job_type>(thread_title)
            , metrics_enabled_(enable_metrics)
            , pool_title_(thread_title)
        {
            if (metrics_enabled_)
            {
                metrics_ = std::make_shared<metrics::thread_pool_metrics>(thread_title);
                
                // Register type-specific metrics
                register_job_type_metrics();
            }
        }
        
        /**
         * @brief Destructor ensures clean shutdown
         */
        virtual ~monitored_typed_thread_pool_t()
        {
            stop();
        }
        
        /**
         * @brief Start the thread pool with metrics tracking
         * @return Result containing error if start fails
         */
        auto start() -> result_void
        {
            auto result = typed_thread_pool_t<job_type>::start();
            
            if (!result.has_error() && metrics_enabled_)
            {
                // Update initial stats
                update_stats();
            }
            
            return result;
        }
        
        // Make base class worker enqueue visible
        using typed_thread_pool_t<job_type>::enqueue;
        
        /**
         * @brief Enqueue a job with automatic metrics tracking
         * @param job The typed job to execute
         * @return Result containing error if enqueue fails
         */
        auto enqueue(std::unique_ptr<typed_job_t<job_type>>&& job) -> result_void
        {
            if (!job)
            {
                return error(error_code::job_creation_failed, "Job is null");
            }
            
            if (metrics_enabled_)
            {
                // Get job type for metrics
                auto type = job->priority();
                auto type_name = get_type_name(type);
                
                // Track type-specific submission
                if (auto it = type_counters_.find(type_name); it != type_counters_.end())
                {
                    it->second->increment();
                }
                
                // Wrap with metrics tracking
                job = wrap_job_with_metrics(std::move(job), type);
            }
            
            // Enqueue the job
            auto result = typed_thread_pool_t<job_type>::enqueue(std::move(job));
            
            if (result.has_error() && metrics_enabled_)
            {
                metrics_->record_rejection();
            }
            
            return result;
        }
        
        /**
         * @brief Enqueue multiple jobs with batch metrics tracking
         * @param jobs Vector of typed jobs to execute
         * @return Result containing error if enqueue fails
         */
        auto enqueue_batch(std::vector<std::unique_ptr<typed_job_t<job_type>>>&& jobs)
            -> result_void
        {
            if (jobs.empty())
            {
                return error(error_code::job_creation_failed, "Jobs vector is empty");
            }
            
            if (metrics_enabled_)
            {
                // Wrap all jobs with metrics
                for (auto& job : jobs)
                {
                    if (job)
                    {
                        auto type = job->priority();
                        auto type_name = get_type_name(type);
                        
                        // Track type-specific submission
                        if (auto it = type_counters_.find(type_name); it != type_counters_.end())
                        {
                            it->second->increment();
                        }
                        
                        job = wrap_job_with_metrics(std::move(job), type);
                    }
                }
            }
            
            // Enqueue the batch
            auto result = typed_thread_pool_t<job_type>::enqueue_batch(std::move(jobs));
            
            if (result.has_error() && metrics_enabled_)
            {
                metrics_->record_rejection();
            }
            
            return result;
        }
        
        /**
         * @brief Stop the thread pool and finalize metrics
         * @param clear_queue If true, cancel pending jobs
         * @return Result containing error if stop fails
         */
        auto stop(bool clear_queue = false) -> result_void
        {
            // Update final stats
            if (metrics_enabled_)
            {
                update_stats();
            }
            
            return typed_thread_pool_t<job_type>::stop(clear_queue);
        }
        
        /**
         * @brief Get the metrics object for this pool
         * @return Shared pointer to metrics object, or nullptr if metrics disabled
         */
        [[nodiscard]] auto get_metrics() const 
            -> std::shared_ptr<metrics::thread_pool_metrics>
        {
            return metrics_;
        }
        
        /**
         * @brief Update worker and queue statistics
         * 
         * Should be called periodically to update gauge metrics
         */
        virtual auto update_stats() -> void
        {
            if (!metrics_enabled_)
            {
                return;
            }
            
            auto now = std::chrono::steady_clock::now();
            
            // Rate limit updates
            if (now - last_stats_update_ < std::chrono::milliseconds(100))
            {
                return;
            }
            
            // Update worker stats
            // TODO: Add getter for worker count in base class
            size_t total_workers = 6; // Default count, should be made configurable
            size_t active_workers = count_active_workers();
            metrics_->update_worker_stats(total_workers, active_workers);
            
            // Update queue stats
            if (auto queue = this->get_job_queue())
            {
                // TODO: Add get_size() method to typed_job_queue
                size_t depth = 0;
                size_t capacity = 10000; // Could be made configurable
                metrics_->update_queue_stats(depth, capacity);
                
                // Check for starvation
                if (active_workers == 0 && depth > 0)
                {
                    metrics_->record_starvation();
                }
            }
            
            last_stats_update_ = now;
        }
        
        /**
         * @brief Get type-specific metrics
         * @param type The job type to query
         * @return JSON object with type-specific metrics
         */
        [[nodiscard]] auto get_type_metrics(job_type type) const -> json
        {
            auto type_name = get_type_name(type);
            json result;
            
            if (auto it = type_counters_.find(type_name); it != type_counters_.end())
            {
                result["submitted"] = it->second->get();
            }
            
            if (auto it = type_histograms_.find(type_name); it != type_histograms_.end())
            {
                result["latency"] = it->second->to_json();
            }
            
            return result;
        }
        
        /**
         * @brief Extended string representation including metrics summary
         * @return String describing pool state and metrics
         */
        [[nodiscard]] auto to_string() const -> std::string
        {
            std::string result = typed_thread_pool_t<job_type>::to_string();
            
            if (metrics_enabled_ && metrics_)
            {
                formatter::format_to(std::back_inserter(result), 
                    "\n\tMetrics Summary:\n{}", 
                    metrics_->to_json().dump(2));
                
                // Add type-specific metrics
                formatter::format_to(std::back_inserter(result), 
                    "\n\tType Metrics:");
                
                for (const auto& [type_name, counter] : type_counters_)
                {
                    formatter::format_to(std::back_inserter(result), 
                        "\n\t  {}: {} jobs", type_name, counter->get());
                }
            }
            
            return result;
        }
        
    protected:
        /**
         * @brief Wrap a typed job with metrics tracking
         * @param original_job The job to wrap
         * @param type The job type
         * @return Job wrapped with metrics tracking
         */
        auto wrap_job_with_metrics(
            std::unique_ptr<typed_job_t<job_type>>&& original_job,
            job_type type) -> std::unique_ptr<typed_job_t<job_type>>
        {
            // Create a metrics tracker
            auto tracker = std::make_shared<metrics::thread_pool_metrics::job_tracker>(*metrics_);
            auto type_name = get_type_name(type);
            auto type_histogram = type_histograms_[type_name];
            
            // Create a wrapper job that preserves the type
            class metrics_wrapper_job : public typed_job_t<job_type>
            {
            private:
                std::unique_ptr<typed_job_t<job_type>> original_job_;
                std::shared_ptr<metrics::thread_pool_metrics::job_tracker> tracker_;
                std::shared_ptr<metrics::histogram<double>> type_histogram_;
                std::chrono::steady_clock::time_point start_time_;
                
            public:
                metrics_wrapper_job(
                    std::unique_ptr<typed_job_t<job_type>>&& job,
                    std::shared_ptr<metrics::thread_pool_metrics::job_tracker> tracker,
                    std::shared_ptr<metrics::histogram<double>> histogram)
                    : typed_job_t<job_type>(job->priority())
                    , original_job_(std::move(job))
                    , tracker_(tracker)
                    , type_histogram_(histogram)
                {
                }
                
                auto do_work() -> result_void override
                {
                    tracker_->on_start();
                    start_time_ = std::chrono::steady_clock::now();
                    
                    try
                    {
                        auto result = original_job_->do_work();
                        
                        auto duration = std::chrono::duration<double>(
                            std::chrono::steady_clock::now() - start_time_
                        ).count();
                        
                        if (type_histogram_)
                        {
                            type_histogram_->observe(duration);
                        }
                        
                        if (result.has_error())
                        {
                            tracker_->on_error();
                        }
                        else
                        {
                            tracker_->on_complete();
                        }
                        
                        return result;
                    }
                    catch (...)
                    {
                        tracker_->on_error();
                        throw;
                    }
                }
            };
            
            return std::make_unique<metrics_wrapper_job>(
                std::move(original_job), tracker, type_histogram
            );
        }
        
    private:
        bool metrics_enabled_;
        std::string pool_title_;
        std::shared_ptr<metrics::thread_pool_metrics> metrics_;
        std::chrono::steady_clock::time_point last_stats_update_;
        
        // Type-specific metrics
        std::unordered_map<std::string, std::shared_ptr<metrics::counter<uint64_t>>> type_counters_;
        std::unordered_map<std::string, std::shared_ptr<metrics::histogram<double>>> type_histograms_;
        
        /**
         * @brief Register metrics for each job type
         */
        void register_job_type_metrics()
        {
            auto& registry = metrics::metric_registry::instance();
            
            // Register metrics for each job type value
            // Assume we have 3 job types for now
            for (int i = 0; i < 3; ++i)
            {
                auto type = static_cast<job_type>(i);
                auto type_name = get_type_name(type);
                
                // Counter for jobs submitted by type
                type_counters_[type_name] = registry.register_metric<metrics::counter<uint64_t>>(
                    pool_title_ + ".jobs." + type_name + ".submitted",
                    "Jobs submitted with type " + type_name
                );
                
                // Histogram for type-specific latency
                type_histograms_[type_name] = registry.register_metric<metrics::histogram<double>>(
                    pool_title_ + ".jobs." + type_name + ".latency_seconds",
                    "Execution time for " + type_name + " jobs"
                );
            }
        }
        
        /**
         * @brief Convert job type to string name
         * @param type The job type
         * @return String representation of the type
         */
        [[nodiscard]] static auto get_type_name(job_type type) -> std::string
        {
            // Default implementation for job_types enum
            if constexpr (std::is_same_v<job_type, job_types>)
            {
                switch (type)
                {
                    case static_cast<job_type>(0): return "realtime";
                    case static_cast<job_type>(1): return "batch";
                    case static_cast<job_type>(2): return "background";
                    default: return "unknown";
                }
            }
            else
            {
                // For custom types, use numeric representation
                return std::to_string(static_cast<int>(type));
            }
        }
        
        /**
         * @brief Count active workers (approximation)
         * @return Number of workers currently processing jobs
         */
        [[nodiscard]] auto count_active_workers() const -> size_t
        {
            // In a real implementation, we would track this in the workers
            // For now, return an approximation based on queue depth
            if (auto queue = const_cast<monitored_typed_thread_pool_t*>(this)->get_job_queue())
            {
                // TODO: Add get_size() method to typed_job_queue
                size_t queue_size = 0;
                // TODO: Add getter for worker count
                size_t total_workers = 6; // Default count
                
                // If queue is empty, assume some workers are idle
                if (queue_size == 0)
                {
                    return 0;
                }
                
                // Otherwise, assume all workers are busy if queue has jobs
                return std::min(queue_size, total_workers);
            }
            
            return 0;
        }
    };
    
    /**
     * @brief Factory function to create a monitored typed thread pool
     * @tparam job_type The job type enum
     * @param worker_count Number of worker threads
     * @param thread_title Pool identifier
     * @param enable_metrics Whether to enable metrics
     * @return Shared pointer to configured pool
     */
    template <typename job_type = job_types>
    auto make_monitored_typed_thread_pool(
        size_t worker_count,
        const std::string& thread_title = "monitored_typed_pool",
        bool enable_metrics = true) -> std::shared_ptr<monitored_typed_thread_pool_t<job_type>>
    {
        auto pool = std::make_shared<monitored_typed_thread_pool_t<job_type>>(
            thread_title, enable_metrics
        );
        
        // Add workers - typed workers handle all job types by default
        for (size_t i = 0; i < worker_count; ++i)
        {
            auto worker = std::make_unique<typed_thread_worker_t<job_type>>();
            
            if (auto result = pool->enqueue(std::move(worker)); result.has_error())
            {
                log_module::write_error("Failed to add worker: {}", 
                    result.get_error().to_string());
            }
        }
        
        return pool;
    }

} // namespace typed_thread_pool_module