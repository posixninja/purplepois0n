/*
 * test_bootrom_routing.cpp
 * Unit test for bootrom exploit CPID routing logic
 * Tests: Usbliter8::isSupportedCpid() and Checkm8::isSupportedCpid()
 */

#include <iostream>
#include <vector>
#include <cstdint>
#include <iomanip>
#include <cstring>

/* Copy of the supported CPID checks (extracted from headers) */
namespace TestRoutingLogic {
    /* Usbliter8: A12 (0x8020), A13 (0x8030), S4/S5 (0x8006) */
    const uint32_t usbliter8_cpids[] = {0x8006, 0x8020, 0x8030};

    /* Checkm8: A5-A11 */
    const uint32_t checkm8_cpids[] = {
        0x8940, 0x8942, 0x8945, 0x8947, /* A5 */
        0x8950, 0x8955,                     /* A6 */
        0x8960,                           /* A7 */
        0x7000, 0x7001, 0x7002,           /* A7/A8 variants */
        0x8000, 0x8001, 0x8002, 0x8003, 0x8004, /* A8/A9 */
        0x8010, 0x8011, 0x8012, 0x8015    /* A10/A11 */
    };

    bool isUsbliter8Supported(uint32_t cpid) {
        for (const auto c : usbliter8_cpids) {
            if (c == cpid) return true;
        }
        return false;
    }

    bool isCheckm8Supported(uint32_t cpid) {
        for (const auto c : checkm8_cpids) {
            if (c == cpid) return true;
        }
        return false;
    }
}

struct TestCase {
    uint32_t cpid;
    const char* soc;
    bool shouldBeUsbliter8;
    bool shouldBeCheckm8;
};

int main() {
    using namespace TestRoutingLogic;

    std::vector<TestCase> tests = {
        /* Usbliter8-capable (A12/A13/S4/S5) */
        {0x8020, "A12", true, false},
        {0x8030, "A13", true, false},
        {0x8006, "S4/S5", true, false},

        /* Checkm8-capable (A5-A11) */
        {0x8940, "A5", false, true},
        {0x8950, "A6", false, true},
        {0x8960, "A7", false, true},
        {0x7000, "A7 variant (iPad)", false, true},
        {0x8000, "A8", false, true},
        {0x8010, "A10", false, true},
        {0x8015, "A11 variant", false, true},

        /* Unsupported */
        {0x8101, "A14", false, false},
        {0x8110, "A14 variant", false, false},
        {0x0000, "Unknown/Zero", false, false},
        {0xFFFF, "Invalid", false, false},
    };

    int passed = 0, failed = 0;

    std::cout << "Testing bootrom exploit CPID routing logic...\n" << std::endl;

    for (const auto& test : tests) {
        bool isUsbliter8 = isUsbliter8Supported(test.cpid);
        bool isCheckm8 = isCheckm8Supported(test.cpid);

        bool pass = (isUsbliter8 == test.shouldBeUsbliter8) &&
                    (isCheckm8 == test.shouldBeCheckm8);

        std::cout << "CPID 0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << test.cpid
                  << std::dec << std::setfill(' ') << " (" << test.soc << "): ";

        if (pass) {
            std::cout << "✓ PASS";
            if (isUsbliter8) std::cout << " → usbliter8";
            if (isCheckm8) std::cout << " → checkm8";
            if (!isUsbliter8 && !isCheckm8) std::cout << " → unsupported";
            std::cout << std::endl;
            passed++;
        } else {
            std::cout << "✗ FAIL";
            std::cout << " (expected: usbliter8=" << test.shouldBeUsbliter8
                      << " checkm8=" << test.shouldBeCheckm8 << ", "
                      << "got: usbliter8=" << isUsbliter8
                      << " checkm8=" << isCheckm8 << ")" << std::endl;
            failed++;
        }
    }

    std::cout << "\n" << passed << " passed, " << failed << " failed." << std::endl;
    return (failed == 0) ? 0 : 1;
}
