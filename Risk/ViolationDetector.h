#pragma once
#include "CommonTypes.h"
#include <vector>

class ViolationDetector {
public:
    RiskState checkRoute(const std::vector<Waypoint>& route,
                         const RiskZoneData& zone) const;
    bool segmentIntersectsZone(const Waypoint& start,
                               const Waypoint& end,
                               const RiskZoneData& zone) const;
};
