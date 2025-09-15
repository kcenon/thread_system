#ifndef KCENON_THREAD_COMPATIBILITY_H
#define KCENON_THREAD_COMPATIBILITY_H

// Backward compatibility namespace aliases
// These will be removed in future versions

namespace kcenon::thread {
    namespace core {}
    namespace interfaces {}
    namespace impl {}
    namespace utils {}
}

// Old namespace aliases (deprecated)
namespace kcenon::thread = kcenon::thread;
namespace thread_pool_module = kcenon::thread::impl;
namespace typed_thread_pool_module = kcenon::thread::impl;
namespace utility_module = kcenon::thread::utils;
namespace monitoring_interface = kcenon::thread::interfaces;

#endif // KCENON_THREAD_COMPATIBILITY_H
