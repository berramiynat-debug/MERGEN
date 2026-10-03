#pragma once
#include <vector>
#include <string>
#include <map>
#include "../CommonTypes.h"

// Forward declare to avoid pulling in heavy SIMDIS headers here
namespace osg { class RenderInfo; }

struct UAVStatus {
    int id;
    std::string name;
    RiskState state;
    bool rerouted;
};

/**
 * ControlPanel - Kisi 3'un ImGui paneli.
 *
 * SIMDIS ortaminda kullanim:
 *   - simExamples::SimExamplesGui'dan turetilmistir.
 *   - OsgImGuiHandler::add() ile viewer'a eklenir.
 *   - drawContents_() override'i ile ImGui widgetlari cizilir.
 *
 * Mock/bagimsiz test ortaminda:
 *   - renderHeadless() fonksiyonu konsola durum yazdirir.
 */

#ifdef SIMDIS_SDK_FOUND
  #include "SimExamplesGui.h"
  #define CONTROL_PANEL_BASE simExamples::SimExamplesGui

class ControlPanel : public CONTROL_PANEL_BASE
{
public:
    explicit ControlPanel();

    // Called by osgEarth ImGui system every frame
    void drawContents_(osg::RenderInfo& renderInfo) override;

#else
  // Headless / mock mode (no SIMDIS headers needed)
class ControlPanel
{
public:
    ControlPanel();
    void renderHeadless();
#endif

    // --- Shared API (used by both SIMDIS and headless mode) ---

    /** Feed live risk zone data from RiskZone module (Kisi 2) */
    void setRiskZoneData(const RiskZoneData& data);
    RiskZoneData getRiskZoneData() const;

    /** Update per-UAV status from main loop */
    void updateUAVStatus(int id, const std::string& name, RiskState state, bool rerouted = false);

private:
    RiskZoneData riskZoneData_;
    std::map<int, UAVStatus> uavStatuses_;

    void drawPanel_();          // shared ImGui drawing logic
};
