// BSD 3-Clause License
//
// Copyright (c) 2021-2025, 🍀☀🌕🌥 🌊
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Copyright (c) 2025, 🍀☀🌕🌥 🌊
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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