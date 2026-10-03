#pragma once
#include <vector>
#include "../CommonTypes.h"

class RouteValidator {
public:
    RouteValidator(double safetyMarginMeters = 0.0);

    bool isSafe(const std::vector<Waypoint>& route,
                const RiskZoneData& zone) const;

private:
    double safetyMarginMeters_;
    static double distanceToSegmentMeters(const Waypoint& p, const Waypoint& segStart, const Waypoint& segEnd);
};
