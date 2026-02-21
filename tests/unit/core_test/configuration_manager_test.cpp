/*****************************************************************************
BSD 3-Clause License

Copyright (c) 2025, kcenon
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

/**
 * @file configuration_manager_test.cpp
 * @brief Unit tests for configuration_manager
 * @since 4.0.0
 */

#include <gtest/gtest.h>

#include <kcenon/thread/core/configuration_manager.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

using namespace kcenon::thread;
using namespace std::chrono_literals;

// =============================================================================
// Test fixture with temporary directory
// =============================================================================

class ConfigurationManagerTest : public ::testing::Test {
protected:
	void SetUp() override {
		tmp_dir_ = std::filesystem::temp_directory_path() / "config_manager_test";
		std::filesystem::create_directories(tmp_dir_);
		mgr_ = std::make_unique<configuration_manager>();
	}

	void TearDown() override {
		mgr_.reset();
		std::filesystem::remove_all(tmp_dir_);
	}

	std::filesystem::path write_config_file(const std::string& filename,
	                                        const std::string& content) {
		auto path = tmp_dir_ / filename;
		std::ofstream file(path);
		file << content;
		return path;
	}

	std::filesystem::path tmp_dir_;
	std::unique_ptr<configuration_manager> mgr_;
};

// =============================================================================
// load_from_file tests
// =============================================================================

TEST_F(ConfigurationManagerTest, LoadFromValidFile) {
	auto path = write_config_file("valid.conf",
	    "enabled=true\n"
	    "max_threads=8\n"
	    "name=test_pool\n");

	EXPECT_TRUE(mgr_->load_from_file(path));
	EXPECT_TRUE(mgr_->has("enabled"));
	EXPECT_TRUE(mgr_->has("max_threads"));
	EXPECT_TRUE(mgr_->has("name"));
}

TEST_F(ConfigurationManagerTest, LoadFromNonexistentFile) {
	EXPECT_FALSE(mgr_->load_from_file(tmp_dir_ / "nonexistent.conf"));
}

TEST_F(ConfigurationManagerTest, LoadSkipsCommentsAndBlankLines) {
	auto path = write_config_file("comments.conf",
	    "# This is a comment\n"
	    "; This is also a comment\n"
	    "\n"
	    "key=value\n");

	EXPECT_TRUE(mgr_->load_from_file(path));
	EXPECT_TRUE(mgr_->has("key"));
	EXPECT_EQ(mgr_->get<std::string>("key"), "value");
}

TEST_F(ConfigurationManagerTest, LoadTrimsWhitespace) {
	auto path = write_config_file("whitespace.conf",
	    "  spaced_key  =  spaced_value  \n");

	EXPECT_TRUE(mgr_->load_from_file(path));
	EXPECT_TRUE(mgr_->has("spaced_key"));
	EXPECT_EQ(mgr_->get<std::string>("spaced_key"), "spaced_value");
}

// =============================================================================
// save_to_file and round-trip tests
// =============================================================================

TEST_F(ConfigurationManagerTest, SaveToFile) {
	mgr_->set("enabled", true);
	mgr_->set("count", 42);
	mgr_->set("label", std::string("hello"));

	auto path = tmp_dir_ / "output.conf";
	EXPECT_TRUE(mgr_->save_to_file(path));
	EXPECT_TRUE(std::filesystem::exists(path));
}

TEST_F(ConfigurationManagerTest, RoundTrip) {
	mgr_->set("flag", true);
	mgr_->set("threads", 4);
	mgr_->set("name", std::string("pool_a"));

	auto path = tmp_dir_ / "roundtrip.conf";
	ASSERT_TRUE(mgr_->save_to_file(path));

	configuration_manager loaded;
	ASSERT_TRUE(loaded.load_from_file(path));

	EXPECT_EQ(loaded.get<bool>("flag"), true);
	EXPECT_EQ(loaded.get<int>("threads"), 4);
	EXPECT_EQ(loaded.get<std::string>("name"), "pool_a");
}

// =============================================================================
// set / get<T> for all variant types
// =============================================================================

TEST_F(ConfigurationManagerTest, SetGetBool) {
	EXPECT_TRUE(mgr_->set("flag", true));
	EXPECT_EQ(mgr_->get<bool>("flag"), true);

	EXPECT_TRUE(mgr_->set("flag", false));
	EXPECT_EQ(mgr_->get<bool>("flag"), false);
}

TEST_F(ConfigurationManagerTest, SetGetInt) {
	EXPECT_TRUE(mgr_->set("count", 42));
	EXPECT_EQ(mgr_->get<int>("count"), 42);
}

TEST_F(ConfigurationManagerTest, SetGetDouble) {
	EXPECT_TRUE(mgr_->set("ratio", 3.14));
	EXPECT_DOUBLE_EQ(mgr_->get<double>("ratio"), 3.14);
}

TEST_F(ConfigurationManagerTest, SetGetString) {
	EXPECT_TRUE(mgr_->set("name", std::string("test")));
	EXPECT_EQ(mgr_->get<std::string>("name"), "test");
}

TEST_F(ConfigurationManagerTest, SetGetStringVector) {
	std::vector<std::string> tags = {"a", "b", "c"};
	EXPECT_TRUE(mgr_->set("tags", tags));

	auto result = mgr_->get<std::vector<std::string>>("tags");
	EXPECT_EQ(result, tags);
}

TEST_F(ConfigurationManagerTest, SetGetMap) {
	std::unordered_map<std::string, std::any> data;
	data["key"] = std::string("value");
	EXPECT_TRUE(mgr_->set("metadata", data));

	auto result = mgr_->get<std::unordered_map<std::string, std::any>>("metadata");
	EXPECT_EQ(result.size(), 1u);
}

// =============================================================================
// get<T> type mismatch returns default
// =============================================================================

TEST_F(ConfigurationManagerTest, TypeMismatchReturnsDefault) {
	mgr_->set("count", 42);
	// Requesting as string when stored as int returns default
	EXPECT_EQ(mgr_->get<std::string>("count", "fallback"), "fallback");
}

TEST_F(ConfigurationManagerTest, MissingKeyReturnsDefault) {
	EXPECT_EQ(mgr_->get<int>("nonexistent", 99), 99);
	EXPECT_EQ(mgr_->get<std::string>("nonexistent", "default"), "default");
}

// =============================================================================
// get_optional<T>
// =============================================================================

TEST_F(ConfigurationManagerTest, GetOptionalReturnsValueWhenPresent) {
	mgr_->set("threads", 8);
	auto result = mgr_->get_optional<int>("threads");
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(*result, 8);
}

TEST_F(ConfigurationManagerTest, GetOptionalReturnsNulloptForMissing) {
	auto result = mgr_->get_optional<int>("nonexistent");
	EXPECT_FALSE(result.has_value());
}

TEST_F(ConfigurationManagerTest, GetOptionalReturnsNulloptForTypeMismatch) {
	mgr_->set("count", 42);
	auto result = mgr_->get_optional<std::string>("count");
	EXPECT_FALSE(result.has_value());
}

// =============================================================================
// has / remove
// =============================================================================

TEST_F(ConfigurationManagerTest, HasReturnsFalseForMissing) {
	EXPECT_FALSE(mgr_->has("nonexistent"));
}

TEST_F(ConfigurationManagerTest, HasReturnsTrueAfterSet) {
	mgr_->set("key", std::string("value"));
	EXPECT_TRUE(mgr_->has("key"));
}

TEST_F(ConfigurationManagerTest, RemoveDeletesKey) {
	mgr_->set("key", std::string("value"));
	EXPECT_TRUE(mgr_->remove("key"));
	EXPECT_FALSE(mgr_->has("key"));
}

TEST_F(ConfigurationManagerTest, RemoveReturnsFalseForMissing) {
	EXPECT_FALSE(mgr_->remove("nonexistent"));
}

// =============================================================================
// on_change / remove_callback
// =============================================================================

TEST_F(ConfigurationManagerTest, OnChangeCallbackInvoked) {
	std::string notified_path;
	bool callback_fired = false;

	mgr_->on_change("pool.size", [&](const std::string& path, const config_value&) {
		notified_path = path;
		callback_fired = true;
	});

	mgr_->set("pool.size", 8);
	EXPECT_TRUE(callback_fired);
	EXPECT_EQ(notified_path, "pool.size");
}

TEST_F(ConfigurationManagerTest, GlobalCallbackReceivesAllChanges) {
	int call_count = 0;
	mgr_->on_change("", [&](const std::string&, const config_value&) {
		++call_count;
	});

	mgr_->set("a", 1);
	mgr_->set("b", 2);
	mgr_->set("c", 3);
	EXPECT_EQ(call_count, 3);
}

TEST_F(ConfigurationManagerTest, RemoveCallbackStopsNotification) {
	int call_count = 0;
	auto id = mgr_->on_change("key", [&](const std::string&, const config_value&) {
		++call_count;
	});

	mgr_->set("key", 1);
	EXPECT_EQ(call_count, 1);

	mgr_->remove_callback("key", id);
	mgr_->set("key", 2);
	EXPECT_EQ(call_count, 1);
}

// =============================================================================
// add_validator / validate_all
// =============================================================================

TEST_F(ConfigurationManagerTest, ValidatorRejectsInvalidSet) {
	mgr_->add_validator("pool.size",
	    [](const std::string&, const config_value& value) -> validation_result {
		    validation_result result;
		    try {
			    auto v = std::get<int>(value);
			    if (v <= 0) {
				    result.add_error("pool size must be positive");
			    }
		    } catch (const std::bad_variant_access&) {
			    result.add_error("pool size must be an integer");
		    }
		    return result;
	    });

	EXPECT_FALSE(mgr_->set("pool.size", -1));
	EXPECT_FALSE(mgr_->has("pool.size"));

	EXPECT_TRUE(mgr_->set("pool.size", 4));
	EXPECT_EQ(mgr_->get<int>("pool.size"), 4);
}

TEST_F(ConfigurationManagerTest, ValidateAllReportsErrors) {
	// Store value first, then add validator so validate_all catches it
	mgr_->set("threads", 200);

	mgr_->add_validator("threads",
	    [](const std::string&, const config_value& value) -> validation_result {
		    validation_result result;
		    if (std::get<int>(value) > 100) {
			    result.add_error("too many threads");
		    }
		    return result;
	    });

	auto result = mgr_->validate_all();
	EXPECT_FALSE(result.is_valid);
	EXPECT_FALSE(result.errors.empty());
}

TEST_F(ConfigurationManagerTest, ValidateAllPassesWhenValid) {
	mgr_->add_validator("threads",
	    [](const std::string&, const config_value& value) -> validation_result {
		    validation_result result;
		    if (std::get<int>(value) > 100) {
			    result.add_error("too many threads");
		    }
		    return result;
	    });

	mgr_->set("threads", 8);
	auto result = mgr_->validate_all();
	EXPECT_TRUE(result.is_valid);
	EXPECT_TRUE(result.errors.empty());
}

// =============================================================================
// apply_system_config / get_system_config
// =============================================================================

TEST_F(ConfigurationManagerTest, ApplyAndGetSystemConfig) {
	std::unordered_map<std::string, config_value> config;
	config["pool_size"] = 4;
	config["enabled"] = true;

	mgr_->apply_system_config("thread_system", config);

	EXPECT_EQ(mgr_->get<int>("thread_system.pool_size"), 4);
	EXPECT_EQ(mgr_->get<bool>("thread_system.enabled"), true);

	auto retrieved = mgr_->get_system_config("thread_system");
	EXPECT_EQ(retrieved.size(), 2u);
	EXPECT_EQ(std::get<int>(retrieved["pool_size"]), 4);
	EXPECT_EQ(std::get<bool>(retrieved["enabled"]), true);
}

TEST_F(ConfigurationManagerTest, GetSystemConfigIgnoresOtherPrefixes) {
	mgr_->set("thread_system.size", 4);
	mgr_->set("network.port", 8080);

	auto config = mgr_->get_system_config("thread_system");
	EXPECT_EQ(config.size(), 1u);
	EXPECT_TRUE(config.find("size") != config.end());
}

// =============================================================================
// clear
// =============================================================================

TEST_F(ConfigurationManagerTest, ClearRemovesAllConfig) {
	mgr_->set("a", 1);
	mgr_->set("b", 2);
	EXPECT_TRUE(mgr_->has("a"));

	mgr_->clear();
	EXPECT_FALSE(mgr_->has("a"));
	EXPECT_FALSE(mgr_->has("b"));
}

// =============================================================================
// Event bus integration
// =============================================================================

TEST_F(ConfigurationManagerTest, ConfigChangedEventPublished) {
	auto bus = std::make_shared<event_bus>();
	configuration_manager mgr(bus);

	std::atomic<bool> event_received{false};
	std::string received_path;

	auto sub = bus->subscribe<config_changed_event>(
	    [&](const config_changed_event& evt) {
		    received_path = evt.config_path;
		    event_received.store(true);
	    });

	mgr.set("test.key", 42);

	// Wait for async event delivery
	for (int i = 0; i < 100 && !event_received.load(); ++i) {
		std::this_thread::sleep_for(10ms);
	}

	EXPECT_TRUE(event_received.load());
	EXPECT_EQ(received_path, "test.key");
}

TEST_F(ConfigurationManagerTest, NoEventOnSameValue) {
	auto bus = std::make_shared<event_bus>();
	configuration_manager mgr(bus);

	std::atomic<int> event_count{0};

	auto sub = bus->subscribe<config_changed_event>(
	    [&](const config_changed_event&) {
		    event_count.fetch_add(1);
	    });

	mgr.set("key", 42);

	// Wait for first event
	for (int i = 0; i < 100 && event_count.load() == 0; ++i) {
		std::this_thread::sleep_for(10ms);
	}
	EXPECT_EQ(event_count.load(), 1);

	// Set same value again
	mgr.set("key", 42);

	// Brief wait to ensure no extra event
	std::this_thread::sleep_for(50ms);
	EXPECT_EQ(event_count.load(), 1);
}
