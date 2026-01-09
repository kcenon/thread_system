/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2024, 🍀☀🌕🌥 🌊
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

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
