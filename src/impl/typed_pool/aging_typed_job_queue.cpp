/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#include <kcenon/thread/impl/typed_pool/aging_typed_job_queue.h>
#include <kcenon/thread/core/error_handling.h>

#include <cmath>
#include <algorithm>

namespace kcenon::thread
{
	template <typename job_type>
	aging_typed_job_queue_t<job_type>::aging_typed_job_queue_t(priority_aging_config config)
		: config_(std::move(config))
		, stats_start_time_(std::chrono::steady_clock::now())
	{
	}

	template <typename job_type>
	aging_typed_job_queue_t<job_type>::~aging_typed_job_queue_t()
	{
		stop_aging();
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::start_aging() -> void
	{
		if (aging_running_.exchange(true))
		{
			return; // Already running
		}

		aging_thread_ = std::make_unique<std::thread>([this]() {
			aging_loop();
		});
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::stop_aging() -> void
	{
		if (!aging_running_.exchange(false))
		{
			return; // Not running
		}

		{
			std::lock_guard lock(aging_mutex_);
			aging_cv_.notify_all();
		}

		if (aging_thread_ && aging_thread_->joinable())
		{
			aging_thread_->join();
		}
		aging_thread_.reset();
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::is_aging_running() const -> bool
	{
		return aging_running_.load();
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::aging_loop() -> void
	{
		while (aging_running_.load())
		{
			{
				std::unique_lock lock(aging_mutex_);
				aging_cv_.wait_for(lock, config_.aging_interval, [this]() {
					return !aging_running_.load();
				});
			}

			if (!aging_running_.load())
			{
				break;
			}

			if (config_.enabled)
			{
				apply_aging();
				check_starvation();
			}
		}
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::apply_aging() -> std::size_t
	{
		std::lock_guard jobs_lock(jobs_mutex_);

		std::size_t boosts_applied = 0;
		std::chrono::milliseconds max_wait{0};
		std::chrono::milliseconds total_wait{0};

		for (auto* job : aging_jobs_)
		{
			if (!job)
			{
				continue;
			}

			auto wait = job->wait_time();
			total_wait += wait;

			if (wait > max_wait)
			{
				max_wait = wait;
			}

			// Calculate number of intervals waited
			auto intervals = wait.count() / config_.aging_interval.count();
			if (intervals > 0)
			{
				auto boost = calculate_boost(wait);
				if (boost > 0 && !job->is_max_boosted())
				{
					job->apply_boost(boost);
					++boosts_applied;

					if (job->is_max_boosted())
					{
						std::lock_guard stats_lock(stats_mutex_);
						++stats_.jobs_reached_max_boost;
					}
				}
			}
		}

		if (!aging_jobs_.empty())
		{
			update_stats(boosts_applied, max_wait, total_wait, aging_jobs_.size());
		}

		return boosts_applied;
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::calculate_boost(
		std::chrono::milliseconds wait_time) const -> int
	{
		auto intervals = static_cast<double>(wait_time.count()) /
		                 static_cast<double>(config_.aging_interval.count());

		int boost = 0;

		switch (config_.curve)
		{
		case aging_curve::linear:
			boost = static_cast<int>(intervals) * config_.priority_boost_per_interval;
			break;

		case aging_curve::exponential:
			// Exponential: boost increases faster over time
			boost = static_cast<int>(
				std::pow(config_.exponential_factor, intervals) - 1.0
			) * config_.priority_boost_per_interval;
			break;

		case aging_curve::logarithmic:
			// Logarithmic: fast initial boost, slower over time
			if (intervals > 0)
			{
				boost = static_cast<int>(
					std::log(intervals + 1) / std::log(2.0)
				) * config_.priority_boost_per_interval;
			}
			break;
		}

		// Cap at max boost
		return std::min(boost, config_.max_priority_boost);
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::check_starvation() -> void
	{
		if (!config_.starvation_callback)
		{
			return;
		}

		auto threshold = std::chrono::duration_cast<std::chrono::milliseconds>(
			config_.starvation_threshold
		);

		std::lock_guard jobs_lock(jobs_mutex_);

		for (auto* job : aging_jobs_)
		{
			if (!job)
			{
				continue;
			}

			if (job->wait_time() > threshold)
			{
				config_.starvation_callback(job->to_job_info());

				{
					std::lock_guard stats_lock(stats_mutex_);
					++stats_.starvation_alerts;
				}
			}
		}
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::get_starving_jobs() const -> std::vector<job_info>
	{
		std::vector<job_info> result;

		auto threshold = std::chrono::duration_cast<std::chrono::milliseconds>(
			config_.starvation_threshold
		);

		std::lock_guard lock(jobs_mutex_);

		for (auto* job : aging_jobs_)
		{
			if (job && job->wait_time() > threshold)
			{
				result.push_back(job->to_job_info());
			}
		}

		return result;
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::get_aging_stats() const -> aging_stats
	{
		std::lock_guard lock(stats_mutex_);
		return stats_;
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::reset_aging_stats() -> void
	{
		std::lock_guard lock(stats_mutex_);
		stats_ = aging_stats{};
		stats_start_time_ = std::chrono::steady_clock::now();
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::set_aging_config(priority_aging_config config) -> void
	{
		std::lock_guard lock(aging_mutex_);
		config_ = std::move(config);
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::get_aging_config() const
		-> const priority_aging_config&
	{
		return config_;
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::enqueue(
		std::unique_ptr<aging_typed_job_t<job_type>>&& value) -> common::VoidResult
	{
		if (!value)
		{
			return common::error_info{
				static_cast<int>(error_code::invalid_argument),
				"Null job",
				"thread_system"
			};
		}

		if (stopped_.load())
		{
			return common::error_info{
				static_cast<int>(error_code::queue_stopped),
				"Queue is stopped",
				"thread_system"
			};
		}

		// Register for aging tracking
		register_aging_job(value.get());

		// Set max boost from config
		value->set_max_boost(config_.max_priority_boost);

		// Get the priority and enqueue to appropriate queue
		auto priority = value->priority();
		auto* queue = get_or_create_queue(priority);
		return queue->enqueue(std::move(value));
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::register_aging_job(
		aging_typed_job_t<job_type>* job) -> void
	{
		if (!job)
		{
			return;
		}

		std::lock_guard lock(jobs_mutex_);
		aging_jobs_.push_back(job);
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::unregister_aging_job(
		aging_typed_job_t<job_type>* job) -> void
	{
		if (!job)
		{
			return;
		}

		std::lock_guard lock(jobs_mutex_);
		auto it = std::find(aging_jobs_.begin(), aging_jobs_.end(), job);
		if (it != aging_jobs_.end())
		{
			aging_jobs_.erase(it);
		}
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::update_stats(
		std::size_t boosts_applied,
		std::chrono::milliseconds max_wait,
		std::chrono::milliseconds total_wait,
		std::size_t job_count) -> void
	{
		std::lock_guard lock(stats_mutex_);

		stats_.total_boosts_applied += boosts_applied;

		if (max_wait > stats_.max_wait_time)
		{
			stats_.max_wait_time = max_wait;
		}

		if (job_count > 0)
		{
			stats_.avg_wait_time = std::chrono::milliseconds{
				total_wait.count() / static_cast<long long>(job_count)
			};
		}

		// Calculate boost rate (boosts per second)
		auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
			std::chrono::steady_clock::now() - stats_start_time_
		);
		if (elapsed.count() > 0)
		{
			stats_.boost_rate = static_cast<double>(stats_.total_boosts_applied) /
			                    static_cast<double>(elapsed.count());
		}
	}

	// ============================================
	// typed_job_queue_t compatible API implementation
	// ============================================

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::get_or_create_queue(const job_type& type) -> queue_type*
	{
		std::unique_lock lock(queues_mutex_);
		auto it = job_queues_.find(type);
		if (it == job_queues_.end())
		{
			auto [new_it, inserted] = job_queues_.emplace(type, std::make_unique<queue_type>());
			return new_it->second.get();
		}
		return it->second.get();
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::enqueue(std::unique_ptr<job>&& value) -> common::VoidResult
	{
		if (!value)
		{
			return common::error_info{
				static_cast<int>(error_code::invalid_argument),
				"Null job",
				"thread_system"
			};
		}

		if (stopped_.load())
		{
			return common::error_info{
				static_cast<int>(error_code::queue_stopped),
				"Queue is stopped",
				"thread_system"
			};
		}

		// For non-typed jobs, use default priority
		auto* queue = get_or_create_queue(job_type{});
		return queue->enqueue(std::move(value));
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::enqueue(
		std::unique_ptr<typed_job_t<job_type>>&& value) -> common::VoidResult
	{
		if (!value)
		{
			return common::error_info{
				static_cast<int>(error_code::invalid_argument),
				"Null job",
				"thread_system"
			};
		}

		if (stopped_.load())
		{
			return common::error_info{
				static_cast<int>(error_code::queue_stopped),
				"Queue is stopped",
				"thread_system"
			};
		}

		auto priority = value->priority();
		auto* queue = get_or_create_queue(priority);
		return queue->enqueue(std::move(value));
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::enqueue_batch(
		std::vector<std::unique_ptr<typed_job_t<job_type>>>&& jobs) -> common::VoidResult
	{
		for (auto& job : jobs)
		{
			auto result = enqueue(std::move(job));
			if (result.is_err())
			{
				return result;
			}
		}
		return common::ok();
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::dequeue() -> common::Result<std::unique_ptr<job>>
	{
		std::shared_lock lock(queues_mutex_);

		for (auto& [type, queue] : job_queues_)
		{
			if (queue && !queue->empty())
			{
				auto result = queue->dequeue();
				if (result.is_ok())
				{
					return result;
				}
			}
		}

		return common::error_info{
			static_cast<int>(error_code::job_invalid),
			"No job available",
			"thread_system"
		};
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::try_dequeue_from_priority(const job_type& priority)
		-> std::optional<std::unique_ptr<typed_job_t<job_type>>>
	{
		std::shared_lock lock(queues_mutex_);

		auto it = job_queues_.find(priority);
		if (it == job_queues_.end() || !it->second || it->second->empty())
		{
			return std::nullopt;
		}

		auto result = it->second->dequeue();
		if (result.is_err())
		{
			return std::nullopt;
		}

		auto job = std::move(result.value());
		// Cast back to typed_job_t if possible
		auto* typed_ptr = dynamic_cast<typed_job_t<job_type>*>(job.release());
		if (typed_ptr)
		{
			return std::unique_ptr<typed_job_t<job_type>>(typed_ptr);
		}

		return std::nullopt;
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::dequeue(const std::vector<job_type>& types)
		-> common::Result<std::unique_ptr<typed_job_t<job_type>>>
	{
		for (const auto& type : types)
		{
			auto result = try_dequeue_from_priority(type);
			if (result.has_value())
			{
				return std::move(result.value());
			}
		}

		return common::error_info{
			static_cast<int>(error_code::job_invalid),
			"No job available for specified types",
			"thread_system"
		};
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::dequeue(utility_module::span<const job_type> types)
		-> common::Result<std::unique_ptr<typed_job_t<job_type>>>
	{
		for (const auto& type : types)
		{
			auto result = try_dequeue_from_priority(type);
			if (result.has_value())
			{
				return std::move(result.value());
			}
		}

		return common::error_info{
			static_cast<int>(error_code::job_invalid),
			"No job available for specified types",
			"thread_system"
		};
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::clear() -> void
	{
		std::unique_lock lock(queues_mutex_);

		for (auto& [type, queue] : job_queues_)
		{
			if (queue)
			{
				queue->clear();
			}
		}

		{
			std::lock_guard jobs_lock(jobs_mutex_);
			aging_jobs_.clear();
		}
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::empty(const std::vector<job_type>& types) const -> bool
	{
		std::shared_lock lock(queues_mutex_);

		for (const auto& type : types)
		{
			auto it = job_queues_.find(type);
			if (it != job_queues_.end() && it->second && !it->second->empty())
			{
				return false;
			}
		}

		return true;
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::empty(utility_module::span<const job_type> types) const -> bool
	{
		std::shared_lock lock(queues_mutex_);

		for (const auto& type : types)
		{
			auto it = job_queues_.find(type);
			if (it != job_queues_.end() && it->second && !it->second->empty())
			{
				return false;
			}
		}

		return true;
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::to_string() const -> std::string
	{
		std::ostringstream oss;
		oss << "aging_typed_job_queue{";
		oss << "aging_running: " << (aging_running_.load() ? "true" : "false");
		oss << ", stopped: " << (stopped_.load() ? "true" : "false");
		oss << ", total_jobs: " << size();
		oss << "}";
		return oss.str();
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::stop() -> void
	{
		stopped_.store(true);

		std::unique_lock lock(queues_mutex_);
		for (auto& [type, queue] : job_queues_)
		{
			if (queue)
			{
				queue->stop();
			}
		}
	}

	template <typename job_type>
	auto aging_typed_job_queue_t<job_type>::size() const -> std::size_t
	{
		std::shared_lock lock(queues_mutex_);
		std::size_t total = 0;

		for (const auto& [type, queue] : job_queues_)
		{
			if (queue)
			{
				total += queue->size();
			}
		}

		return total;
	}

	// Explicit template instantiation for job_types
	template class aging_typed_job_queue_t<job_types>;

} // namespace kcenon::thread
