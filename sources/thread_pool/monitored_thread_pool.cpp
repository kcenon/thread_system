#include "monitored_thread_pool.h"
#include "../thread_base/callback_job.h"
#include "logger.h"
#include "formatter.h"

namespace thread_pool_module
{
    monitored_thread_pool::monitored_thread_pool(
        const std::string& thread_title,
        bool enable_metrics)
        : thread_pool(thread_title)
        , metrics_enabled_(enable_metrics)
    {
        if (metrics_enabled_)
        {
            metrics_ = std::make_shared<metrics::thread_pool_metrics>(thread_title);
        }
    }
    
    monitored_thread_pool::~monitored_thread_pool()
    {
        stop();
    }
    
    auto monitored_thread_pool::start() -> std::optional<std::string>
    {
        auto result = thread_pool::start();
        
        if (!result.has_value() && metrics_enabled_)
        {
            // Update initial worker stats
            update_stats();
        }
        
        return result;
    }
    
    auto monitored_thread_pool::enqueue(std::unique_ptr<job>&& job) 
        -> std::optional<std::string>
    {
        if (!job)
        {
            return "Job is null";
        }
        
        // Check queue capacity before enqueuing
        auto queue = get_job_queue();
        if (!queue)
        {
            return "Job queue is null";
        }
        
        // Wrap job with metrics if enabled
        if (metrics_enabled_)
        {
            job = wrap_job_with_metrics(std::move(job));
        }
        
        // Enqueue the job
        auto result = thread_pool::enqueue(std::move(job));
        
        // Record rejection if failed
        if (result.has_value() && metrics_enabled_)
        {
            metrics_->record_rejection();
        }
        
        return result;
    }
    
    auto monitored_thread_pool::enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs)
        -> std::optional<std::string>
    {
        if (jobs.empty())
        {
            return "Jobs are empty";
        }
        
        // Wrap all jobs with metrics if enabled
        if (metrics_enabled_)
        {
            for (auto& job : jobs)
            {
                if (job)
                {
                    job = wrap_job_with_metrics(std::move(job));
                }
            }
        }
        
        // Enqueue the batch
        auto result = thread_pool::enqueue_batch(std::move(jobs));
        
        // Record rejection if failed
        if (result.has_value() && metrics_enabled_)
        {
            metrics_->record_rejection();
        }
        
        return result;
    }
    
    auto monitored_thread_pool::stop(const bool& immediately_stop) -> void
    {
        // Update final stats before stopping
        if (metrics_enabled_)
        {
            update_stats();
        }
        
        thread_pool::stop(immediately_stop);
    }
    
    auto monitored_thread_pool::get_metrics() const 
        -> std::shared_ptr<metrics::thread_pool_metrics>
    {
        return metrics_;
    }
    
    auto monitored_thread_pool::update_stats() -> void
    {
        if (!metrics_enabled_)
        {
            return;
        }
        
        auto now = std::chrono::steady_clock::now();
        
        // Rate limit stats updates to avoid overhead
        if (now - last_stats_update_ < std::chrono::milliseconds(100))
        {
            return;
        }
        
        std::shared_lock lock(stats_mutex_);
        
        // Update worker stats
        size_t total_workers = get_worker_count();
        size_t active_workers = get_active_worker_count();
        metrics_->update_worker_stats(total_workers, active_workers);
        
        // Update queue stats
        auto queue = get_job_queue();
        if (queue)
        {
            // TODO: Add get_size() method to job_queue
            // For now, use 0 as placeholder
            size_t depth = 0;
            size_t capacity = 10000; // Default capacity, could be made configurable
            metrics_->update_queue_stats(depth, capacity);
        }
        
        last_stats_update_ = now;
    }
    
    auto monitored_thread_pool::get_queue_depth() const -> size_t
    {
        // TODO: Add get_size() method to job_queue
        // For now, return 0
        return 0;
    }
    
    auto monitored_thread_pool::get_active_worker_count() const -> size_t
    {
        // This is an approximation - in a real implementation,
        // we would track this in the worker threads
        return active_workers_.load();
    }
    
    auto monitored_thread_pool::get_worker_count() const -> size_t
    {
        // TODO: Add getter for workers_.size() in base class
        // For now, return the approximate count based on active workers
        return active_workers_.load();
    }
    
    auto monitored_thread_pool::to_string() const -> std::string
    {
        std::string result = thread_pool::to_string();
        
        if (metrics_enabled_ && metrics_)
        {
            formatter::format_to(std::back_inserter(result), 
                "\n\tMetrics Summary:\n{}", 
                metrics_->to_json().dump(2));
        }
        
        return result;
    }
    
    auto monitored_thread_pool::wrap_job_with_metrics(std::unique_ptr<job>&& original_job)
        -> std::unique_ptr<job>
    {
        // Create a custom job class that wraps the original job with metrics
        class metrics_wrapper_job : public job
        {
        private:
            std::unique_ptr<job> original_job_;
            std::shared_ptr<metrics::thread_pool_metrics::job_tracker> tracker_;
            std::atomic<size_t>* active_workers_;
            
        public:
            metrics_wrapper_job(
                std::unique_ptr<job>&& original_job,
                std::shared_ptr<metrics::thread_pool_metrics::job_tracker> tracker,
                std::atomic<size_t>* active_workers)
                : job("metrics_wrapper")
                , original_job_(std::move(original_job))
                , tracker_(tracker)
                , active_workers_(active_workers)
            {
            }
            
            auto do_work() -> result_void override
            {
                // Increment active workers
                active_workers_->fetch_add(1);
                
                // Mark job as started
                tracker_->on_start();
                
                try
                {
                    // Execute the original job
                    auto result = original_job_->do_work();
                    
                    if (result.has_error())
                    {
                        tracker_->on_error();
                        active_workers_->fetch_sub(1);
                        return result;
                    }
                    
                    // Mark job as completed
                    tracker_->on_complete();
                    active_workers_->fetch_sub(1);
                    return result;
                }
                catch (...)
                {
                    tracker_->on_error();
                    active_workers_->fetch_sub(1);
                    throw;
                }
            }
        };
        
        // Create a metrics tracker for this job
        auto tracker = std::make_shared<metrics::thread_pool_metrics::job_tracker>(*metrics_);
        
        return std::make_unique<metrics_wrapper_job>(
            std::move(original_job), tracker, &active_workers_
        );
    }
    
    auto make_monitored_thread_pool(
        size_t worker_count,
        const std::string& thread_title,
        bool enable_metrics) -> std::shared_ptr<monitored_thread_pool>
    {
        auto pool = std::make_shared<monitored_thread_pool>(thread_title, enable_metrics);
        
        // Add the requested number of workers
        for (size_t i = 0; i < worker_count; ++i)
        {
            auto worker = std::make_unique<thread_worker>(true);  // use_time_tag = true
            
            if (auto error = pool->enqueue(std::move(worker)))
            {
                log_module::write_error("Failed to add worker: {}", *error);
            }
        }
        
        return pool;
    }

} // namespace thread_pool_module