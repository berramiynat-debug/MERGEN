#pragma once
#include <vector>
#include <string>

struct Waypoint {
    double latitude;
    double longitude;
    double altitude;
};

struct AircraftState {
    int id;
    double latitude;
    double longitude;
    double altitude;
    double heading;
    double speed;
};

struct RiskZoneData {
    double latitude;
    double longitude;
    double radiusMeters;
    bool active;
};

enum class RiskState { SAFE, WARNING, VIOLATION };
