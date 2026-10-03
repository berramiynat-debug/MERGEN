#include "RoutePlanner.h"
#include <cmath>
#include <algorithm>

constexpr double PI = 3.14159265358979323846;
constexpr double EARTH_RADIUS_METERS = 6371000.0;

RoutePlanner::RoutePlanner(double safetyMarginMeters)
    : safetyMarginMeters_(safetyMarginMeters) {}

double RoutePlanner::distanceMeters(const Waypoint& p1, const Waypoint& p2) {
    double lat1Rad = p1.latitude * PI / 180.0;
    double lat2Rad = p2.latitude * PI / 180.0;
    double dLatRad = (p2.latitude - p1.latitude) * PI / 180.0;
    double dLonRad = (p2.longitude - p1.longitude) * PI / 180.0;

    double a = std::sin(dLatRad / 2.0) * std::sin(dLatRad / 2.0) +
               std::cos(lat1Rad) * std::cos(lat2Rad) *
               std::sin(dLonRad / 2.0) * std::sin(dLonRad / 2.0);
    double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return EARTH_RADIUS_METERS * c;
}

double RoutePlanner::distanceToSegmentMeters(const Waypoint& p, const Waypoint& segStart, const Waypoint& segEnd) {
    // Local planar projection around segStart
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

std::vector<Waypoint> RoutePlanner::calculateDetour(
    const Waypoint& currentPosition,
    const Waypoint& destination,
    const RiskZoneData& riskZone) const {

    std::vector<Waypoint> route;
    route.push_back(currentPosition);

    if (!riskZone.active) {
        route.push_back(destination);
        return route;
    }

    Waypoint zoneCenter{riskZone.latitude, riskZone.longitude, currentPosition.altitude};
    double effectiveRadius = riskZone.radiusMeters + safetyMarginMeters_;

    double distToSegment = distanceToSegmentMeters(zoneCenter, currentPosition, destination);

    if (distToSegment >= effectiveRadius) {
        // Safe, no detour needed
        route.push_back(destination);
        return route;
    }

    // Calculate detour waypoint
    double cosLat = std::cos(riskZone.latitude * PI / 180.0);
    double metersPerDegreeLat = 111139.0;
    double metersPerDegreeLon = 111139.0 * cosLat;

    double dx = (destination.longitude - currentPosition.longitude) * metersPerDegreeLon;
    double dy = (destination.latitude - currentPosition.latitude) * metersPerDegreeLat;
    double len = std::sqrt(dx * dx + dy * dy);

    if (len == 0.0) {
        route.push_back(destination);
        return route;
    }

    // Perpendicular unit vector (nx, ny)
    double nx = -dy / len;
    double ny = dx / len;

    // Check which direction (+N or -N) moves away from center
    double cx = (riskZone.longitude - currentPosition.longitude) * metersPerDegreeLon;
    double cy = (riskZone.latitude - currentPosition.latitude) * metersPerDegreeLat;

    // Projection of center vector onto normal
    double dotN = cx * nx + cy * ny;

    // Choose opposite side to detour
    double dir = (dotN >= 0.0) ? -1.0 : 1.0;

    double detourX = cx + dir * nx * effectiveRadius;
    double detourY = cy + dir * ny * effectiveRadius;

    Waypoint detourPoint;
    detourPoint.longitude = currentPosition.longitude + (detourX / metersPerDegreeLon);
    detourPoint.latitude = currentPosition.latitude + (detourY / metersPerDegreeLat);
    detourPoint.altitude = (currentPosition.altitude + destination.altitude) / 2.0;

    route.push_back(detourPoint);
    route.push_back(destination);
    return route;
}
