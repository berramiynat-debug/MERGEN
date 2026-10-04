#pragma once
#include "CommonTypes.h"
#include <memory>
#include <cstddef>

namespace simData { class DataStore; }

// Single-threaded API: call from the simulation/update thread.
// Degrees (WGS84), metres (ellipsoid altitude), seconds, m/s;
// heading is clockwise from north in degrees. time must be nondecreasing.
class PlatformManager {
public:
    explicit PlatformManager(simData::DataStore& dataStore);
    ~PlatformManager();
    PlatformManager(const PlatformManager&) = delete;
    PlatformManager& operator=(const PlatformManager&) = delete;

    int createPlatform(const std::string& name, const Waypoint& start);
    AircraftState getState(int platformId) const;
    // Current position followed by unvisited waypoints. At arrival: one point.
    std::vector<Waypoint> getRoute(int platformId) const;
    // Keeps the current position; validates the whole input before modifying it.
    // This is structural validation only; caller must validate risk safety first.
    void setRoute(int platformId, const std::vector<Waypoint>& route);
    void update(double simulationTime);

    void setSpeed(int platformId, double metresPerSecond);
    double getCruiseSpeed(int platformId) const;
    std::string getName(int platformId) const;
    std::vector<int> getPlatformIds() const;
    std::vector<Waypoint> getOriginalRoute(int platformId) const;
    std::vector<Waypoint> getAssignedRoute(int platformId) const;
    std::size_t getRouteRevision(int platformId) const;
    bool hasArrived(int platformId) const;
    double simulationTime() const;
    // Restores original routes/positions/speeds and clears stored track history.
    void reset();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
