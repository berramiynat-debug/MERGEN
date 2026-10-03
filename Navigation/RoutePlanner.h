#pragma once
#include <vector>
#include "../CommonTypes.h"

class RoutePlanner {
public:
    RoutePlanner(double safetyMarginMeters = 500.0);

    std::vector<Waypoint> calculateDetour(
        const Waypoint& currentPosition,
        const Waypoint& destination,
        const RiskZoneData& riskZone) const;

private:
    double safetyMarginMeters_;

    // Helper math functions
    static double distanceMeters(const Waypoint& p1, const Waypoint& p2);
    static double distanceToSegmentMeters(const Waypoint& p, const Waypoint& segStart, const Waypoint& segEnd);
};
