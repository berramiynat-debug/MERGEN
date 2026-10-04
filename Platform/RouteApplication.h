#pragma once
#include "PlatformManager.h"
#include "Navigation/RouteValidator.h"

// Integration boundary for person 3. Validate the actual path from the current
// aircraft position, including the joining segment, before calling setRoute.
inline bool applyValidatedRoute(PlatformManager& platforms,int id,
    const std::vector<Waypoint>& candidate,const RiskZoneData& zone,
    const RouteValidator& validator) {
    if(candidate.size()<2) return false;
    const auto state=platforms.getState(id);
    std::vector<Waypoint> complete{{state.latitude,state.longitude,state.altitude}};
    complete.insert(complete.end(),candidate.begin(),candidate.end());
    if(!validator.isSafe(complete,zone)) return false;
    platforms.setRoute(id,candidate);
    return true;
}
