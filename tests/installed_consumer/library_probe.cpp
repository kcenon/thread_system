#include "abi_snapshot.h"

extern "C" unsigned thread_library_features() { return feature_snapshot(); }
extern "C" std::size_t thread_library_base_size()
{
    return sizeof(kcenon::thread::thread_base);
}
extern "C" std::size_t thread_library_lifecycle_size()
{
    return sizeof(kcenon::thread::lifecycle_controller);
}
extern "C" std::size_t thread_library_worker_size()
{
    return sizeof(kcenon::thread::thread_worker);
}
