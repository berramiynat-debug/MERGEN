#include "Risk/RiskZone.h"
#include "Risk/ViolationDetector.h"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>

namespace {
    int g_passCount = 0;
    int g_failCount = 0;

    void reportResult(const std::string& testName, bool condition, const std::string& details = "") {
        std::cout << "[" << (condition ? "PASS" : "FAIL") << "] " << testName;
        if (!details.empty()) {
            std::cout << " (" << details << ")";
        }
        std::cout << "\n";
        if (condition) {
            ++g_passCount;
        } else {
            ++g_failCount;
        }
    }
}

int main() {
    std::cout << "====================================================\n";
    std::cout << " MERGEN Projesi - Risk & ViolationDetector Birim Testleri\n";
    std::cout << "====================================================\n\n";

    RiskZone zone;
    ViolationDetector detector;

    // ----------------------------------------------------
    // Test Grubu 1: RiskZone Temel Fonksiyonları ve contains()
    // ----------------------------------------------------
    std::cout << "--- TEST GRUBU 1: RiskZone Durum ve Nokta Kontrolu ---\n";
    {
        zone.setCenter(38.45, 27.20);
        zone.setRadius(1500.0); // 1500 metre
        zone.deactivate();

        reportResult("1.1 RiskZone varsayilan/deaktif durumu", !zone.isActive());

        RiskZoneData data = zone.getData();
        reportResult("1.2 getData() dogru degerler",
                     (data.latitude == 38.45 && data.longitude == 27.20 &&
                      data.radiusMeters == 1500.0 && !data.active));

        Waypoint centerPoint{38.45, 27.20, 1000.0};
        reportResult("1.3 contains() bolge pasifken merkez icin false donmeli",
                     !zone.contains(centerPoint));

        zone.activate();
        reportResult("1.4 activate() sonrasi isActive() true", zone.isActive());
        reportResult("1.5 contains() bolge aktifken merkez icin true donmeli",
                     zone.contains(centerPoint));

        Waypoint farPoint{38.55, 27.35, 1000.0}; // ~15-20 km uzakta
        reportResult("1.6 contains() uzaktaki nokta icin false donmeli",
                     !zone.contains(farPoint));

        // Tam sinir uzerinde bir nokta (~1500 metre kuzey)
        // 1500 m / 111320 m/deg = ~0.01347457779 derece enlem
        Waypoint boundaryPoint{38.45 + (1500.0 / 111320.0), 27.20, 1000.0};
        reportResult("1.7 contains() sinir uzerindeki nokta icin true (<= kurali)",
                     zone.contains(boundaryPoint));
    }

    std::cout << "\n--- TEST GRUBU 2: Kesisim (Segment & Route) Kontrolleri ---\n";
    {
        // Dokuman ile uyumlu referans koordinatlar (Izmir civari)
        // Merkez: {38.45, 27.20}, Yaricap: 1500 m
        RiskZoneData activeZone{38.45, 27.20, 1500.0, true};
        RiskZoneData inactiveZone{38.45, 27.20, 1500.0, false};

        // 2.1 Kritik Test: Iki uc nokta disarida ama segment merkezden geciyor
        Waypoint startOutside{38.40, 27.20, 1000.0};  // Guneyde (~5.5 km)
        Waypoint endOutside{38.50, 27.20, 1000.0};    // Kuzeyde (~5.5 km)
        std::vector<Waypoint> crossingRoute{startOutside, endOutside};

        bool segmentCross = detector.segmentIntersectsZone(startOutside, endOutside, activeZone);
        RiskState routeCross = detector.checkRoute(crossingRoute, activeZone);
        reportResult("2.1 [KRITIK] Uclar disarida fakat segment bolgeyi kesiyor -> VIOLATION",
                     segmentCross && (routeCross == RiskState::VIOLATION));

        // 2.2 Bolgeden tamamen uzak rota -> SAFE
        Waypoint safeStart{38.40, 27.05, 1000.0};
        Waypoint safeEnd{38.50, 27.05, 1000.0};
        std::vector<Waypoint> safeRoute{safeStart, safeEnd};
        reportResult("2.2 Risk alanini kesmeyen rota -> SAFE",
                     !detector.segmentIntersectsZone(safeStart, safeEnd, activeZone) &&
                     (detector.checkRoute(safeRoute, activeZone) == RiskState::SAFE));

        // 2.3 Bir ucu bolgenin icinde, diger ucu disinda
        Waypoint insidePoint{38.4505, 27.2005, 1000.0}; // Merkeze ~70 metre
        std::vector<Waypoint> oneEndInsideRoute{insidePoint, safeEnd};
        reportResult("2.3 Bir ucu iceride olan segment -> VIOLATION",
                     detector.segmentIntersectsZone(insidePoint, safeEnd, activeZone) &&
                     (detector.checkRoute(oneEndInsideRoute, activeZone) == RiskState::VIOLATION));

        // 2.4 Bolge PASIF iken kesen rota -> SAFE (T-01 / T-07)
        reportResult("2.4 Bolge PASIF iken kesen rota -> SAFE (ihlal uretilmez)",
                     !detector.segmentIntersectsZone(startOutside, endOutside, inactiveZone) &&
                     (detector.checkRoute(crossingRoute, inactiveZone) == RiskState::SAFE));

        // 2.5 Bolge aktif ama rotadan cok uzakta (T-02)
        RiskZoneData farAwayZone{39.00, 28.00, 1500.0, true}; // ~100 km uzakta
        reportResult("2.5 Bolge aktif fakat cok uzakta -> SAFE",
                     detector.checkRoute(crossingRoute, farAwayZone) == RiskState::SAFE);
    }

    std::cout << "\n--- TEST GRUBU 3: Cok Segmentli Rota & Dinamik Yaricap ---\n";
    {
        RiskZoneData zoneData{38.45, 27.20, 1500.0, true};

        // 3.1 Cok segmentli rota: 1. ve 2. segment guvenli, 3. segment kesiyor
        Waypoint wp1{38.40, 27.05, 1000.0};
        Waypoint wp2{38.42, 27.08, 1000.0};
        Waypoint wp3{38.42, 27.20, 1000.0}; // Merkezin ~3.3 km guneyinde
        Waypoint wp4{38.48, 27.20, 1000.0}; // Merkezin ~3.3 km kuzeyinde (wp3->wp4 bolgeyi keser)
        std::vector<Waypoint> multiSegRoute{wp1, wp2, wp3, wp4};

        reportResult("3.1 Cok segmentli rotada yalnizca son segment kesiyor -> VIOLATION",
                     detector.checkRoute(multiSegRoute, zoneData) == RiskState::VIOLATION);

        // 3.2 Cok segmentli tamamen guvenli rota
        std::vector<Waypoint> multiSegSafeRoute{wp1, wp2, {38.45, 27.08, 1000.0}, {38.50, 27.05, 1000.0}};
        reportResult("3.2 Cok segmentli hicbir segmenti kesmeyen rota -> SAFE",
                     detector.checkRoute(multiSegSafeRoute, zoneData) == RiskState::SAFE);

        // 3.3 Dinamik yaricap degisimi (T-06):
        // Segment merkezden ~1000 m mesafeden geciyor:
        // Yaricap 500 m iken SAFE, yaricap 1500 m iken VIOLATION olmali
        Waypoint nearStart{38.40, 27.2114, 1000.0}; // ~1000 m dogudan gecen dikey rota
        Waypoint nearEnd{38.50, 27.2114, 1000.0};
        std::vector<Waypoint> nearRoute{nearStart, nearEnd};

        RiskZoneData smallZone{38.45, 27.20, 500.0, true};
        RiskZoneData largeZone{38.45, 27.20, 1500.0, true};

        bool smallResultSafe = (detector.checkRoute(nearRoute, smallZone) == RiskState::SAFE);
        bool largeResultViolation = (detector.checkRoute(nearRoute, largeZone) == RiskState::VIOLATION);
        reportResult("3.3 Yaricap degisimi: Kucuk yaricapta SAFE, genisletilince VIOLATION",
                     smallResultSafe && largeResultViolation);
    }

    std::cout << "\n--- TEST GRUBU 4: Kenar Durumlar (Edge Cases) ---\n";
    {
        RiskZoneData zoneData{38.45, 27.20, 1500.0, true};

        // 4.1 Bos rota -> SAFE
        std::vector<Waypoint> emptyRoute;
        reportResult("4.1 Bos rota -> SAFE",
                     detector.checkRoute(emptyRoute, zoneData) == RiskState::SAFE);

        // 4.2 Tek waypoint iceride -> VIOLATION
        std::vector<Waypoint> singleInsideRoute{{38.45, 27.20, 1000.0}};
        reportResult("4.2 Tek waypoint (iceride) -> VIOLATION",
                     detector.checkRoute(singleInsideRoute, zoneData) == RiskState::VIOLATION);

        // 4.3 Tek waypoint disarida -> SAFE
        std::vector<Waypoint> singleOutsideRoute{{38.55, 27.35, 1000.0}};
        reportResult("4.3 Tek waypoint (disarida) -> SAFE",
                     detector.checkRoute(singleOutsideRoute, zoneData) == RiskState::SAFE);

        // 4.4 Sifir uzunluklu segment (Start == End)
        Waypoint pInside{38.45, 27.20, 1000.0};
        Waypoint pOutside{38.55, 27.35, 1000.0};
        reportResult("4.4 Sifir uzunluklu segment (icte true, dista false)",
                     detector.segmentIntersectsZone(pInside, pInside, zoneData) &&
                     !detector.segmentIntersectsZone(pOutside, pOutside, zoneData));
    }

    std::cout << "\n--- TEST GRUBU 5: 3 IHA Simule Senaryosu (T-03 Mantigi) ---\n";
    {
        RiskZoneData activeZone{38.45, 27.20, 1500.0, true};

        // IHA 1: Bolgenin tam ortasindan gecen rota -> VIOLATION
        std::vector<Waypoint> uav1Route = {
            {38.40, 27.15, 1000.0},
            {38.45, 27.20, 1000.0},
            {38.50, 27.25, 1000.0}
        };

        // IHA 2: Kuzeyden guvenli bypass rotasi -> SAFE
        std::vector<Waypoint> uav2Route = {
            {38.48, 27.10, 1000.0},
            {38.50, 27.20, 1000.0},
            {38.52, 27.30, 1000.0}
        };

        // IHA 3: Guneyden guvenli bypass rotasi -> SAFE
        std::vector<Waypoint> uav3Route = {
            {38.38, 27.10, 1000.0},
            {38.40, 27.20, 1000.0},
            {38.42, 27.30, 1000.0}
        };

        RiskState uav1State = detector.checkRoute(uav1Route, activeZone);
        RiskState uav2State = detector.checkRoute(uav2Route, activeZone);
        RiskState uav3State = detector.checkRoute(uav3Route, activeZone);

        bool multiUavSuccess = (uav1State == RiskState::VIOLATION) &&
                               (uav2State == RiskState::SAFE) &&
                               (uav3State == RiskState::SAFE);

        reportResult("5.1 3 IHA Senaryosu: Yalnizca bolgeden gecen IHA-1 VIOLATION, digerleri SAFE",
                     multiUavSuccess,
                     "UAV1: " + std::string(uav1State == RiskState::VIOLATION ? "VIOLATION" : "SAFE") +
                     ", UAV2: " + std::string(uav2State == RiskState::SAFE ? "SAFE" : "VIOLATION") +
                     ", UAV3: " + std::string(uav3State == RiskState::SAFE ? "SAFE" : "VIOLATION"));
    }

    std::cout << "\n====================================================\n";
    std::cout << " TEST SONUCU: " << g_passCount << " GECTI (PASS), "
              << g_failCount << " BASARISIZ (FAIL)\n";
    std::cout << "====================================================\n";

    return (g_failCount == 0) ? 0 : 1;
}
