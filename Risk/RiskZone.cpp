#include "RiskZone.h"
#include <cmath>

namespace {
    // Coğrafi hesaplamalar için sabitler (WGS-84 yerel düzlem yaklaşımı / Equirectangular projection)
    // 1 enlem derecesi ~111.320 metredir (111.32 km).
    // Boylam mesafesi ise enleme bağlı olarak cos(lat) ile daralır.
    constexpr double PI = 3.14159265358979323846;
    constexpr double DEG_TO_RAD = PI / 180.0;
    constexpr double METERS_PER_DEG_LAT = 111320.0;
}

void RiskZone::setCenter(double latitude, double longitude) {
    m_data.latitude = latitude;
    m_data.longitude = longitude;
}

void RiskZone::setRadius(double radiusMeters) {
    m_data.radiusMeters = radiusMeters;
}

void RiskZone::activate() {
    m_data.active = true;
}

void RiskZone::deactivate() {
    m_data.active = false;
}

bool RiskZone::isActive() const {
    return m_data.active;
}

RiskZoneData RiskZone::getData() const {
    return m_data;
}

bool RiskZone::contains(const Waypoint& point) const {
    // Bölge pasif ise hiçbir nokta ihlal sayılmaz
    if (!m_data.active) {
        return false;
    }

    // Yerel düzlem (Equirectangular) projeksiyonu ile mesafe hesabı:
    // Merkez referans alınarak X ve Y farkları metre cinsine çevrilir.
    // İrtifa (altitude) tasarım gereği 2B silindirik model kabul edildiğinden ihmal edilmiştir.
    const double centerLatRad = m_data.latitude * DEG_TO_RAD;
    const double dy = (point.latitude - m_data.latitude) * METERS_PER_DEG_LAT;
    const double dx = (point.longitude - m_data.longitude) * METERS_PER_DEG_LAT * std::cos(centerLatRad);

    const double distanceSquared = dx * dx + dy * dy;
    const double radiusSquared = m_data.radiusMeters * m_data.radiusMeters;

    // Sınır güvenliği kararı: Sınır noktaları dahil (<=) kabul edilir.
    // Havacılık ve emniyet standartlarında sınır çizgisi risk bölgesi kabul edilir.
    return distanceSquared <= radiusSquared;
}
