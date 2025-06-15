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

#include "hazard_pointer.h"
#include <algorithm>
#include <unordered_set>

namespace thread_module
{
	// Thread-local static member definitions for MinGW
	#ifdef __MINGW32__
	thread_local hazard_pointer_manager::HazardRecord* hazard_pointer_manager::local_record_ = nullptr;
	thread_local std::vector<hazard_pointer_manager::RetiredNode> hazard_pointer_manager::retired_list_;
	thread_local std::chrono::steady_clock::time_point hazard_pointer_manager::last_scan_;
	#endif

	hazard_pointer_manager::hazard_pointer_manager(size_t max_threads, size_t pointers_per_thread)
		: max_threads_(max_threads)
		, pointers_per_thread_(pointers_per_thread)
	{
		// Pre-allocate hazard records
		for (size_t i = 0; i < max_threads; ++i) {
			auto* record = new HazardRecord(pointers_per_thread);
			
			// Add to the list
			auto* head = head_record_.load(std::memory_order_acquire);
			do {
				record->next.store(head, std::memory_order_relaxed);
			} while (!head_record_.compare_exchange_weak(head, record,
														  std::memory_order_release,
														  std::memory_order_acquire));
		}
	}
	
	hazard_pointer_manager::~hazard_pointer_manager()
	{
		// Force final reclamation
		scan_and_reclaim();
		
		// Clean up all hazard records
		auto* record = head_record_.load(std::memory_order_acquire);
		while (record) {
			auto* next = record->next.load(std::memory_order_acquire);
			delete record;
			record = next;
		}
	}
	
	// hazard_pointer implementation
	
	hazard_pointer_manager::hazard_pointer::hazard_pointer()
		: hp_slot_(nullptr)
	{
	}
	
	hazard_pointer_manager::hazard_pointer::~hazard_pointer()
	{
		clear();
	}
	
	hazard_pointer_manager::hazard_pointer::hazard_pointer(hazard_pointer&& other) noexcept
		: hp_slot_(other.hp_slot_)
	{
		other.hp_slot_ = nullptr;
	}
	
	auto hazard_pointer_manager::hazard_pointer::operator=(hazard_pointer&& other) noexcept -> hazard_pointer&
	{
		if (this != &other) {
			clear();
			hp_slot_ = other.hp_slot_;
			other.hp_slot_ = nullptr;
		}
		return *this;
	}
	
	// hazard_pointer_manager methods
	
	auto hazard_pointer_manager::acquire() -> hazard_pointer
	{
		auto* slot = acquire_slot();
		return hazard_pointer(slot);
	}
	
	auto hazard_pointer_manager::scan_and_reclaim() -> void
	{
		// Collect all hazard pointers
		auto hazards = collect_hazard_pointers();
		std::sort(hazards.begin(), hazards.end());
		
		// Process retired list
		std::vector<RetiredNode> new_retired_list;
		
		for (auto& retired : retired_list_) {
			if (std::binary_search(hazards.begin(), hazards.end(), retired.ptr)) {
				// Still hazardous, keep in retired list
				new_retired_list.push_back(std::move(retired));
			} else {
				// Safe to reclaim
				retired.deleter(retired.ptr);
				total_reclaimed_.fetch_add(1, std::memory_order_relaxed);
			}
		}
		
		retired_list_ = std::move(new_retired_list);
		last_scan_ = std::chrono::steady_clock::now();
	}
	
	auto hazard_pointer_manager::get_statistics() const -> statistics
	{
		statistics stats;
		
		// Count active hazard pointers
		size_t active_count = 0;
		auto* record = head_record_.load(std::memory_order_acquire);
		while (record) {
			if (record->owner.load(std::memory_order_acquire) != std::thread::id{}) {
				for (const auto& hazard : record->hazards) {
					if (hazard.load(std::memory_order_acquire) != nullptr) {
						++active_count;
					}
				}
			}
			record = record->next.load(std::memory_order_acquire);
		}
		
		stats.active_hazard_pointers = active_count;
		stats.retired_list_size = retired_list_.size();
		stats.total_reclaimed = total_reclaimed_.load(std::memory_order_relaxed);
		stats.total_retired = total_retired_.load(std::memory_order_relaxed);
		
		return stats;
	}
	
	auto hazard_pointer_manager::acquire_record() -> HazardRecord*
	{
		// Try to reuse existing record
		if (local_record_ && local_record_->owner.load(std::memory_order_acquire) == std::this_thread::get_id()) {
			return local_record_;
		}
		
		// Find an unused record
		auto* record = head_record_.load(std::memory_order_acquire);
		while (record) {
			std::thread::id expected{};
			if (record->owner.compare_exchange_strong(expected, std::this_thread::get_id(),
													   std::memory_order_acquire,
													   std::memory_order_relaxed)) {
				local_record_ = record;
				return record;
			}
			record = record->next.load(std::memory_order_acquire);
		}
		
		// No free records available
		throw std::runtime_error("No free hazard records available");
	}
	
	auto hazard_pointer_manager::release_record(HazardRecord* record) -> void
	{
		if (record) {
			// Clear all hazard pointers
			for (auto& hazard : record->hazards) {
				hazard.store(nullptr, std::memory_order_release);
			}
			
			// Release ownership
			record->owner.store(std::thread::id{}, std::memory_order_release);
			
			if (local_record_ == record) {
				local_record_ = nullptr;
			}
		}
	}
	
	auto hazard_pointer_manager::acquire_slot() -> std::atomic<void*>*
	{
		auto* record = acquire_record();
		
		// Find an empty slot
		for (auto& hazard : record->hazards) {
			void* expected = nullptr;
			if (hazard.compare_exchange_strong(expected, reinterpret_cast<void*>(1),
											   std::memory_order_acquire,
											   std::memory_order_relaxed)) {
				hazard.store(nullptr, std::memory_order_release);
				return &hazard;
			}
		}
		
		// No free slots in record
		throw std::runtime_error("No free hazard pointer slots");
	}
	
	auto hazard_pointer_manager::release_slot(std::atomic<void*>* slot) -> void
	{
		if (slot) {
			slot->store(nullptr, std::memory_order_release);
		}
	}
	
	auto hazard_pointer_manager::retire_impl(void* ptr, std::function<void(void*)> deleter) -> void
	{
		retired_list_.emplace_back(ptr, std::move(deleter));
		total_retired_.fetch_add(1, std::memory_order_relaxed);
		
		// Check if we should scan
		if (should_scan()) {
			scan_and_reclaim();
		}
	}
	
	auto hazard_pointer_manager::collect_hazard_pointers() const -> std::vector<void*>
	{
		std::vector<void*> hazards;
		
		auto* record = head_record_.load(std::memory_order_acquire);
		while (record) {
			if (record->owner.load(std::memory_order_acquire) != std::thread::id{}) {
				for (const auto& hazard : record->hazards) {
					void* ptr = hazard.load(std::memory_order_acquire);
					if (ptr && ptr != reinterpret_cast<void*>(1)) {
						hazards.push_back(ptr);
					}
				}
			}
			record = record->next.load(std::memory_order_acquire);
		}
		
		return hazards;
	}
	
	auto hazard_pointer_manager::should_scan() const -> bool
	{
		// Scan if retired list is large
		if (retired_list_.size() >= RETIRED_THRESHOLD) {
			return true;
		}
		
		// Scan if enough time has passed
		auto now = std::chrono::steady_clock::now();
		if (now - last_scan_ >= SCAN_INTERVAL) {
			return true;
		}
		
		return false;
	}

} // namespace thread_module