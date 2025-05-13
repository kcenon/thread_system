#pragma once

/**
 * @file cancellation_token.h
 * @brief Implementation of a cancellation token for cooperative cancellation
 */

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace thread_module {

/**
 * @class cancellation_token
 * @brief Provides a mechanism for cooperative cancellation of operations
 *
 * Cancellation tokens allow long-running operations to be gracefully canceled.
 * They are particularly useful for worker threads that need to be notified
 * when their work should be aborted.
 */
class cancellation_token {
private:
    // Private implementation to allow copying/moving the token
    struct token_state {
        std::atomic<bool> is_cancelled{false};
        std::vector<std::function<void()>> callbacks;
        std::mutex callback_mutex;
    };
    
    std::shared_ptr<token_state> state_;
    
    // Private constructor that takes a state
    explicit cancellation_token(std::shared_ptr<token_state> state)
        : state_(std::move(state)) {}
    
public:
    // Default constructor creates a new token
    cancellation_token() : state_(std::make_shared<token_state>()) {}
    
    // Allow copy/move operations
    cancellation_token(const cancellation_token&) = default;
    cancellation_token& operator=(const cancellation_token&) = default;
    cancellation_token(cancellation_token&&) = default;
    cancellation_token& operator=(cancellation_token&&) = default;
    
    /**
     * @brief Creates a new cancellation token
     * @return A new cancellation token
     */
    static cancellation_token create() {
        return cancellation_token();
    }
    
    /**
     * @brief Creates a linked token that is canceled when any of the parent tokens are canceled
     * @param tokens The parent tokens
     * @return A new token linked to the parents
     */
    static cancellation_token create_linked(std::initializer_list<cancellation_token> tokens) {
        auto new_token = create();
        auto new_token_ptr = std::make_shared<cancellation_token>(new_token);
        
        for (const auto& token : tokens) {
            // Use a shared_ptr to capture a mutable copy
            auto token_copy = token;
            token_copy.register_callback([token_ptr = new_token_ptr]() {
                token_ptr->cancel();
            });
        }
        
        return new_token;
    }
    
    /**
     * @brief Cancels the operation
     *
     * Sets the token to the canceled state and invokes all registered callbacks.
     */
    void cancel() {
        bool was_cancelled = state_->is_cancelled.exchange(true);
        if (!was_cancelled) {
            std::lock_guard<std::mutex> lock(state_->callback_mutex);
            for (const auto& callback : state_->callbacks) {
                callback();
            }
        }
    }
    
    /**
     * @brief Checks if the token has been canceled
     * @return true if the token has been canceled, false otherwise
     */
    [[nodiscard]] bool is_cancelled() const {
        return state_->is_cancelled.load();
    }
    
    /**
     * @brief Throws an exception if the token has been canceled
     * @throws std::runtime_error if the token has been canceled
     */
    void throw_if_cancelled() const {
        if (is_cancelled()) {
            throw std::runtime_error("Operation cancelled");
        }
    }
    
    /**
     * @brief Registers a callback to be invoked when the token is canceled
     * @param callback The function to call when the token is canceled
     *
     * If the token is already canceled, the callback is invoked immediately.
     */
    void register_callback(std::function<void()> callback) {
        if (is_cancelled()) {
            callback();
            return;
        }
        
        {
            std::lock_guard<std::mutex> lock(state_->callback_mutex);
            state_->callbacks.push_back(std::move(callback));
        }
        
        // Check again in case the token was canceled between the check and adding callback
        if (is_cancelled()) {
            callback();
        }
    }
};

} // namespace thread_module