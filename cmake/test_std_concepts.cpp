// BSD 3-Clause License
// Copyright (c) 2024, 🍀☀🌕🌥 🌊
// See the LICENSE file in the project root for full license information.

// Test for std::concepts support
#include <concepts>
#include <type_traits>

// Define a simple custom concept
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

// Simple concept constraint
template<typename T>
concept Addable = requires(T a, T b) { a + b; };

// Function using concept
template<Numeric T>
T add(T a, T b) {
    return a + b;
}

int main() {
    // Test built-in concepts
    static_assert(std::integral<int>);
    static_assert(std::floating_point<double>);

    // Test custom concept
    static_assert(Numeric<int>);
    static_assert(Numeric<double>);
    static_assert(Addable<int>);

    // Use constrained function
    int result = add(1, 2);
    double result2 = add(1.5, 2.5);

    return 0;
}