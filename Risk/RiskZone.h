#pragma once
#include "CommonTypes.h"

class RiskZone {
public:
    void setCenter(double latitude, double longitude);
    void setRadius(double radiusMeters);
    void activate();
    void deactivate();
    bool isActive() const;
    RiskZoneData getData() const;
    bool contains(const Waypoint& point) const;

private:
    // Risk bölgesinin durumunu ve geometrisini tutan veri yapısı.
    // Başlangıçta pasif (active = false) ve 0 metre yarıçap ile başlatılır.
    RiskZoneData m_data{0.0, 0.0, 0.0, false};
};
