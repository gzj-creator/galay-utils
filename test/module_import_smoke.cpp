import galay.utils;

#include <cassert>
#include <iostream>

int main() {
    using namespace galay::utils;

    auto parts = StringUtils::split("a,b,c", ',');
    assert(parts.size() == 3);
    assert(parts[1] == "b");
    assert(StringUtils::join(parts, "-") == "a-b-c");
    assert(System::cpuCount() > 0);

    std::cout << "Module import smoke test passed." << std::endl;
    return 0;
}
