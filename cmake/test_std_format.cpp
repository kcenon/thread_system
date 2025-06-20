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

#include <format>
#include <string>
#include <string_view>
#include <vector>

// Test basic formatting
std::string test_basic() {
    return std::format("{} {}", "Hello", "World");
}

// Test numeric formatting
std::string test_numeric() {
    return std::format("{:d} {:x} {:o} {:b}", 42, 42, 42, 42);
}

// Test floating point formatting
std::string test_float() {
    return std::format("{:.2f} {:.4e} {:.3g}", 3.14159, 3.14159, 3.14159);
}

// Test width and alignment
std::string test_alignment() {
    return std::format("{:<10} {:^10} {:>10}", "left", "center", "right");
}

// Test custom type formatting
struct Point {
    int x, y;
};

template<>
struct std::formatter<Point> : std::formatter<std::string_view> {
    template<typename FormatContext>
    auto format(const Point& p, FormatContext& ctx) const {
        return std::formatter<std::string_view>::format(
            std::format("({}, {})", p.x, p.y), ctx);
    }
};

std::string test_custom() {
    Point p{10, 20};
    return std::format("Point: {}", p);
}

// Test format string compile-time checking
template<typename... Args>
std::string test_compile_time(std::format_string<Args...> fmt, Args&&... args) {
    return std::format(fmt, std::forward<Args>(args)...);
}

int main() {
    // Test all formatting features
    auto basic = test_basic();
    auto numeric = test_numeric();
    auto floats = test_float();
    auto aligned = test_alignment();
    auto custom = test_custom();
    
    // Test compile-time format string checking
    auto compile_time = test_compile_time("Value: {}", 42);
    
    // Test with containers
    std::vector<int> vec = {1, 2, 3};
    auto container = std::format("First element: {}", vec[0]);
    
    return 0;
}