#include <logit.hpp>

#include <iostream>

int main() {
    LOGIT_ADD_WINDOWS_DEBUG(true);
    LOGIT_INFO("windows debug macro compile test");
    LOGIT_SHUTDOWN();
    std::cout << "PASS: windows_debug_macro_compile" << std::endl;
    return 0;
}
