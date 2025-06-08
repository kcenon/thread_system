// Test for std::filesystem support
#include <filesystem>
#include <iostream>

int main() {
    std::filesystem::path p = "/test/path";
    bool exists = std::filesystem::exists(p);
    
    // Test directory iteration
    if (std::filesystem::exists(".")) {
        for (const auto& entry : std::filesystem::directory_iterator(".")) {
            std::cout << entry.path() << std::endl;
        }
    }
    
    // Test path manipulation
    auto absolute = std::filesystem::absolute(p);
    auto filename = p.filename();
    auto extension = p.extension();
    
    return 0;
}