#pragma once
#include <vector>
#include <string>
#include <map>
#include "../CommonTypes.h"

struct UAVStatus {
    int id;
    std::string name;
    RiskState state;
    bool rerouted;
};

class ControlPanel {
public:
    ControlPanel();

    void setRiskZoneData(const RiskZoneData& data);
    RiskZoneData getRiskZoneData() const;

    void updateUAVStatus(int id, const std::string& name, RiskState state, bool rerouted);

    // Call this inside the ImGui render loop
    void renderUI();

private:
    RiskZoneData riskZoneData_;
    std::map<int, UAVStatus> uavStatuses_;
};
