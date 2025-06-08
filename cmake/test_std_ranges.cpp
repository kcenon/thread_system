// Test for std::ranges support
#include <ranges>
#include <vector>
#include <algorithm>

int main() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    
    // Test ranges algorithms
    auto even = vec | std::views::filter([](int n) { return n % 2 == 0; });
    auto squared = even | std::views::transform([](int n) { return n * n; });
    
    // Test ranges concepts
    static_assert(std::ranges::range<decltype(vec)>);
    static_assert(std::ranges::forward_range<decltype(vec)>);
    
    // Test ranges algorithms
    std::ranges::sort(vec);
    auto it = std::ranges::find(vec, 3);
    
    return 0;
}