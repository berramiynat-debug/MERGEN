#pragma once
#include "Platform/PlatformManager.h"
#include "Risk/RiskZone.h"
#include "Risk/ViolationDetector.h"
#include <map>
#include <optional>
#include <string>

// Person 1 integration: calls the existing cemre/berra APIs.
// UI and algorithm modules never receive SIMDIS objects.
class DemoController {
public:
    struct Status {
        RiskState risk=RiskState::SAFE;
        bool rerouted=false,blocked=false;
        unsigned applications=0,planningAttempts=0;
        std::string message;
    };
    explicit DemoController(PlatformManager& platforms,double safetyMargin=500.,double noticeSeconds=.75);
    void update(const RiskZoneData& requestedZone);
    void reset();
    const Status& status(int platformId) const;
    RiskZoneData zone() const;
    double safetyMargin() const;
    bool hasBlockedRoute() const;
private:
    struct Record {
        Status status;
        std::optional<double> pendingSince;
        std::size_t attemptedRevision=0;
        bool attempted=false;
    };
    PlatformManager& platforms_;
    RiskZone riskZone_;
    ViolationDetector detector_;
    double margin_,noticeSeconds_;
    std::map<int,Record> records_;
};
