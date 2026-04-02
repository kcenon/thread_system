// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

#include "gtest/gtest.h"

#include <kcenon/thread/stealing/enhanced_steal_policy.h>

using namespace kcenon::thread;

// ============================================
// Enum Value Tests
// ============================================

TEST(enhanced_steal_policy_test, all_policies_have_distinct_values)
{
	EXPECT_NE(static_cast<int>(enhanced_steal_policy::random),
	          static_cast<int>(enhanced_steal_policy::round_robin));
	EXPECT_NE(static_cast<int>(enhanced_steal_policy::round_robin),
	          static_cast<int>(enhanced_steal_policy::adaptive));
	EXPECT_NE(static_cast<int>(enhanced_steal_policy::adaptive),
	          static_cast<int>(enhanced_steal_policy::numa_aware));
	EXPECT_NE(static_cast<int>(enhanced_steal_policy::numa_aware),
	          static_cast<int>(enhanced_steal_policy::locality_aware));
	EXPECT_NE(static_cast<int>(enhanced_steal_policy::locality_aware),
	          static_cast<int>(enhanced_steal_policy::hierarchical));
}

// ============================================
// to_string Tests
// ============================================

TEST(enhanced_steal_policy_test, to_string_random)
{
	EXPECT_STREQ(to_string(enhanced_steal_policy::random), "random");
}

TEST(enhanced_steal_policy_test, to_string_round_robin)
{
	EXPECT_STREQ(to_string(enhanced_steal_policy::round_robin), "round_robin");
}

TEST(enhanced_steal_policy_test, to_string_adaptive)
{
	EXPECT_STREQ(to_string(enhanced_steal_policy::adaptive), "adaptive");
}

TEST(enhanced_steal_policy_test, to_string_numa_aware)
{
	EXPECT_STREQ(to_string(enhanced_steal_policy::numa_aware), "numa_aware");
}

TEST(enhanced_steal_policy_test, to_string_locality_aware)
{
	EXPECT_STREQ(to_string(enhanced_steal_policy::locality_aware), "locality_aware");
}

TEST(enhanced_steal_policy_test, to_string_hierarchical)
{
	EXPECT_STREQ(to_string(enhanced_steal_policy::hierarchical), "hierarchical");
}

TEST(enhanced_steal_policy_test, to_string_unknown)
{
	auto unknown = static_cast<enhanced_steal_policy>(255);
	EXPECT_STREQ(to_string(unknown), "unknown");
}

// ============================================
// Type Size Tests
// ============================================

TEST(enhanced_steal_policy_test, enum_is_uint8)
{
	// Enhanced steal policy should be uint8_t for memory efficiency
	EXPECT_EQ(sizeof(enhanced_steal_policy), sizeof(std::uint8_t));
}
