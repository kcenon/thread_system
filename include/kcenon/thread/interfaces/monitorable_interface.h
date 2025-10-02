/*
 * BSD 3-Clause License
 * Copyright (c) 2025, DongCheol Shin
 */

#pragma once

#include "monitoring_interface.h"

#ifdef BUILD_WITH_COMMON_SYSTEM
#include <kcenon/common/patterns/result.h>
#endif

namespace kcenon::thread {

/**
 * @brief Interface for components exposing metrics.
 */
class monitorable_interface {
public:
    virtual ~monitorable_interface() = default;

    /** @brief Fetch current metrics snapshot. */
    virtual auto get_metrics() -> ::monitoring_interface::metrics_snapshot = 0;

    /**
     * @brief Reset internal metrics counters.
     * @return VoidResult indicating success or error
     *
     * @note Can fail if metrics system state cannot be reset
     */
#ifdef BUILD_WITH_COMMON_SYSTEM
    virtual auto reset_metrics() -> common::VoidResult = 0;
#else
    virtual void reset_metrics() = 0;
#endif
};

} // namespace kcenon::thread

