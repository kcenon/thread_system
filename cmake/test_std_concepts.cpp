// Test for std::concepts support
#include <concepts>
#include <type_traits>

// Define a custom concept
template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

// Function using concept
template<Numeric T>
T add(T a, T b) {
    return a + b;
}

// Class template with concept constraint
template<typename T>
    requires std::copyable<T> && std::default_initializable<T>
class Container {
    T value;
public:
    Container() = default;
    Container(T v) : value(v) {}
};

int main() {
    // Test built-in concepts
    static_assert(std::integral<int>);
    static_assert(std::floating_point<double>);
    static_assert(std::copyable<int>);
    static_assert(std::movable<std::string>);
    
    // Test custom concept
    static_assert(Numeric<int>);
    static_assert(Numeric<double>);
    static_assert(!Numeric<std::string>);
    
    // Use constrained function
    auto result = add(1, 2);
    auto result2 = add(1.5, 2.5);
    
    // Use constrained class
    Container<int> c1;
    Container<double> c2(3.14);
    
    return 0;
}