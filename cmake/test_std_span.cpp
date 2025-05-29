#include <span>
#include <vector>
#include <array>
#include <string>

// Test basic span functionality with const access
bool test_basic_span() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::span<const int> s(vec);
    
    // Basic size check
    if (s.size() != 5) return false;
    
    // Element access
    if (s[0] != 1 || s[4] != 5) return false;
    
    // Range-based for loop
    int sum = 0;
    for (const auto& i : s) {
        sum += i;
    }
    if (sum != 15) return false;
    
    return true;
}

// Test span with C-style array
bool test_array_span() {
    int arr[] = {1, 2, 3, 4, 5};
    std::span<int> s(arr, 5);
    
    // Basic size check
    if (s.size() != 5) return false;
    
    // Modify through span
    s[0] = 10;
    if (arr[0] != 10) return false;
    
    return true;
}

// Test span with std::array
bool test_std_array_span() {
    std::array<int, 5> arr = {1, 2, 3, 4, 5};
    std::span<int> s(arr);
    
    // Basic size check
    if (s.size() != 5) return false;
    
    return true;
}

// Test span with subview capabilities
bool test_subspan() {
    std::vector<int> vec = {1, 2, 3, 4, 5, 6, 7, 8};
    std::span<const int> s(vec);
    
    // Get subspan
    auto sub = s.subspan(2, 3);
    if (sub.size() != 3) return false;
    if (sub[0] != 3 || sub[2] != 5) return false;
    
    return true;
}

int main() {
    bool all_tests_passed = 
        test_basic_span() && 
        test_array_span() && 
        test_std_array_span() && 
        test_subspan();
        
    return all_tests_passed ? 0 : 1;
}