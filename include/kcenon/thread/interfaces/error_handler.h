// BSD 3-Clause License
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

/**
 * @file error_handler.h
 * @brief Error handler interface for customizable error handling strategies.
 *
 */

#pragma once

#include <functional>
#include <string>

#include <kcenon/common/interfaces/logger_interface.h>
#include <kcenon/common/interfaces/global_logger_registry.h>

namespace kcenon::thread {

/**
 * @brief Error handler interface
 *
 * Provides a way to handle errors in the thread system without
 * depending on a specific error handling implementation.
 */
class error_handler {
public:
  using error_callback = std::function<void(const std::string& context,
                                            const std::string& error)>;

  virtual ~error_handler() = default;

  /**
   * @brief Handle an error
   * @param context Context where the error occurred
   * @param error Error message
   */
  virtual void handle_error(const std::string& context,
                            const std::string& error) = 0;

  /**
   * @brief Set a callback for error handling
   * @param callback Callback function to be called on errors
   */
  virtual void set_error_callback(error_callback callback) = 0;
};

/**
 * @brief Default error handler implementation
 *
 * Uses the common_system ILogger interface if available and calls registered callbacks.
 *
 * @note Issue #261: Migrated to use common_system's GlobalLoggerRegistry.
 */
class default_error_handler : public error_handler {
private:
  error_callback callback_;

public:
  void handle_error(const std::string& context,
                    const std::string& error) override {
    // Log the error using GlobalLoggerRegistry
    auto logger = common::interfaces::GlobalLoggerRegistry::instance().get_default_logger();
    if (logger) {
      logger->log(common::interfaces::log_level::error, context + ": " + error);
    }

    // Call the callback if registered
    if (callback_) {
      callback_(context, error);
    }
  }

  void set_error_callback(error_callback callback) override {
    callback_ = callback;
  }
};

} // namespace kcenon::thread