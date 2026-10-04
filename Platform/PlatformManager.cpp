#include "PlatformManager.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include "simCore/Calc/Coordinate.h"
#include "simCore/Calc/CoordinateConverter.h"
#include "simData/DataStore.h"
#include "simData/DataTypeProperties.h"
#include "simData/EntityPreferences.h"
#include "simData/DataTypeUpdates.h"

namespace {
constexpr double pi = 3.14159265358979323846;
constexpr double rad = pi / 180.;
constexpr double metresPerDegree = 111139.; // same small-area convention as Navigation
struct Segment { double north, east, up, length; };
Segment segment(const Waypoint& a, const Waypoint& b) {
    const double north = (b.latitude-a.latitude)*metresPerDegree;
    const double east = (b.longitude-a.longitude)*metresPerDegree*std::cos((a.latitude+b.latitude)*.5*rad);
    const double up = b.altitude-a.altitude;
    return {north,east,up,std::hypot(std::hypot(north,east),up)};
}
void validate(const Waypoint& p) {
    if (!std::isfinite(p.latitude) || !std::isfinite(p.longitude) || !std::isfinite(p.altitude)
        || std::abs(p.latitude)>85. || std::abs(p.longitude)>180.)
        throw std::invalid_argument("Invalid waypoint: finite degrees/metres required, latitude within +/-85");
}
Waypoint position(const AircraftState& s) { return {s.latitude,s.longitude,s.altitude}; }
}

struct PlatformManager::Impl {
    struct Flight {
        simData::ObjectId sdkId;
        std::string name;
        AircraftState state;
        Waypoint start;
        std::vector<Waypoint> original, assigned;
        double cruise = 80., initialSpeed = 80., epoch = 0.;
        std::size_t next = 1, revision = 0;
        bool arrived = true;
        double pitch = 0.;
    };
    explicit Impl(simData::DataStore& store) : store(store) { store.setDataLimiting(true); }
    simData::DataStore& store;
    std::map<int,Flight> flights;
    double time = 0.;
    int nextId = 1;
    Flight& at(int id) { return flights.at(id); }
    const Flight& at(int id) const { return flights.at(id); }
    void evaluate(Flight& f) {
        double remaining = (time-f.epoch)*f.cruise;
        f.arrived = true;
        Waypoint p = f.assigned.back();
        f.next = f.assigned.size();
        for (std::size_t i=1; i<f.assigned.size(); ++i) {
            const auto& a=f.assigned[i-1]; const auto& b=f.assigned[i];
            const auto d=segment(a,b);
            if (remaining>=d.length) { remaining-=d.length; continue; }
            const double u=remaining/d.length;
            p={a.latitude+(b.latitude-a.latitude)*u, a.longitude+(b.longitude-a.longitude)*u, a.altitude+(b.altitude-a.altitude)*u};
            f.state.heading=std::fmod(std::atan2(d.east,d.north)/rad+360.,360.);
            f.pitch=std::atan2(d.up,std::hypot(d.north,d.east));
            f.next=i; f.arrived=false; break;
        }
        f.state.latitude=p.latitude; f.state.longitude=p.longitude; f.state.altitude=p.altitude;
        f.state.speed=f.arrived ? 0. : f.cruise;
    }
    void publish(Flight& f) {
        const double h=f.state.heading*rad, v=f.state.speed;
        simCore::Coordinate lla(simCore::COORD_SYS_LLA,
            simCore::Vec3(f.state.latitude*rad,f.state.longitude*rad,f.state.altitude),
            simCore::Vec3(h,f.pitch,0.),
            simCore::Vec3(v*std::cos(f.pitch)*std::cos(h),v*std::cos(f.pitch)*std::sin(h),-v*std::sin(f.pitch)));
        const auto ecef=simCore::CoordinateConverter::convertGeodeticToEcef(lla,simCore::LOCAL_LEVEL_FRAME_NED);
        if (!ecef) throw std::runtime_error("LLA to ECEF conversion failed");
        simData::DataStore::Transaction tx;
        auto* u=store.addPlatformUpdate(f.sdkId,&tx);
        if (!u) throw std::runtime_error("Unable to add platform update");
        u->set_time(time); u->set_x(ecef->x()); u->set_y(ecef->y()); u->set_z(ecef->z());
        u->set_psi(ecef->psi()); u->set_theta(ecef->theta()); u->set_phi(ecef->phi());
        u->set_vx(ecef->vx()); u->set_vy(ecef->vy()); u->set_vz(ecef->vz());
        tx.complete(&u);
    }
};

PlatformManager::PlatformManager(simData::DataStore& store) : impl_(std::make_unique<Impl>(store)) {}
PlatformManager::~PlatformManager()=default;
int PlatformManager::createPlatform(const std::string& name,const Waypoint& start) {
    validate(start);
    if (name.empty()) throw std::invalid_argument("Platform name cannot be empty");
    for (const auto& [id,f]:impl_->flights) if (f.name==name) throw std::invalid_argument("Duplicate platform name");
    simData::DataStore::Transaction tx;
    auto* props=impl_->store.addPlatform(&tx);
    const auto sdkId=props->id(); tx.complete(&props);
    const int id=impl_->nextId++;
    auto* prefs=impl_->store.mutable_platformPrefs(sdkId,&tx);
    prefs->set_icon("dragon_eye.ive");
    prefs->set_dynamicscale(true); prefs->set_scale(3.0f);
    auto* common=prefs->mutable_commonprefs();
    common->set_name(name); common->set_draw(true); common->set_datalimitpoints(12000);
    common->mutable_labelprefs()->set_draw(true);
    static constexpr unsigned colors[]={0x36C5F0FF,0xFFB347FF,0xC792EAFF};
    common->mutable_labelprefs()->set_color(colors[(id-1)%3]);
    prefs->mutable_trackprefs()->set_trackdrawmode(simData::TrackPrefs::Mode::LINE);
    prefs->mutable_trackprefs()->set_trackcolor(colors[(id-1)%3]);
    prefs->mutable_trackprefs()->set_tracklength(180);
    tx.complete(&prefs);
    Impl::Flight f{}; f.sdkId=sdkId; f.name=name; f.start=start;
    f.state={id,start.latitude,start.longitude,start.altitude,0.,0.};
    f.assigned={start}; f.epoch=impl_->time;
    auto& stored=impl_->flights.emplace(id,std::move(f)).first->second;
    impl_->publish(stored);
    return id;
}
AircraftState PlatformManager::getState(int id) const { return impl_->at(id).state; }
std::vector<Waypoint> PlatformManager::getRoute(int id) const {
    const auto& f=impl_->at(id);
    std::vector<Waypoint> result{position(f.state)};
    result.insert(result.end(),f.assigned.begin()+f.next,f.assigned.end());
    return result;
}
void PlatformManager::setRoute(int id,const std::vector<Waypoint>& route) {
    auto& f=impl_->at(id);
    if (route.empty()) throw std::invalid_argument("Route cannot be empty");
    std::vector<Waypoint> clean{position(f.state)};
    for (const auto& p:route) {
        validate(p);
        const auto distance=segment(clean.back(),p).length;
        if (distance>100000.) throw std::invalid_argument("Demo route segments must be within 100 km");
        if (distance>1e-6) clean.push_back(p);
    }
    // Atomic replacement only after every point has passed validation.
    f.assigned=std::move(clean); f.epoch=impl_->time; ++f.revision;
    if (f.original.empty()) { f.original=f.assigned; f.initialSpeed=f.cruise; }
    impl_->evaluate(f); impl_->publish(f);
}
void PlatformManager::update(double time) {
    if (!std::isfinite(time) || time<impl_->time) throw std::invalid_argument("Simulation time must be finite and nondecreasing; use reset to restart");
    const bool changed=time!=impl_->time;
    impl_->time=time;
    if (changed) for (auto& [id,f]:impl_->flights) { impl_->evaluate(f); impl_->publish(f); }
    impl_->store.update(time);
}
void PlatformManager::setSpeed(int id,double speed) {
    if (!std::isfinite(speed) || speed<=0. || speed>1000.) throw std::invalid_argument("Speed must be in (0,1000] m/s");
    auto& f=impl_->at(id);
    auto remaining=getRoute(id); f.cruise=speed; f.assigned=std::move(remaining); f.epoch=impl_->time; ++f.revision;
    impl_->evaluate(f); impl_->publish(f);
}
double PlatformManager::getCruiseSpeed(int id) const { return impl_->at(id).cruise; }
std::string PlatformManager::getName(int id) const { return impl_->at(id).name; }
std::vector<int> PlatformManager::getPlatformIds() const { std::vector<int> ids; for(const auto& [id,f]:impl_->flights) ids.push_back(id); return ids; }
std::vector<Waypoint> PlatformManager::getOriginalRoute(int id) const { return impl_->at(id).original; }
std::vector<Waypoint> PlatformManager::getAssignedRoute(int id) const { return impl_->at(id).assigned; }
std::size_t PlatformManager::getRouteRevision(int id) const { return impl_->at(id).revision; }
bool PlatformManager::hasArrived(int id) const { return impl_->at(id).arrived; }
double PlatformManager::simulationTime() const { return impl_->time; }
void PlatformManager::reset() {
    impl_->time=0.;
    for(auto& [id,f]:impl_->flights) {
        impl_->store.flush(f.sdkId,simData::DataStore::FLUSH_NONRECURSIVE,simData::DataStore::FLUSH_UPDATES);
        f.assigned=f.original.empty()? std::vector<Waypoint>{f.start}:f.original;
        f.state={id,f.start.latitude,f.start.longitude,f.start.altitude,0.,0.};
        f.cruise=f.initialSpeed; f.epoch=0.; ++f.revision;
        impl_->evaluate(f); impl_->publish(f);
    }
    impl_->store.update(0.);
}
