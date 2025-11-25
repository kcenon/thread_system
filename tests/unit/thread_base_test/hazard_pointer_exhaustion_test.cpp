// IMPORTANT: This test uses the legacy hazard_pointer which has memory ordering issues.
// This macro enables the code for testing purposes only.
// For production code, use safe_hazard_pointer.h or atomic_shared_ptr.h instead.
// See TICKET-002 for details.
#define HAZARD_POINTER_FORCE_ENABLE

#include <gtest/gtest.h>
#include <kcenon/thread/core/hazard_pointer.h>
#include <vector>
#include <stdexcept>

namespace kcenon::thread {

TEST(HazardPointerExhaustionTest, ThrowsOnExhaustion) {
    std::vector<hazard_pointer> pointers;
    // Get the limit
    constexpr size_t limit = detail::thread_hazard_list::MAX_HAZARDS_PER_THREAD;

    // Acquire all available slots
    for (size_t i = 0; i < limit; ++i) {
        ASSERT_NO_THROW({
            pointers.push_back(hazard_pointer());
        });
    }

    // Verify that we actually have 'limit' active pointers
    // (This step implicitly checks if the previous loop succeeded)
    ASSERT_EQ(pointers.size(), limit);

    // Try to acquire one more, expecting exception
    EXPECT_THROW({
        hazard_pointer hp;
    }, std::runtime_error);
}

} // namespace kcenon::thread
