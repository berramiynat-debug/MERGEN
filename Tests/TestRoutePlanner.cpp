#include <iostream>
#include <cassert>
#include "../Navigation/RoutePlanner.h"
#include "../Navigation/RouteValidator.h"

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << " Running Kişi 3 - RoutePlanner Unit Test " << std::endl;
    std::cout << "==========================================" << std::endl;

    Waypoint start{38.40, 27.10, 1000.0};
    Waypoint target{38.50, 27.30, 1000.0};

    // Scenario 1: Inactive Risk Zone
    RiskZoneData zoneInactive{38.45, 27.20, 1500.0, false};
    RoutePlanner planner(500.0); // 500m safety margin
    auto routeNormal = planner.calculateDetour(start, target, zoneInactive);
    std::cout << "[Test 1] Inactive zone route waypoint count: " << routeNormal.size() << " (Expected: 2)" << std::endl;
    assert(routeNormal.size() == 2);

    // Scenario 2: Active Risk Zone directly on path
    RiskZoneData zoneActive{38.45, 27.20, 1500.0, true};
    auto routeDetour = planner.calculateDetour(start, target, zoneActive);
    std::cout << "[Test 2] Detour route waypoint count: " << routeDetour.size() << " (Expected: 3)" << std::endl;
    assert(routeDetour.size() == 3);

    std::cout << "  Start: (" << routeDetour[0].latitude << ", " << routeDetour[0].longitude << ")" << std::endl;
    std::cout << "  Detour Waypoint: (" << routeDetour[1].latitude << ", " << routeDetour[1].longitude << ")" << std::endl;
    std::cout << "  Target: (" << routeDetour[2].latitude << ", " << routeDetour[2].longitude << ")" << std::endl;

    // Validate detour using RouteValidator
    RouteValidator validator(0.0);
    bool isDetourSafe = validator.isSafe(routeDetour, zoneActive);
    std::cout << "[Test 3] Is detour route safe? " << (isDetourSafe ? "YES (PASSED)" : "NO (FAILED)") << std::endl;
    assert(isDetourSafe);

    // Validate that direct route would fail validator
    bool isDirectSafe = validator.isSafe(routeNormal, zoneActive);
    std::cout << "[Test 4] Is direct route marked unsafe? " << (!isDirectSafe ? "YES (PASSED)" : "NO (FAILED)") << std::endl;
    assert(!isDirectSafe);

    std::cout << "\n>>> ALL ROUTE PLANNER TESTS PASSED SUCCESSFULLY! <<<\n" << std::endl;
    return 0;
}
