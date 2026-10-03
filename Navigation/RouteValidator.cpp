#include "RouteValidator.h"
#include <cmath>
#include <algorithm>

constexpr double PI = 3.14159265358979323846;

RouteValidator::RouteValidator(double safetyMarginMeters)
    : safetyMarginMeters_(safetyMarginMeters) {}

double RouteValidator::distanceToSegmentMeters(const Waypoint& p, const Waypoint& segStart, const Waypoint& segEnd) {
    double cosLat = std::cos(segStart.latitude * PI / 180.0);
    double metersPerDegreeLat = 111139.0;
    double metersPerDegreeLon = 111139.0 * cosLat;

    double px = (p.longitude - segStart.longitude) * metersPerDegreeLon;
    double py = (p.latitude - segStart.latitude) * metersPerDegreeLat;

    double ax = 0.0;
    double ay = 0.0;
    double bx = (segEnd.longitude - segStart.longitude) * metersPerDegreeLon;
    double by = (segEnd.latitude - segStart.latitude) * metersPerDegreeLat;

    double abx = bx - ax;
    double aby = by - ay;
    double abLenSq = abx * abx + aby * aby;

    if (abLenSq == 0.0) {
        return std::sqrt(px * px + py * py);
    }

    double apx = px - ax;
    double apy = py - ay;
    double t = (apx * abx + apy * aby) / abLenSq;
    t = std::clamp(t, 0.0, 1.0);

    double projX = ax + t * abx;
    double projY = ay + t * aby;

    double dx = px - projX;
    double dy = py - projY;
    return std::sqrt(dx * dx + dy * dy);
}

bool RouteValidator::isSafe(const std::vector<Waypoint>& route,
                            const RiskZoneData& zone) const {
    if (!zone.active || route.size() < 2) {
        return true;
    }

    Waypoint zoneCenter{zone.latitude, zone.longitude, 0.0};
    double thresholdRadius = zone.radiusMeters + safetyMarginMeters_;

    for (size_t i = 0; i < route.size() - 1; ++i) {
        double dist = distanceToSegmentMeters(zoneCenter, route[i], route[i + 1]);
        if (dist < thresholdRadius) {
            return false; // Route intersects or is too close to risk zone
        }
    }

    return true;
}
