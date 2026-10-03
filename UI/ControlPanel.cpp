#include "ControlPanel.h"
#include <iostream>

#if __has_include(<imgui.h>)
  #include <imgui.h>
  #define HAS_IMGUI 1
#else
  #define HAS_IMGUI 0
#endif

ControlPanel::ControlPanel() {
    riskZoneData_ = {38.45, 27.20, 1500.0, false};
    uavStatuses_[1] = {1, "UAV-1", RiskState::SAFE, false};
    uavStatuses_[2] = {2, "UAV-2", RiskState::SAFE, false};
    uavStatuses_[3] = {3, "UAV-3", RiskState::SAFE, false};
}

void ControlPanel::setRiskZoneData(const RiskZoneData& data) {
    riskZoneData_ = data;
}

RiskZoneData ControlPanel::getRiskZoneData() const {
    return riskZoneData_;
}

void ControlPanel::updateUAVStatus(int id, const std::string& name, RiskState state, bool rerouted) {
    uavStatuses_[id] = {id, name, state, rerouted};
}

void ControlPanel::renderUI() {
#if HAS_IMGUI
    ImGui::Begin("SIMDIS RISK ZONE DEMO");

    ImGui::Text("Risk Zone Status: %s", riskZoneData_.active ? "ACTIVE [ON]" : "INACTIVE [OFF]");
    
    float radius = static_cast<float>(riskZoneData_.radiusMeters);
    if (ImGui::SliderFloat("Radius (m)", &radius, 100.0f, 5000.0f)) {
        riskZoneData_.radiusMeters = radius;
    }

    if (ImGui::Button(riskZoneData_.active ? "Deactivate Risk Zone" : "Activate Risk Zone")) {
        riskZoneData_.active = !riskZoneData_.active;
    }

    ImGui::Separator();
    ImGui::Text("UAV Statuses:");

    for (const auto& [id, status] : uavStatuses_) {
        std::string stateStr = "SAFE";
        if (status.rerouted) {
            stateStr = "REROUTED";
        } else if (status.state == RiskState::VIOLATION) {
            stateStr = "VIOLATION";
        } else if (status.state == RiskState::WARNING) {
            stateStr = "WARNING";
        }

        ImGui::Text("%s: %s", status.name.c_str(), stateStr.c_str());
    }

    ImGui::End();
#else
    // Fallback console rendering for headless / unit test verification
    std::cout << "--- SIMDIS RISK ZONE DEMO ---" << std::endl;
    std::cout << "Risk Zone: " << (riskZoneData_.active ? "ON" : "OFF")
              << " | Radius: " << riskZoneData_.radiusMeters << " m" << std::endl;
    for (const auto& [id, status] : uavStatuses_) {
        std::string stateStr = "SAFE";
        if (status.rerouted) stateStr = "REROUTED";
        else if (status.state == RiskState::VIOLATION) stateStr = "VIOLATION";

        std::cout << "  " << status.name << ": " << stateStr << std::endl;
    }
    std::cout << "-----------------------------" << std::endl;
#endif
}
