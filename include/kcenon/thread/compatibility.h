#pragma once

// Forward declare canonical namespaces to allow aliasing without including
// heavier headers.
namespace kcenon {
namespace thread {
namespace core {}
namespace interfaces {}
namespace impl {}
namespace utils {}
namespace detail {}
} // namespace thread
} // namespace kcenon

// Legacy namespace aliases. These remain until dependent projects
// migrate to the unified kcenon::thread namespace.
namespace thread_system = kcenon::thread;
namespace thread_module = kcenon::thread;
namespace thread_namespace = kcenon::thread;

// Legacy utility namespace names still expected by some consumers.
namespace utility_module = kcenon::thread::utils;
