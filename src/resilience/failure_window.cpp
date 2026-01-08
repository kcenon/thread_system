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

#include <kcenon/thread/resilience/failure_window.h>

#include <algorithm>

namespace kcenon::thread
{
	failure_window::failure_window(
		std::chrono::seconds window_size,
		std::size_t bucket_count)
		: window_size_(window_size)
		, bucket_count_(bucket_count)
		, buckets_(bucket_count)
	{
		bucket_duration_ = std::chrono::seconds(
			std::max<std::int64_t>(1, window_size_.count() / static_cast<std::int64_t>(bucket_count_)));
	}

	auto failure_window::record_success() -> void
	{
		expire_old_buckets();
		auto idx = get_current_bucket_index();
		buckets_[idx].successes.fetch_add(1, std::memory_order_relaxed);
	}

	auto failure_window::record_failure() -> void
	{
		expire_old_buckets();
		auto idx = get_current_bucket_index();
		buckets_[idx].failures.fetch_add(1, std::memory_order_relaxed);
	}

	auto failure_window::total_requests() const -> std::size_t
	{
		expire_old_buckets();

		std::size_t total = 0;
		for (const auto& bucket : buckets_)
		{
			total += bucket.successes.load(std::memory_order_relaxed);
			total += bucket.failures.load(std::memory_order_relaxed);
		}
		return total;
	}

	auto failure_window::failure_count() const -> std::size_t
	{
		expire_old_buckets();

		std::size_t failures = 0;
		for (const auto& bucket : buckets_)
		{
			failures += bucket.failures.load(std::memory_order_relaxed);
		}
		return failures;
	}

	auto failure_window::success_count() const -> std::size_t
	{
		expire_old_buckets();

		std::size_t successes = 0;
		for (const auto& bucket : buckets_)
		{
			successes += bucket.successes.load(std::memory_order_relaxed);
		}
		return successes;
	}

	auto failure_window::failure_rate() const -> double
	{
		auto total = total_requests();
		if (total == 0)
		{
			return 0.0;
		}
		return static_cast<double>(failure_count()) / static_cast<double>(total);
	}

	auto failure_window::reset() -> void
	{
		std::lock_guard<std::mutex> lock(mutex_);
		for (auto& bucket : buckets_)
		{
			bucket.successes.store(0, std::memory_order_relaxed);
			bucket.failures.store(0, std::memory_order_relaxed);
			bucket.timestamp.store(0, std::memory_order_relaxed);
		}
	}

	auto failure_window::get_current_bucket_index() const -> std::size_t
	{
		auto now = std::chrono::steady_clock::now();
		auto epoch_seconds = std::chrono::duration_cast<std::chrono::seconds>(
			now.time_since_epoch()).count();
		auto bucket_idx = static_cast<std::size_t>(
			(epoch_seconds / bucket_duration_.count()) % bucket_count_);

		// Initialize or reset bucket timestamp
		auto expected_ts = epoch_seconds - (epoch_seconds % bucket_duration_.count());
		auto current_ts = buckets_[bucket_idx].timestamp.load(std::memory_order_relaxed);

		if (current_ts != expected_ts)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			current_ts = buckets_[bucket_idx].timestamp.load(std::memory_order_relaxed);
			if (current_ts != expected_ts)
			{
				buckets_[bucket_idx].successes.store(0, std::memory_order_relaxed);
				buckets_[bucket_idx].failures.store(0, std::memory_order_relaxed);
				buckets_[bucket_idx].timestamp.store(expected_ts, std::memory_order_relaxed);
			}
		}

		return bucket_idx;
	}

	auto failure_window::expire_old_buckets() const -> void
	{
		auto now = std::chrono::steady_clock::now();
		auto epoch_seconds = std::chrono::duration_cast<std::chrono::seconds>(
			now.time_since_epoch()).count();
		auto window_start = epoch_seconds - window_size_.count();

		for (auto& bucket : buckets_)
		{
			auto ts = bucket.timestamp.load(std::memory_order_relaxed);
			if (ts != 0 && ts < window_start)
			{
				std::lock_guard<std::mutex> lock(mutex_);
				ts = bucket.timestamp.load(std::memory_order_relaxed);
				if (ts != 0 && ts < window_start)
				{
					bucket.successes.store(0, std::memory_order_relaxed);
					bucket.failures.store(0, std::memory_order_relaxed);
					bucket.timestamp.store(0, std::memory_order_relaxed);
				}
			}
		}
	}

} // namespace kcenon::thread
