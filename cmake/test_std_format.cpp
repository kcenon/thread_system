// Test file for std::format compatibility
// This file tests real-world usage of std::format with custom formatters

#include <format>
#include <string>
#include <string_view>
#include <memory>

// Test custom type similar to our thread system classes
struct test_thread_pool {
    std::string name;
    bool running;
    
    test_thread_pool(const std::string& n) : name(n), running(false) {}
    
    std::string to_string() const {
        return std::format("[thread_pool: {} ({})]", name, running ? "running" : "stopped");
    }
};

// Test formatter specialization for narrow strings
template<>
struct std::formatter<test_thread_pool> : std::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const test_thread_pool& pool, FormatContext& ctx) const {
        return std::formatter<std::string_view>::format(pool.to_string(), ctx);
    }
};

// Test formatter specialization for wide strings  
template<>
struct std::formatter<test_thread_pool, wchar_t> : std::formatter<std::wstring_view, wchar_t> {
    template<typename FormatContext>
    auto format(const test_thread_pool& pool, FormatContext& ctx) const {
        auto str = pool.to_string();
        std::wstring wstr(str.begin(), str.end());
        return std::formatter<std::wstring_view, wchar_t>::format(wstr, ctx);
    }
};

int main() {
    // Test basic formatting
    std::string basic = std::format("{} + {} = {}", 1, 2, 3);
    
    // Test custom type formatting
    test_thread_pool pool("TestPool");
    std::string custom = std::format("Pool status: {}", pool);
    
    // Test wide string formatting
    std::wstring wide = std::format(L"Wide string: {}", pool);
    
    // Test complex formatting
    auto shared_pool = std::make_shared<test_thread_pool>("SharedPool");
    std::string complex = std::format("Shared pool: {}", *shared_pool);
    
    // Test format with different argument types
    std::string mixed = std::format("Mixed: {} {} {} {}", 
        42, "string", 3.14, pool);
    
    return basic.empty() || custom.empty() || wide.empty() || 
           complex.empty() || mixed.empty() ? 1 : 0;
}