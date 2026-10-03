#include <iostream>
#include <cassert>
#include "../Navigation/RouteValidator.h"

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << " Running Kişi 3 - RouteValidator Unit Test" << std::endl;
    std::cout << "==========================================" << std::endl;

    RiskZoneData zone{38.45, 27.20, 1500.0, true};
    RouteValidator validator;

    // Route 1: Safe route passing far away
    std::vector<Waypoint> safeRoute = {
        {38.40, 27.00, 1000.0},
        {38.50, 27.00, 1000.0}
    };
    bool test1 = validator.isSafe(safeRoute, zone);
    std::cout << "[Test 1] Safe route check: " << (test1 ? "PASSED" : "FAILED") << std::endl;
    assert(test1 == true);

    // Route 2: Unsafe route passing right through center
    std::vector<Waypoint> unsafeRoute = {
        {38.40, 27.10, 1000.0},
        {38.50, 27.30, 1000.0}
    };
    bool test2 = validator.isSafe(unsafeRoute, zone);
    std::cout << "[Test 2] Unsafe route check: " << (!test2 ? "PASSED" : "FAILED") << std::endl;
    assert(test2 == false);

    std::cout << "\n>>> ALL ROUTE VALIDATOR TESTS PASSED SUCCESSFULLY! <<<\n" << std::endl;
    return 0;
}
