#pragma once

#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

namespace monitoring_module {

    // Lock-free 링 버퍼 구현 (SPSC - Single Producer Single Consumer)
    template<typename T>
    class ring_buffer {
    public:
        explicit ring_buffer(std::size_t capacity)
            : capacity_(capacity + 1)  // 빈 슬롯 하나 예약 (full/empty 구분용)
            , buffer_(capacity_)
            , head_(0)
            , tail_(0) {
        }

        // 데이터 추가 (Producer)
        auto push(const T& item) -> bool {
            const auto current_tail = tail_.load(std::memory_order_relaxed);
            const auto next_tail = (current_tail + 1) % capacity_;

            if (next_tail == head_.load(std::memory_order_acquire)) {
                return false; // 버퍼가 가득 참
            }

            buffer_[current_tail] = item;
            tail_.store(next_tail, std::memory_order_release);
            return true;
        }

        // 데이터 제거 (Consumer)
        auto pop(T& item) -> bool {
            const auto current_head = head_.load(std::memory_order_relaxed);

            if (current_head == tail_.load(std::memory_order_acquire)) {
                return false; // 버퍼가 비어 있음
            }

            item = buffer_[current_head];
            head_.store((current_head + 1) % capacity_, std::memory_order_release);
            return true;
        }

        // 현재 크기
        auto size() const -> std::size_t {
            const auto current_tail = tail_.load(std::memory_order_acquire);
            const auto current_head = head_.load(std::memory_order_acquire);

            if (current_tail >= current_head) {
                return current_tail - current_head;
            } else {
                return capacity_ - current_head + current_tail;
            }
        }

        // 비어있는지 확인
        auto empty() const -> bool {
            return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
        }

        // 가득 찼는지 확인
        auto full() const -> bool {
            const auto current_tail = tail_.load(std::memory_order_acquire);
            const auto next_tail = (current_tail + 1) % capacity_;
            return next_tail == head_.load(std::memory_order_acquire);
        }

        // 용량
        auto capacity() const -> std::size_t {
            return capacity_ - 1;  // 실제 사용 가능한 크기
        }

        // 최근 N개 항목 가져오기 (thread-safe)
        auto get_recent_items(std::size_t count) const -> std::vector<T> {
            std::vector<T> result;
            result.reserve(count);

            const auto current_tail = tail_.load(std::memory_order_acquire);
            const auto current_head = head_.load(std::memory_order_acquire);
            const auto current_size = size();

            if (current_size == 0) {
                return result;
            }

            const auto items_to_copy = std::min(count, current_size);
            auto start_index = current_tail;

            // 뒤에서부터 items_to_copy개만큼 가져오기
            if (items_to_copy <= current_tail) {
                start_index = current_tail - items_to_copy;
            } else {
                start_index = capacity_ - (items_to_copy - current_tail);
            }

            for (std::size_t i = 0; i < items_to_copy; ++i) {
                const auto index = (start_index + i) % capacity_;
                result.push_back(buffer_[index]);
            }

            return result;
        }

    private:
        const std::size_t capacity_;
        std::vector<T> buffer_;
        std::atomic<std::size_t> head_;
        std::atomic<std::size_t> tail_;
    };

    // 멀티 프로듀서용 thread-safe 링 버퍼
    template<typename T>
    class thread_safe_ring_buffer {
    public:
        explicit thread_safe_ring_buffer(std::size_t capacity)
            : capacity_(capacity)
            , buffer_(capacity)
            , head_(0)
            , tail_(0) {
        }

        auto push(const T& item) -> bool {
            std::lock_guard<std::mutex> lock(mutex_);

            if ((tail_ + 1) % capacity_ == head_) {
                return false; // 버퍼가 가득 찬 상태
            }

            buffer_[tail_] = item;
            tail_ = (tail_ + 1) % capacity_;
            return true;
        }

        auto pop(T& item) -> bool {
            std::lock_guard<std::mutex> lock(mutex_);

            if (head_ == tail_) {
                return false; // 버퍼가 비어 있음
            }

            item = buffer_[head_];
            head_ = (head_ + 1) % capacity_;
            return true;
        }

        auto size() const -> std::size_t {
            std::lock_guard<std::mutex> lock(mutex_);
            
            if (tail_ >= head_) {
                return tail_ - head_;
            } else {
                return capacity_ - head_ + tail_;
            }
        }

        auto empty() const -> bool {
            std::lock_guard<std::mutex> lock(mutex_);
            return head_ == tail_;
        }

        auto get_all_items() const -> std::vector<T> {
            std::lock_guard<std::mutex> lock(mutex_);
            std::vector<T> result;

            if (head_ == tail_) {
                return result;
            }

            result.reserve(size());

            if (tail_ > head_) {
                for (std::size_t i = head_; i < tail_; ++i) {
                    result.push_back(buffer_[i]);
                }
            } else {
                for (std::size_t i = head_; i < capacity_; ++i) {
                    result.push_back(buffer_[i]);
                }
                for (std::size_t i = 0; i < tail_; ++i) {
                    result.push_back(buffer_[i]);
                }
            }

            return result;
        }

    private:
        const std::size_t capacity_;
        std::vector<T> buffer_;
        std::size_t head_;
        std::size_t tail_;
        mutable std::mutex mutex_;
    };

} // namespace monitoring_module