#include "ControlPanel.h"
#include <imgui.h>
#include <iostream>

// ──────────────────────────────────────────────────────────
// Helpers shared by both SIMDIS and headless implementations
// ──────────────────────────────────────────────────────────

static const char* riskStateStr(RiskState state, bool rerouted)
{
    if (rerouted)           return "REROUTED";
    switch (state) {
        case RiskState::VIOLATION: return "VIOLATION";
        case RiskState::WARNING:   return "WARNING";
        default:                   return "SAFE";
    }
}

// ──────────────────────────────────────────────────────────
// Constructor
// ──────────────────────────────────────────────────────────

#ifdef SIMDIS_SDK_FOUND
ControlPanel::ControlPanel()
    : CONTROL_PANEL_BASE("SIMDIS Risk Zone Demo")
{
#else
ControlPanel::ControlPanel()
{
#endif
    riskZoneData_ = {38.45, 27.20, 1500.0, false};
    uavStatuses_[1] = {1, "UAV-1", RiskState::SAFE, false};
    uavStatuses_[2] = {2, "UAV-2", RiskState::SAFE, false};
    uavStatuses_[3] = {3, "UAV-3", RiskState::SAFE, false};
}

// ──────────────────────────────────────────────────────────
// Shared API
// ──────────────────────────────────────────────────────────

void ControlPanel::setRiskZoneData(const RiskZoneData& data)  { riskZoneData_ = data; }
RiskZoneData ControlPanel::getRiskZoneData() const             { return riskZoneData_; }

void ControlPanel::updateUAVStatus(int id, const std::string& name, RiskState state, bool rerouted)
{
    uavStatuses_[id] = {id, name, state, rerouted};
}

// ──────────────────────────────────────────────────────────
// ImGui drawing logic (shared between SIMDIS and headless)
// ──────────────────────────────────────────────────────────

void ControlPanel::drawPanel_()
{
    // ── Header ──────────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "SIMDIS RISK ZONE DEMO");
    ImGui::Separator();

    // ── Risk Zone Controls ───────────────────────────────────
    bool active = riskZoneData_.active;
    ImGui::Text("Risk Zone:");
    ImGui::SameLine();
    if (active) {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "ACTIVE [ON]");
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "INACTIVE [OFF]");
    }

    float radius = static_cast<float>(riskZoneData_.radiusMeters);
    if (ImGui::SliderFloat("Radius (m)", &radius, 100.0f, 5000.0f)) {
        riskZoneData_.radiusMeters = static_cast<double>(radius);
    }

    if (ImGui::Button(active ? "Deactivate Risk Zone" : "Activate Risk Zone")) {
        riskZoneData_.active = !riskZoneData_.active;
    }

    // ── UAV Status Table ─────────────────────────────────────
    ImGui::Separator();
    ImGui::Text("UAV Statuses:");
    ImGui::Spacing();

    if (ImGui::BeginTable("uav_table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("UAV",    ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& [id, status] : uavStatuses_) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", status.name.c_str());

            ImGui::TableSetColumnIndex(1);
            const char* label = riskStateStr(status.state, status.rerouted);
            if (status.rerouted) {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "%s", label);
            } else if (status.state == RiskState::VIOLATION) {
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", label);
            } else if (status.state == RiskState::WARNING) {
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%s", label);
            } else {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "%s", label);
            }
        }
        ImGui::EndTable();
    }
}

// ──────────────────────────────────────────────────────────
// SIMDIS mode: override drawContents_ (called by osgEarth)
// ──────────────────────────────────────────────────────────
#ifdef SIMDIS_SDK_FOUND
void ControlPanel::drawContents_(osg::RenderInfo& /*renderInfo*/)
{
    drawPanel_();
}

// ──────────────────────────────────────────────────────────
// Headless / mock mode: print to console
// ──────────────────────────────────────────────────────────
#else
void ControlPanel::renderHeadless()
{
    std::cout << "\n--- SIMDIS RISK ZONE DEMO ---\n";
    std::cout << "Risk Zone: " << (riskZoneData_.active ? "ON" : "OFF")
              << " | Radius: " << riskZoneData_.radiusMeters << " m\n";
    for (const auto& [id, status] : uavStatuses_) {
        std::cout << "  " << status.name << ": "
                  << riskStateStr(status.state, status.rerouted) << "\n";
    }
    std::cout << "-----------------------------\n";
}
#endif
