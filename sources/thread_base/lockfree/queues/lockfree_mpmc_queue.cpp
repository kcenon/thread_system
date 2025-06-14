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

#include "lockfree_mpmc_queue.h"

namespace thread_module
{
	lockfree_mpmc_queue::lockfree_mpmc_queue(size_t max_threads)
		: node_pool_(std::make_unique<node_pool<Node>>())
		, hp_manager_(std::make_unique<hazard_pointer_manager>(max_threads))
	{
		// Initialize with a dummy node
		Node* dummy = allocate_node();
		if (!dummy) {
			throw std::runtime_error("Failed to allocate initial dummy node");
		}
		head_.store(dummy, std::memory_order_relaxed);
		tail_.store(dummy, std::memory_order_relaxed);
	}
	
	lockfree_mpmc_queue::~lockfree_mpmc_queue()
	{
		// Clear all remaining nodes
		clear();
		
		// Delete the dummy node
		Node* dummy = head_.load(std::memory_order_relaxed);
		if (dummy) {
			deallocate_node(dummy);
		}
	}
	
	auto lockfree_mpmc_queue::enqueue(std::unique_ptr<job>&& value) -> result_void
	{
		if (!value) {
			return error{error_code::invalid_argument, "Cannot enqueue null job"};
		}
		
		if (stop_.load(std::memory_order_acquire)) {
			return error{error_code::queue_stopped, "Queue is stopped"};
		}
		
		auto start_time = std::chrono::high_resolution_clock::now();
		
		// Allocate storage for the job
		job_ptr* data_storage = new job_ptr(std::move(value));
		
		try {
			auto result = enqueue_impl(data_storage);
			
			auto duration = std::chrono::high_resolution_clock::now() - start_time;
			record_enqueue_time(std::chrono::duration_cast<std::chrono::nanoseconds>(duration));
			
			return result;
		} catch (...) {
			delete data_storage;
			return error{error_code::unknown_error, "Exception during enqueue"};
		}
	}
	
	auto lockfree_mpmc_queue::enqueue_batch(std::vector<std::unique_ptr<job>>&& jobs) -> result_void
	{
		if (jobs.empty()) {
			return error{error_code::invalid_argument, "Cannot enqueue empty batch"};
		}
		
		if (stop_.load(std::memory_order_acquire)) {
			return error{error_code::queue_stopped, "Queue is stopped"};
		}
		
		if (jobs.size() > MAX_BATCH_SIZE) {
			return error{error_code::invalid_argument, "Batch size exceeds maximum"};
		}
		
		auto start_time = std::chrono::high_resolution_clock::now();
		
		// Prepare nodes and data storage
		std::vector<Node*> nodes;
		std::vector<job_ptr*> data_storage;
		
		try {
			nodes.reserve(jobs.size());
			data_storage.reserve(jobs.size());
			
			// Create linked list of nodes
			for (size_t i = 0; i < jobs.size(); ++i) {
				if (!jobs[i]) {
					throw std::invalid_argument("Null job in batch");
				}
				
				Node* node = allocate_node();
				if (!node) {
					throw std::runtime_error("Failed to allocate node");
				}
				job_ptr* data = new job_ptr(std::move(jobs[i]));
				
				node->set_data(data);
				nodes.push_back(node);
				data_storage.push_back(data);
				
				if (i > 0 && nodes.size() > 1) {
					nodes[i-1]->next.store(node, std::memory_order_release);
				}
			}
			
			// Link the batch into the queue
			Node* first_node = nodes.front();
			Node* last_node = nodes.back();
			size_t retry_count = 0;
			
			while (true) {
				Node* tail = tail_.load(std::memory_order_acquire);
				Node* next = tail->next.load(std::memory_order_acquire);
				
				if (tail == tail_.load(std::memory_order_acquire)) {
					if (next == nullptr) {
						// Try to link the batch
						if (tail->next.compare_exchange_weak(next, first_node,
															  std::memory_order_release,
															  std::memory_order_relaxed)) {
							// Try to advance tail
							tail_.compare_exchange_weak(tail, last_node,
													   std::memory_order_release,
													   std::memory_order_relaxed);
							break;
						}
					} else {
						// Help advance tail
						tail_.compare_exchange_weak(tail, next,
												   std::memory_order_release,
												   std::memory_order_relaxed);
					}
				}
				
				if (++retry_count > RETRY_THRESHOLD) {
					increment_retry_count();
					retry_count = 0;
				}
			}
			
			// Update statistics
			stats_.enqueue_batch_count.fetch_add(1, std::memory_order_relaxed);
			stats_.enqueue_count.fetch_add(jobs.size(), std::memory_order_relaxed);
			stats_.current_size.fetch_add(jobs.size(), std::memory_order_relaxed);
			
			auto duration = std::chrono::high_resolution_clock::now() - start_time;
			record_enqueue_time(std::chrono::duration_cast<std::chrono::nanoseconds>(duration));
			
			return result_void{};
			
		} catch (...) {
			// Clean up on failure
			for (auto* data : data_storage) {
				delete data;
			}
			for (auto* node : nodes) {
				deallocate_node(node);
			}
			return error{error_code::unknown_error, "Exception during batch enqueue"};
		}
	}
	
	auto lockfree_mpmc_queue::dequeue() -> result<std::unique_ptr<job>>
	{
		if (stop_.load(std::memory_order_acquire)) {
			return error{error_code::queue_stopped, "Queue is stopped"};
		}
		
		auto start_time = std::chrono::high_resolution_clock::now();
		
		auto result = dequeue_impl();
		
		auto duration = std::chrono::high_resolution_clock::now() - start_time;
		record_dequeue_time(std::chrono::duration_cast<std::chrono::nanoseconds>(duration));
		
		if (result.has_value()) {
			condition_.notify_one();
		}
		
		return result;
	}
	
	auto lockfree_mpmc_queue::dequeue_batch() -> std::deque<std::unique_ptr<job>>
	{
		std::deque<std::unique_ptr<job>> result;
		
		// Try to dequeue up to MAX_BATCH_SIZE items
		for (size_t i = 0; i < MAX_BATCH_SIZE; ++i) {
			auto job_result = try_dequeue();
			if (!job_result.has_value()) {
				break;
			}
			result.push_back(std::move(job_result.value()));
		}
		
		if (!result.empty()) {
			stats_.dequeue_batch_count.fetch_add(1, std::memory_order_relaxed);
			condition_.notify_all();
		}
		
		return result;
	}
	
	auto lockfree_mpmc_queue::try_enqueue(std::unique_ptr<job>&& value) -> result_void
	{
		return enqueue(std::move(value));
	}
	
	auto lockfree_mpmc_queue::try_dequeue() -> result<std::unique_ptr<job>>
	{
		return dequeue_impl();
	}
	
	auto lockfree_mpmc_queue::clear() -> void
	{
		// Dequeue all items
		while (true) {
			auto result = try_dequeue();
			if (!result.has_value()) {
				break;
			}
		}
		
		stats_.current_size.store(0, std::memory_order_relaxed);
		condition_.notify_all();
	}
	
	auto lockfree_mpmc_queue::empty() const -> bool
	{
		Node* head = head_.load(std::memory_order_acquire);
		Node* tail = tail_.load(std::memory_order_acquire);
		return (head == tail) && (head->next.load(std::memory_order_acquire) == nullptr);
	}
	
	auto lockfree_mpmc_queue::size() const -> std::size_t
	{
		return stats_.current_size.load(std::memory_order_relaxed);
	}
	
	auto lockfree_mpmc_queue::get_statistics() const -> queue_statistics
	{
		queue_statistics stats;
		stats.enqueue_count = stats_.enqueue_count.load();
		stats.dequeue_count = stats_.dequeue_count.load();
		stats.enqueue_batch_count = stats_.enqueue_batch_count.load();
		stats.dequeue_batch_count = stats_.dequeue_batch_count.load();
		stats.total_enqueue_time = stats_.total_enqueue_time.load();
		stats.total_dequeue_time = stats_.total_dequeue_time.load();
		stats.retry_count = stats_.retry_count.load();
		stats.current_size = stats_.current_size.load();
		return stats;
	}
	
	auto lockfree_mpmc_queue::reset_statistics() -> void
	{
		stats_.enqueue_count.store(0, std::memory_order_relaxed);
		stats_.dequeue_count.store(0, std::memory_order_relaxed);
		stats_.enqueue_batch_count.store(0, std::memory_order_relaxed);
		stats_.dequeue_batch_count.store(0, std::memory_order_relaxed);
		stats_.total_enqueue_time.store(0, std::memory_order_relaxed);
		stats_.total_dequeue_time.store(0, std::memory_order_relaxed);
		stats_.retry_count.store(0, std::memory_order_relaxed);
	}
	
	auto lockfree_mpmc_queue::to_string() const -> std::string
	{
		auto stats = get_statistics();
		return formatter::format(
			"lockfree_mpmc_queue[size={}, enqueued={}, dequeued={}, "
			"avg_enqueue_latency={:.1f}ns, avg_dequeue_latency={:.1f}ns, retries={}]",
			stats.current_size,
			stats.enqueue_count,
			stats.dequeue_count,
			stats.get_average_enqueue_latency_ns(),
			stats.get_average_dequeue_latency_ns(),
			stats.retry_count
		);
	}
	
	// Private implementation methods
	
	auto lockfree_mpmc_queue::allocate_node() -> node_ptr
	{
		return node_pool_->allocate();
	}
	
	auto lockfree_mpmc_queue::deallocate_node(node_ptr node) -> void
	{
		if (node) {
			node->clear_data();
			node->next.store(nullptr, std::memory_order_relaxed);
			node->version.fetch_add(1, std::memory_order_relaxed);
			node_pool_->deallocate(node);
		}
	}
	
	auto lockfree_mpmc_queue::retire_node(node_ptr node) -> void
	{
		hp_manager_->retire(node, std::function<void(Node*)>([this](Node* n) { deallocate_node(n); }));
	}
	
	auto lockfree_mpmc_queue::enqueue_impl(job_ptr* data_storage) -> result_void
	{
		Node* new_node = allocate_node();
		if (!new_node) {
			return error{error_code::resource_allocation_failed, "Failed to allocate node"};
		}
		
		size_t retry_count = 0;
		size_t total_retries = 0;
		
		while (total_retries < MAX_TOTAL_RETRIES) {
			Node* tail = tail_.load(std::memory_order_acquire);
			Node* next = tail->next.load(std::memory_order_acquire);
			
			// Check if tail is still valid
			if (tail == tail_.load(std::memory_order_acquire)) {
				if (next == nullptr) {
					// Try to link new node
					if (tail->next.compare_exchange_weak(next, new_node,
														  std::memory_order_release,
														  std::memory_order_relaxed)) {
						// Set data after linking
						new_node->set_data(data_storage);
						
						// Try to advance tail
						tail_.compare_exchange_weak(tail, new_node,
												   std::memory_order_release,
												   std::memory_order_relaxed);
						
						stats_.enqueue_count.fetch_add(1, std::memory_order_relaxed);
						stats_.current_size.fetch_add(1, std::memory_order_relaxed);
						
						return result_void{};
					}
				} else {
					// Help advance tail
					tail_.compare_exchange_weak(tail, next,
											   std::memory_order_release,
											   std::memory_order_relaxed);
				}
			}
			
			if (++retry_count > RETRY_THRESHOLD) {
				increment_retry_count();
				retry_count = 0;
			}
			
			++total_retries;
			
			// Add small delay to reduce contention
			if (total_retries % 100 == 0) {
				std::this_thread::yield();
			}
		}
		
		// If we reach here, we've exceeded max retries
		deallocate_node(new_node);
		return error{error_code::resource_limit_reached, "Enqueue failed after maximum retries"};
	}
	
	auto lockfree_mpmc_queue::dequeue_impl() -> result<job_ptr>
	{
		size_t retry_count = 0;
		size_t total_retries = 0;
		
		while (total_retries < MAX_TOTAL_RETRIES) {
			// Use hazard pointers to protect head access
			auto hp = hp_manager_->acquire();
			Node* head = hp.protect(head_);
			
			if (!head) {
				// This should never happen if constructor succeeded
				return error{error_code::unknown_error, "Head pointer is null"};
			}
			
			Node* tail = tail_.load(std::memory_order_acquire);
			Node* next = head->next.load(std::memory_order_acquire);
			
			// Check if head is still valid
			if (head == head_.load(std::memory_order_acquire)) {
				if (head == tail) {
					if (next == nullptr) {
						// Queue is empty
						return error{error_code::queue_empty, "Queue is empty"};
					}
					
					// Help advance tail
					tail_.compare_exchange_weak(tail, next,
											   std::memory_order_release,
											   std::memory_order_relaxed);
				} else {
					// Read data from next node
					if (next == nullptr) {
						continue; // Retry
					}
					
					job_ptr* data = next->get_data(std::memory_order_acquire);
					
					// Try to advance head
					if (head_.compare_exchange_weak(head, next,
													 std::memory_order_release,
													 std::memory_order_relaxed)) {
						if (data) {
							auto result = std::move(*data);
							delete data;
							
							// Retire the old head node
							retire_node(head);
							
							stats_.dequeue_count.fetch_add(1, std::memory_order_relaxed);
							stats_.current_size.fetch_sub(1, std::memory_order_relaxed);
							
							return result;
						} else {
							// Data was null, retry
							retire_node(head);
							continue;
						}
					}
				}
			}
			
			if (++retry_count > RETRY_THRESHOLD) {
				increment_retry_count();
				retry_count = 0;
			}
			
			++total_retries;
			
			// Add small delay to reduce contention
			if (total_retries % 100 == 0) {
				std::this_thread::yield();
			}
		}
		
		// If we reach here, we've exceeded max retries
		return error{error_code::resource_limit_reached, "Dequeue failed after maximum retries"};
	}
	
	auto lockfree_mpmc_queue::record_enqueue_time(std::chrono::nanoseconds duration) -> void
	{
		stats_.total_enqueue_time.fetch_add(static_cast<uint64_t>(duration.count()), std::memory_order_relaxed);
	}
	
	auto lockfree_mpmc_queue::record_dequeue_time(std::chrono::nanoseconds duration) -> void
	{
		stats_.total_dequeue_time.fetch_add(static_cast<uint64_t>(duration.count()), std::memory_order_relaxed);
	}
	
	auto lockfree_mpmc_queue::increment_retry_count() -> void
	{
		stats_.retry_count.fetch_add(1, std::memory_order_relaxed);
	}

} // namespace thread_module