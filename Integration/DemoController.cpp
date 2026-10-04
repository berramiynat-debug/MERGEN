#include "DemoController.h"
#include "Navigation/RoutePlanner.h"
#include "Navigation/RouteValidator.h"
#include "Platform/RouteApplication.h"
#include <cmath>
#include <stdexcept>

namespace {
bool sameZone(const RiskZoneData& a,const RiskZoneData& b) {
    return a.latitude==b.latitude&&a.longitude==b.longitude&&a.radiusMeters==b.radiusMeters&&a.active==b.active;
}
void validateZone(const RiskZoneData& z) {
    if(!std::isfinite(z.latitude)||!std::isfinite(z.longitude)||!std::isfinite(z.radiusMeters)
        ||std::abs(z.latitude)>85.||std::abs(z.longitude)>180.||z.radiusMeters<=0.||z.radiusMeters>10000.)
        throw std::invalid_argument("Invalid risk zone: finite local coordinates and radius in (0,10000] m required");
}
}
DemoController::DemoController(PlatformManager& p,double margin,double seconds)
    :platforms_(p),margin_(margin),noticeSeconds_(seconds) {
    if(!std::isfinite(margin)||margin<=0.||!std::isfinite(seconds)||seconds<0.)
        throw std::invalid_argument("Invalid controller margin or notice duration");
    reset();
}
void DemoController::reset() {
    records_.clear();
    for(int id:platforms_.getPlatformIds()) records_[id]=Record{};
    riskZone_.deactivate();
}
void DemoController::update(const RiskZoneData& requested) {
    validateZone(requested);
    const bool changed=!sameZone(requested,riskZone_.getData());
    riskZone_.setCenter(requested.latitude,requested.longitude);
    riskZone_.setRadius(requested.radiusMeters);
    if(requested.active) riskZone_.activate(); else riskZone_.deactivate();
    const auto z=riskZone_.getData();
    if(changed) for(auto& [id,r]:records_) { r.attempted=false; r.pendingSince.reset(); r.status.blocked=false; r.status.message.clear(); }
    for(int id:platforms_.getPlatformIds()) {
        auto& r=records_[id];
        const auto route=platforms_.getRoute(id);
        r.status.risk=detector_.checkRoute(route,z);
        if(r.status.risk!=RiskState::VIOLATION) {
            r.pendingSince.reset(); r.status.blocked=false;
            continue;
        }
        const auto revision=platforms_.getRouteRevision(id);
        if(r.attempted&&r.attemptedRevision==revision) continue;
        if(!r.pendingSince) {
            r.pendingSince=platforms_.simulationTime();
            r.status.message=platforms_.getName(id)+": VIOLATION detected";
        }
        const auto state=platforms_.getState(id);
        const Waypoint current{state.latitude,state.longitude,state.altitude};
        const auto target=route.back();
        const RouteValidator validator(margin_);
        RiskZoneData buffered=z; buffered.radiusMeters+=margin_;
        // A route cannot start safely if current position or destination is already
        // in the clearance zone. Hold instead of claiming a safe detour.
        if(detector_.segmentIntersectsZone(current,current,buffered)
            ||detector_.segmentIntersectsZone(target,target,buffered)) {
            r.attempted=true; r.attemptedRevision=revision; r.status.blocked=true;
            r.status.message=platforms_.getName(id)+": current position or target inside safety boundary; paused";
            continue;
        }
        if(platforms_.simulationTime()-*r.pendingSince<noticeSeconds_) continue;
        r.attempted=true; r.attemptedRevision=revision;
        r.status.blocked=true;
        // Bounded retries call Berra's unchanged planner with increasing clearance.
        // Every returned segment is independently checked by both supplied modules.
        for(unsigned attempt=0;attempt<5;++attempt) {
            ++r.status.planningAttempts;
            const RoutePlanner planner(margin_*std::pow(2.,attempt));
            auto candidate=planner.calculateDetour(current,target,z);
            if(candidate.size()<2) continue;
            std::vector<Waypoint> complete{current};
            complete.insert(complete.end(),candidate.begin(),candidate.end());
            if(detector_.checkRoute(complete,buffered)!=RiskState::SAFE) continue;
            if(!applyValidatedRoute(platforms_,id,candidate,z,validator)) continue;
            r.status.blocked=false; r.status.rerouted=true; ++r.status.applications;
            r.attemptedRevision=platforms_.getRouteRevision(id);
            r.status.message=platforms_.getName(id)+": VIOLATION -> REROUTED (validated "
                +std::to_string(static_cast<int>(margin_))+" m clearance)";
            break;
        }
        if(r.status.blocked) r.status.message=platforms_.getName(id)+": no validated safe route; paused";
    }
}
const DemoController::Status& DemoController::status(int id) const { return records_.at(id).status; }
RiskZoneData DemoController::zone() const { return riskZone_.getData(); }
double DemoController::safetyMargin() const { return margin_; }
bool DemoController::hasBlockedRoute() const { for(const auto& [id,r]:records_) if(r.status.blocked) return true; return false; }
