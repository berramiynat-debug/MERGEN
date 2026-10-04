#include "ViolationDetector.h"
#include <cmath>
#include <algorithm>

namespace {
    // Coğrafi dönüşüm sabitleri
    constexpr double PI = 3.14159265358979323846;
    constexpr double DEG_TO_RAD = PI / 180.0;
    constexpr double METERS_PER_DEG_LAT = 111320.0;
}

bool ViolationDetector::segmentIntersectsZone(const Waypoint& start,
                                             const Waypoint& end,
                                             const RiskZoneData& zone) const {
    // Bölge pasif ise hiçbir kesişim ihlal sayılmaz
    if (!zone.active) {
        return false;
    }

    // 1. Koordinatları bölge merkezini (0,0) kabul eden yerel metre düzlemine dönüştür
    const double centerLatRad = zone.latitude * DEG_TO_RAD;
    const double cosLat = std::cos(centerLatRad);

    const double ax = (start.longitude - zone.longitude) * METERS_PER_DEG_LAT * cosLat;
    const double ay = (start.latitude - zone.latitude) * METERS_PER_DEG_LAT;

    const double bx = (end.longitude - zone.longitude) * METERS_PER_DEG_LAT * cosLat;
    const double by = (end.latitude - zone.latitude) * METERS_PER_DEG_LAT;

    // 2. AB doğru parçasının yön vektörü
    const double dx = bx - ax;
    const double dy = by - ay;
    const double segLenSq = dx * dx + dy * dy;

    double t = 0.0;
    if (segLenSq > 0.0) {
        // Çember merkezi O(0,0) noktasının AB doğrusu üzerine iz düşüm parametresi t:
        // Vektör AO = (0 - ax, 0 - ay) = (-ax, -ay)
        // t = (AO . AB) / (AB . AB)
        t = -(ax * dx + ay * dy) / segLenSq;
        // t parametresini [0.0, 1.0] aralığına sınırla (Segment üzerindeki en yakın nokta)
        t = std::clamp(t, 0.0, 1.0);
    }

    // 3. Segment üzerinde çember merkezine en yakın olan noktanın koordinatları
    const double closestX = ax + t * dx;
    const double closestY = ay + t * dy;

    // 4. En yakın noktanın merkeze olan mesafesinin karesi
    const double minDistSq = closestX * closestX + closestY * closestY;
    const double radiusSq = zone.radiusMeters * zone.radiusMeters;

    // Sınır kuralı: Sınır noktası (teğet dahil) emniyet gereği ihlal (<=) kabul edilir.
    return minDistSq <= radiusSq;
}

RiskState ViolationDetector::checkRoute(const std::vector<Waypoint>& route,
                                       const RiskZoneData& zone) const {
    // 1. Bölge pasifse veya rota boşsa doğrudan SAFE
    if (!zone.active || route.empty()) {
        return RiskState::SAFE;
    }

    // 2. Tek waypoint'li rota durumu (İHA sabit veya tek hedefli)
    if (route.size() == 1) {
        if (segmentIntersectsZone(route[0], route[0], zone)) {
            return RiskState::VIOLATION;
        }
        return RiskState::SAFE;
    }

    // 3. Çok segmentli rota: ardışık her waypoint çifti için kontrol et
    for (size_t i = 0; i + 1 < route.size(); ++i) {
        if (segmentIntersectsZone(route[i], route[i + 1], zone)) {
            // Herhangi bir segment kesiyorsa tüm rota risklidir
            return RiskState::VIOLATION;
        }
    }

    // Hiçbir segment bölge ile kesişmiyorsa rota güvenlidir
    return RiskState::SAFE;
}
