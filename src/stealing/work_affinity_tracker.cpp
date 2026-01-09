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

#include <kcenon/thread/stealing/work_affinity_tracker.h>

#include <algorithm>

namespace kcenon::thread
{

work_affinity_tracker::work_affinity_tracker(std::size_t worker_count,
                                             std::size_t history_size)
	: worker_count_(worker_count)
	, history_size_(history_size)
	, matrix_size_(0)
	, total_cooperations_(0)
{
	if (worker_count_ > 1)
	{
		// Size of upper triangular matrix without diagonal
		// For n workers: n*(n-1)/2 pairs
		matrix_size_ = (worker_count_ * (worker_count_ - 1)) / 2;
		cooperation_matrix_ =
			std::make_unique<std::atomic<std::uint64_t>[]>(matrix_size_);

		// Initialize all counters to zero
		for (std::size_t i = 0; i < matrix_size_; ++i)
		{
			cooperation_matrix_[i].store(0, std::memory_order_relaxed);
		}
	}
}

work_affinity_tracker::work_affinity_tracker(work_affinity_tracker&& other) noexcept
	: worker_count_(other.worker_count_)
	, history_size_(other.history_size_)
	, matrix_size_(other.matrix_size_)
	, cooperation_matrix_(std::move(other.cooperation_matrix_))
	, total_cooperations_(other.total_cooperations_.load(std::memory_order_relaxed))
{
	other.worker_count_ = 0;
	other.history_size_ = 0;
	other.matrix_size_ = 0;
	other.total_cooperations_.store(0, std::memory_order_relaxed);
}

auto work_affinity_tracker::operator=(work_affinity_tracker&& other) noexcept
	-> work_affinity_tracker&
{
	if (this != &other)
	{
		worker_count_ = other.worker_count_;
		history_size_ = other.history_size_;
		matrix_size_ = other.matrix_size_;
		cooperation_matrix_ = std::move(other.cooperation_matrix_);
		total_cooperations_.store(
			other.total_cooperations_.load(std::memory_order_relaxed),
			std::memory_order_relaxed);

		other.worker_count_ = 0;
		other.history_size_ = 0;
		other.matrix_size_ = 0;
		other.total_cooperations_.store(0, std::memory_order_relaxed);
	}
	return *this;
}

void work_affinity_tracker::record_cooperation(std::size_t thief_id,
                                               std::size_t victim_id)
{
	if (thief_id >= worker_count_ || victim_id >= worker_count_ ||
	    thief_id == victim_id || !cooperation_matrix_)
	{
		return;
	}

	auto idx = get_matrix_index(thief_id, victim_id);
	if (idx < matrix_size_)
	{
		cooperation_matrix_[idx].fetch_add(1, std::memory_order_relaxed);
		total_cooperations_.fetch_add(1, std::memory_order_relaxed);
	}
}

auto work_affinity_tracker::get_affinity(std::size_t worker_a,
                                         std::size_t worker_b) const -> double
{
	if (worker_a >= worker_count_ || worker_b >= worker_count_ ||
	    worker_a == worker_b || !cooperation_matrix_)
	{
		return 0.0;
	}

	auto idx = get_matrix_index(worker_a, worker_b);
	if (idx >= matrix_size_)
	{
		return 0.0;
	}

	auto cooperation_count = cooperation_matrix_[idx].load(std::memory_order_relaxed);
	if (cooperation_count == 0)
	{
		return 0.0;
	}

	// Normalize by history size to make scores comparable
	return static_cast<double>(cooperation_count) / static_cast<double>(history_size_);
}

auto work_affinity_tracker::get_preferred_victims(std::size_t worker_id,
                                                  std::size_t max_count) const
	-> std::vector<std::size_t>
{
	if (worker_id >= worker_count_ || max_count == 0)
	{
		return {};
	}

	// Build list of (affinity, worker_id) pairs for all other workers
	std::vector<std::pair<double, std::size_t>> affinities;
	affinities.reserve(worker_count_ - 1);

	for (std::size_t i = 0; i < worker_count_; ++i)
	{
		if (i != worker_id)
		{
			double affinity = get_affinity(worker_id, i);
			affinities.emplace_back(affinity, i);
		}
	}

	// Sort by descending affinity
	std::sort(affinities.begin(), affinities.end(),
	          [](const auto& lhs, const auto& rhs) {
		          return lhs.first > rhs.first;
	          });

	// Extract worker IDs up to max_count
	std::vector<std::size_t> result;
	result.reserve(std::min(max_count, affinities.size()));

	for (std::size_t i = 0; i < std::min(max_count, affinities.size()); ++i)
	{
		result.push_back(affinities[i].second);
	}

	return result;
}

void work_affinity_tracker::reset()
{
	if (cooperation_matrix_)
	{
		for (std::size_t i = 0; i < matrix_size_; ++i)
		{
			cooperation_matrix_[i].store(0, std::memory_order_relaxed);
		}
	}
	total_cooperations_.store(0, std::memory_order_relaxed);
}

auto work_affinity_tracker::worker_count() const -> std::size_t
{
	return worker_count_;
}

auto work_affinity_tracker::history_size() const -> std::size_t
{
	return history_size_;
}

auto work_affinity_tracker::total_cooperations() const -> std::uint64_t
{
	return total_cooperations_.load(std::memory_order_relaxed);
}

auto work_affinity_tracker::get_matrix_index(std::size_t worker_a,
                                             std::size_t worker_b) const
	-> std::size_t
{
	auto [i, j] = normalize_pair(worker_a, worker_b);

	// Upper triangular matrix index formula:
	// For pair (i, j) where i < j:
	// index = i * worker_count - i*(i+1)/2 + j - i - 1
	return (i * worker_count_) - ((i * (i + 1)) / 2) + j - i - 1;
}

auto work_affinity_tracker::normalize_pair(std::size_t a, std::size_t b)
	-> std::pair<std::size_t, std::size_t>
{
	if (a < b)
	{
		return {a, b};
	}
	return {b, a};
}

} // namespace kcenon::thread
