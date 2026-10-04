#include "Integration/DemoController.h"
#include "Integration/DemoScenario.h"
#include "Platform/RouteApplication.h"
#include "Navigation/RoutePlanner.h"
#include "Navigation/RouteValidator.h"
#include "simData/MemoryDataStore.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
bool near(double a,double b) { return std::abs(a-b)<1e-8; }
void samePosition(const AircraftState& a,const AircraftState& b) {
    check(near(a.latitude,b.latitude)&&near(a.longitude,b.longitude)&&near(a.altitude,b.altitude),"Route switch teleported a platform");
}
void sameRoute(const std::vector<Waypoint>& a,const std::vector<Waypoint>& b) {
    check(a.size()==b.size(),"Unaffected route size changed");
    for(size_t i=0;i<a.size();++i) check(near(a[i].latitude,b[i].latitude)&&near(a[i].longitude,b[i].longitude)&&near(a[i].altitude,b[i].altitude),"Unaffected waypoint changed");
}
}
int main() {
    try {
        simData::MemoryDataStore store;
        PlatformManager p(store);
        createDemoScenario(p);
        DemoController controller(p,500.,0.);
        ViolationDetector detector;
        auto zone=defaultRiskZone();
        controller.update(zone);
        for(int id:p.getPlatformIds()) check(controller.status(id).risk==RiskState::SAFE&&controller.status(id).applications==0,"T01 inactive zone must not replan");
        zone.active=true;
        auto far=zone; far.latitude+=.3;
        controller.update(far);
        for(int id:p.getPlatformIds()) check(controller.status(id).applications==0,"T02 nonintersecting zone must not replan");
        p.update(25.);
        const auto before=p.getState(2);
        const auto first=p.getAssignedRoute(1),third=p.getAssignedRoute(3);
        const auto firstRev=p.getRouteRevision(1),thirdRev=p.getRouteRevision(3);
        check(detector.checkRoute(p.getRoute(2),zone)==RiskState::VIOLATION,"T03 must detect route intersection");
        check(detector.checkRoute(p.getRoute(1),zone)==RiskState::SAFE&&detector.checkRoute(p.getRoute(3),zone)==RiskState::SAFE,"T03 only IHA-2 may intersect default zone");
        const auto initialCandidate=RoutePlanner(500.).calculateDetour({before.latitude,before.longitude,before.altitude},p.getRoute(2).back(),zone);
        const auto beforeRev=p.getRouteRevision(2);
        check(!applyValidatedRoute(p,2,initialCandidate,zone,RouteValidator(500.)),"T04 unsafe first candidate must be rejected");
        check(p.getRouteRevision(2)==beforeRev,"T04 rejected route changed platform");
        samePosition(before,p.getState(2));
        controller.update(zone);
        check(controller.status(2).applications==1&&controller.status(2).planningAttempts>1&&!controller.hasBlockedRoute(),"T05 bounded planner retry did not find a validated route");
        samePosition(before,p.getState(2));
        auto buffered=zone; buffered.radiusMeters+=500.;
        check(detector.checkRoute(p.getRoute(2),buffered)==RiskState::SAFE&&RouteValidator(500.).isSafe(p.getRoute(2),zone),"T05 full remaining route violates 500 m margin");
        check(p.getRouteRevision(1)==firstRev&&p.getRouteRevision(3)==thirdRev,"T08 other platforms were replanned");
        sameRoute(first,p.getAssignedRoute(1)); sameRoute(third,p.getAssignedRoute(3));
        controller.update(zone);
        check(controller.status(2).risk==RiskState::SAFE&&controller.status(2).rerouted,"T05 successful detour did not become SAFE/REROUTED");
        const auto attempts=controller.status(2).planningAttempts;
        for(int frame=1;frame<=600;++frame) {
            p.update(25.+frame*.5); controller.update(zone);
            const auto s=p.getState(2);
            check(!detector.segmentIntersectsZone({s.latitude,s.longitude,s.altitude},{s.latitude,s.longitude,s.altitude},buffered),"T05 actual flight entered safety circle");
        }
        check(p.hasArrived(1)&&p.hasArrived(2)&&p.hasArrived(3),"All platforms must reach their destinations");
        check(controller.status(2).applications==1&&controller.status(2).planningAttempts==attempts,"Stability: same zone replanned every frame");

        // A fresh edit must be assessed, but a blocked route must not be retried per frame.
        p.reset(); controller.reset();
        const auto initial=p.getState(2);
        auto inside=zone; inside.longitude=initial.longitude;
        controller.update(inside);
        check(controller.hasBlockedRoute()&&controller.status(2).applications==0,"Unsafe zone containing current position must hold");
        samePosition(initial,p.getState(2));
        const auto blockedAttempts=controller.status(2).planningAttempts;
        for(int i=0;i<10;++i) controller.update(inside);
        check(controller.status(2).planningAttempts==blockedAttempts,"Stability: blocked route retried every frame");
        inside.active=false; controller.update(inside);
        check(!controller.hasBlockedRoute(),"Deactivation did not release safety hold");
        controller.update(zone);
        check(controller.status(2).applications==1,"T06 changing zone center did not re-evaluate");
        auto expanded=zone; expanded.radiusMeters=4500.;
        check(detector.checkRoute(p.getRoute(2),expanded)==RiskState::VIOLATION,"T06 edited test zone must intersect the existing detour");
        controller.update(expanded);
        check(controller.status(2).applications==2||controller.hasBlockedRoute(),"T06 larger radius did not re-evaluate remaining path");
        const auto editedAttempts=controller.status(2).planningAttempts;
        controller.update(expanded);
        check(controller.status(2).planningAttempts==editedAttempts,"Stability: unchanged radius caused repeated planning");
        auto reduced=zone; controller.update(reduced);
        check(!controller.hasBlockedRoute(),"T06 shrinking radius did not re-evaluate/release hold");
        expanded.active=false; controller.update(expanded);
        for(int id:p.getPlatformIds()) check(controller.status(id).risk==RiskState::SAFE,"T07 deactivated zone still produces violation");
        check(!controller.hasBlockedRoute(),"Inactive edited zone did not release hold");
        const auto count=controller.status(2).applications;
        p.update(10.); controller.update(expanded);
        check(controller.status(2).applications==count,"Inactive zone caused replanning");

        for(int repeat=0;repeat<3;++repeat) {
            p.reset(); controller.reset(); controller.update(zone);
            check(controller.status(2).applications==1&&controller.status(1).applications==0&&controller.status(3).applications==0,"Stability: reset is not repeatable");
        }
        auto invalid=zone; invalid.radiusMeters=std::numeric_limits<double>::quiet_NaN();
        bool rejected=false; try { controller.update(invalid); } catch(const std::invalid_argument&) { rejected=true; }
        check(rejected&&near(controller.zone().radiusMeters,zone.radiusMeters),"Invalid zone edit was accepted or mutated state");

        // The visible VIOLATION dwell is deterministic and safety hold is immediate.
        p.reset(); DemoController delayed(p,500.,.75);
        delayed.update(zone);
        check(delayed.status(2).risk==RiskState::VIOLATION&&!delayed.status(2).rerouted,"Visible violation phase missing");
        p.update(.5); delayed.update(zone); check(delayed.status(2).applications==0,"Violation notice ended early");
        p.update(.75); delayed.update(zone); check(delayed.status(2).applications==1,"Violation notice never progressed to reroute");
        p.reset(); delayed.reset(); inside.active=true;
        delayed.update(inside); check(delayed.hasBlockedRoute(),"Unsafe start was allowed during notice interval");
        std::cout<<"PASS: T01-T08 integrated APIs; full-margin rejection/retry; actual flight clearance; continuity; unaffected UAVs; zone edits; hold/release; bounded replanning; repeatable reset\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<"\n"; return 1; }
}
