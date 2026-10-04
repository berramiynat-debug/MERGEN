#include "Platform/PlatformManager.h"
#include "Platform/RouteApplication.h"
#include "Navigation/RoutePlanner.h"
#include "simData/MemoryDataStore.h"
#include "simData/DataTypeUpdates.h"
#include "simCore/Calc/CoordinateConverter.h"
#include "simCore/Calc/Coordinate.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void check(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
bool near(double a,double b,double eps=1e-8) { return std::abs(a-b)<eps; }
void samePosition(const AircraftState& a,const AircraftState& b) {
    check(near(a.latitude,b.latitude)&&near(a.longitude,b.longitude)&&near(a.altitude,b.altitude),"Position changed unexpectedly");
}
template<class F> void rejects(F fn) { bool rejected=false; try { fn(); } catch(const std::exception&) { rejected=true; } check(rejected,"Invalid input was accepted"); }
}
int main() {
    try {
        simData::MemoryDataStore store;
        PlatformManager p(store);
        for(int i=0;i<3;++i) {
            const int id=p.createPlatform("IHA-"+std::to_string(i+1),{38.+i*.02,27.,1000.+100*i});
            p.setSpeed(id,100.+10*i);
            p.setRoute(id,{{38.+i*.02,27.,1000.+100*i},{38.01+i*.02,27.,1000.+100*i},{38.02+i*.02,27.,1000.+100*i}});
        }
        check(p.getPlatformIds().size()==3,"Three platforms required");
        p.update(5.);
        simData::DataStore::IdList sdkIds;
        store.idList(&sdkIds,simData::PLATFORM);
        check(sdkIds.size()==3,"SIMDIS must contain three real platform entities");
        for(const auto sdkId:sdkIds) {
            const auto* u=store.platformUpdateSlice(sdkId)->current();
            check(u && near(u->time(),5.),"SIMDIS update timestamp incorrect");
            auto lla=simCore::CoordinateConverter::convertEcefToGeodetic(
                simCore::Coordinate(simCore::COORD_SYS_ECEF,simCore::Vec3(u->x(),u->y(),u->z())));
            check(lla.has_value(),"Invalid ECEF state in datastore");
            bool found=false;
            for(int id:p.getPlatformIds()) {
                const auto s=p.getState(id);
                if(near(lla->lat()*180./3.14159265358979323846,s.latitude,1e-6)
                    && near(lla->lon()*180./3.14159265358979323846,s.longitude,1e-6)
                    && near(lla->alt(),s.altitude,.01)) found=true;
            }
            check(found,"SIMDIS ECEF update and public state disagree");
        }
        check(near(p.getState(1).latitude,38.+500./111139.),"Position does not match speed x time");
        check(near(p.getState(1).heading,0.),"Northbound heading is wrong");
        const auto one=p.getState(1),two=p.getState(2),three=p.getState(3);
        check(two.latitude>38.02&&three.latitude>38.04,"All three must move");
        const auto oneRev=p.getRouteRevision(1),threeRev=p.getRouteRevision(3);
        p.setRoute(2,{{two.latitude,two.longitude,two.altitude},{two.latitude,27.02,two.altitude}});
        samePosition(two,p.getState(2));
        samePosition(one,p.getState(1)); samePosition(three,p.getState(3));
        check(p.getRouteRevision(1)==oneRev&&p.getRouteRevision(3)==threeRev,"Other aircraft routes changed");
        p.update(10.);
        check(near(p.getState(2).latitude,two.latitude)&&p.getState(2).longitude>27.,"Replacement route is not followed");
        check(p.getState(1).latitude>one.latitude&&p.getState(3).latitude>three.latitude,"Unchanged aircraft did not continue");
        p.update(15.);
        auto remaining=p.getRoute(1);
        check(remaining.size()==2,"Visited waypoint was not removed from remaining route");
        check(near(remaining.front().latitude,p.getState(1).latitude),"Remaining route must start at current position");
        const auto before=p.getState(1); const auto revision=p.getRouteRevision(1);
        rejects([&]{p.setRoute(1,{});});
        rejects([&]{p.setRoute(1,{{38.,27.,1000.},{std::numeric_limits<double>::quiet_NaN(),27.,1000.}});});
        rejects([&]{p.setSpeed(1,0.);}); rejects([&]{p.update(14.);}); rejects([&]{p.getState(999);});
        samePosition(before,p.getState(1)); check(revision==p.getRouteRevision(1),"Invalid input mutated route");
        p.update(15.); samePosition(before,p.getState(1));
        p.update(1000.); check(p.hasArrived(1)&&p.hasArrived(2)&&p.hasArrived(3),"Arrival not detected");
        check(p.getRoute(1).size()==1&&near(p.getState(1).speed,0.),"Arrival state is invalid");
        p.reset(); check(near(p.simulationTime(),0.)&&near(p.getState(1).latitude,38.),"Reset failed");
        p.update(5.); samePosition(one,p.getState(1)); samePosition(two,p.getState(2)); samePosition(three,p.getState(3));

        // Partition invariance: many small frames and one big step agree.
        p.reset(); for(int i=1;i<=100;++i) p.update(i*.05); samePosition(one,p.getState(1));

        // Person 3 validator must reject the joining segment, not just candidate legs.
        const auto s=p.getState(1);
        RiskZoneData zone{s.latitude+.004,27.,200.,true};
        const auto rev=p.getRouteRevision(1);
        check(!applyValidatedRoute(p,1,{{s.latitude+.008,27.,1000.},{s.latitude+.012,27.,1000.}},zone,RouteValidator(0.)),"Unsafe joining segment accepted");
        check(p.getRouteRevision(1)==rev,"Rejected candidate changed route");
        check(!applyValidatedRoute(p,1,{},zone,RouteValidator()),"Empty candidate accepted");
        zone.active=false;
        auto candidate=RoutePlanner().calculateDetour({s.latitude,s.longitude,s.altitude},{38.03,27.02,1000.},zone);
        check(applyValidatedRoute(p,1,candidate,zone,RouteValidator(500.)),"Person 3 candidate integration failed");
        samePosition(s,p.getState(1)); p.update(7.); check(p.getState(1).longitude>27.,"Accepted candidate was not applied");
        const auto speedChange=p.getState(1);
        p.setSpeed(1,200.); samePosition(speedChange,p.getState(1));
        check(near(p.getState(1).speed,200.),"Speed change did not apply");

        // Record an upstream integration limitation without changing person 3's algorithm.
        const RiskZoneData sampleZone{38.45,27.20,1500.,true};
        const auto detour=RoutePlanner(500.).calculateDetour({38.40,27.10,1000.},{38.50,27.30,1000.},sampleZone);
        std::cout<<"Person 3 sample with 500 m margin: "
            <<(RouteValidator(500.).isSafe(detour,sampleZone)?"accepted":"rejected (full margin not met)")<<"\n";
        std::cout<<"PASS: 3 platforms; movement; remaining route; replacement continuity; isolation; invalid input; arrival; reset; frame independence; person 3 integration\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<"FAIL: "<<e.what()<<"\n"; return 1; }
}
