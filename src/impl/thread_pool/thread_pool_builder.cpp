/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, kcenon
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

#include <kcenon/thread/core/thread_pool_builder.h>

namespace kcenon::thread
{
	thread_pool_builder::thread_pool_builder(const std::string& name)
		: name_(name)
		, worker_count_(0)
		, context_()
		, enable_diagnostics_(false)
		, enable_enhanced_metrics_(false)
	{
	}

	thread_pool_builder& thread_pool_builder::with_workers(std::size_t count)
	{
		worker_count_ = count;
		return *this;
	}

	thread_pool_builder& thread_pool_builder::with_context(const thread_context& context)
	{
		context_ = context;
		return *this;
	}

	thread_pool_builder& thread_pool_builder::with_queue(std::shared_ptr<job_queue> queue)
	{
		custom_queue_ = std::move(queue);
		return *this;
	}

	thread_pool_builder& thread_pool_builder::with_queue_adapter(
		std::unique_ptr<pool_queue_adapter_interface> adapter)
	{
		queue_adapter_ = std::move(adapter);
		return *this;
	}

	thread_pool_builder& thread_pool_builder::with_circuit_breaker(
		const circuit_breaker_config& config)
	{
		circuit_breaker_config_ = config;
		shared_circuit_breaker_.reset();
		return *this;
	}

	thread_pool_builder& thread_pool_builder::with_circuit_breaker(
		std::shared_ptr<circuit_breaker> cb)
	{
		shared_circuit_breaker_ = std::move(cb);
		circuit_breaker_config_.reset();
		return *this;
	}

	thread_pool_builder& thread_pool_builder::with_autoscaling(
		const autoscaling_policy& config)
	{
		autoscaling_config_ = config;
		return *this;
	}

	thread_pool_builder& thread_pool_builder::with_work_stealing()
	{
		worker_policy config;
		config.enable_work_stealing = true;
		work_stealing_config_ = config;
		return *this;
	}

	thread_pool_builder& thread_pool_builder::with_work_stealing(const worker_policy& config)
	{
		work_stealing_config_ = config;
		return *this;
	}

	thread_pool_builder& thread_pool_builder::with_diagnostics()
	{
		enable_diagnostics_ = true;
		return *this;
	}

	thread_pool_builder& thread_pool_builder::with_enhanced_metrics()
	{
		enable_enhanced_metrics_ = true;
		return *this;
	}

	thread_pool_builder& thread_pool_builder::with_policy(std::unique_ptr<pool_policy> policy)
	{
		policies_.push_back(std::move(policy));
		return *this;
	}

	std::shared_ptr<thread_pool> thread_pool_builder::build()
	{
		std::shared_ptr<thread_pool> pool;

		if (queue_adapter_)
		{
			pool = std::make_shared<thread_pool>(name_, std::move(queue_adapter_), context_);
		}
		else if (custom_queue_)
		{
			pool = std::make_shared<thread_pool>(name_, custom_queue_, context_);
		}
		else
		{
			pool = std::make_shared<thread_pool>(name_, context_);
		}

		std::size_t worker_count = worker_count_;
		if (worker_count == 0)
		{
			worker_count = std::thread::hardware_concurrency();
			if (worker_count == 0)
			{
				worker_count = 4;
			}
		}

		for (std::size_t i = 0; i < worker_count; ++i)
		{
			auto worker = std::make_unique<thread_worker>(true, context_);
			worker->set_job_queue(pool->get_job_queue());
			pool->enqueue(std::move(worker));
		}

		if (circuit_breaker_config_.has_value())
		{
			auto cb_policy = std::make_unique<circuit_breaker_policy>(
				circuit_breaker_config_.value());
			pool->add_policy(std::move(cb_policy));
		}
		else if (shared_circuit_breaker_)
		{
			auto cb_policy = std::make_unique<circuit_breaker_policy>(
				shared_circuit_breaker_);
			pool->add_policy(std::move(cb_policy));
		}

		if (autoscaling_config_.has_value())
		{
			auto as_policy = std::make_unique<autoscaling_pool_policy>(
				*pool, autoscaling_config_.value());
			pool->add_policy(std::move(as_policy));
		}

		if (work_stealing_config_.has_value())
		{
			auto ws_policy = std::make_unique<work_stealing_pool_policy>(
				work_stealing_config_.value());
			pool->add_policy(std::move(ws_policy));
		}

		for (auto& policy : policies_)
		{
			pool->add_policy(std::move(policy));
		}

		if (enable_enhanced_metrics_)
		{
			pool->set_enhanced_metrics_enabled(true);
		}

		if (enable_diagnostics_)
		{
			(void)pool->diagnostics();
		}

		reset();

		return pool;
	}

	std::shared_ptr<thread_pool> thread_pool_builder::build_and_start()
	{
		auto pool = build();
		pool->start();
		return pool;
	}

	void thread_pool_builder::reset()
	{
		name_ = "thread_pool";
		worker_count_ = 0;
		context_ = thread_context();
		custom_queue_.reset();
		queue_adapter_.reset();
		policies_.clear();
		enable_diagnostics_ = false;
		enable_enhanced_metrics_ = false;
		circuit_breaker_config_.reset();
		shared_circuit_breaker_.reset();
		autoscaling_config_.reset();
		work_stealing_config_.reset();
	}

} // namespace kcenon::thread
