#include <chrono>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <thread>
#include <osg/Camera>
#include <osg/GraphicsContext>
#include <osg/Image>
#include <osgDB/WriteFile>
#include <osgViewer/CompositeViewer>
#include "simCore/Common/Version.h"
#include "simData/MemoryDataStore.h"
#include "simVis/Viewer.h"
#include "simVis/SceneManager.h"
#include "simVis/Scenario.h"
#include "simUtil/ExampleResources.h"
#include "OsgImGuiHandler.h"
#include "Platform/PlatformManager.h"
#include "Platform/RouteGraphics.h"
#include "Platform/RiskGraphics.h"
#include "Integration/DemoController.h"
#include "Integration/DemoScenario.h"
#include "ControlPanel.h"

namespace {
struct Commands { bool paused=false,reset=false; float rate=5.f; };
// SDK 1.25 window wrapper around person 3's unchanged panel logic.
class RiskPanel : public ControlPanel {
public:
    void draw(osg::RenderInfo& ri) override {
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x-10,30),ImGuiCond_Always,ImVec2(1,0));
        ImGui::SetNextWindowSize(ImVec2(290,0),ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(.92f);
        ImGui::Begin(name(),nullptr,ImGuiWindowFlags_NoResize);
        ControlPanel::draw(ri);
        ImGui::End();
    }
};
class FlightPanel : public simExamples::SimExamplesGui {
public:
    FlightPanel(PlatformManager& p,DemoController& d,ControlPanel& risk,Commands& c)
        :SimExamplesGui("MERGEN | Platform Simulation"),p_(p),d_(d),risk_(risk),c_(c) {}
    void draw(osg::RenderInfo&) override {
        ImGui::SetNextWindowPos(ImVec2(10,30),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(440,0),ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowBgAlpha(.92f);
        ImGui::Begin(name(),nullptr,ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::TextColored(ImVec4(.25f,.8f,1.f,1.f),"MERGEN - LIVE FLIGHT DEMO");
        ImGui::Text("Simulation time: %.1f s",p_.simulationTime());
        if(ImGui::Button(c_.paused?"Resume":"Pause")) c_.paused=!c_.paused;
        ImGui::SameLine(); if(ImGui::Button("Reset scenario")) c_.reset=true;
        ImGui::SliderFloat("Time scale",&c_.rate,1.f,20.f,"%.1fx");
        if(d_.hasBlockedRoute()) ImGui::TextColored(ImVec4(1,.3f,.3f,1),"SAFETY HOLD: change the zone or reset.");
        ImGui::Separator();
        if(ImGui::BeginTable("flights",4,ImGuiTableFlags_Borders|ImGuiTableFlags_RowBg)) {
            for(const char* s:{"Platform","Speed m/s","Alt m","Flight"}) ImGui::TableSetupColumn(s);
            ImGui::TableHeadersRow();
            for(int id:p_.getPlatformIds()) {
                const auto s=p_.getState(id);
                ImGui::TableNextRow(); ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(p_.getName(id).c_str());
                ImGui::TableSetColumnIndex(1); ImGui::Text("%.1f",s.speed);
                ImGui::TableSetColumnIndex(2); ImGui::Text("%.0f",s.altitude);
                ImGui::TableSetColumnIndex(3); ImGui::TextUnformatted(p_.hasArrived(id)?"ARRIVED":"FLYING");
            }
            ImGui::EndTable();
        }
        for(int id:p_.getPlatformIds()) {
            const auto& status=d_.status(id);
            if(!status.message.empty()) ImGui::TextWrapped("%s",status.message.c_str());
        }
        ImGui::Separator();
        ImGui::TextWrapped("Thin colored: original routes. Green: new route.\nRed circle: risk. Yellow circle: 500 m safety margin.");
        ImGui::TextWrapped("Activate the zone early to reroute IHA-2. Reset to repeat.");
        if(ImGui::CollapsingHeader("Risk zone center")) {
            auto z=risk_.getRiskZoneData();
            bool changed=ImGui::InputDouble("Latitude",&z.latitude,.001,.01,"%.5f");
            changed=ImGui::InputDouble("Longitude",&z.longitude,.001,.01,"%.5f")||changed;
            if(changed&&std::isfinite(z.latitude)&&std::isfinite(z.longitude)&&std::abs(z.latitude)<=85.&&std::abs(z.longitude)<=180.) risk_.setRiskZoneData(z);
        }
        ImGui::End();
    }
private: PlatformManager& p_; DemoController& d_; ControlPanel& risk_; Commands& c_;
};
class Capture : public osg::Camera::DrawCallback {
public:
    mutable std::string pending;
    mutable bool failed=false;
    void operator()(osg::RenderInfo& ri) const override {
        if(pending.empty()) return;
        auto* vp=ri.getCurrentCamera()->getViewport();
        osg::ref_ptr<osg::Image> image=new osg::Image;
        image->readPixels(static_cast<int>(vp->x()),static_cast<int>(vp->y()),
            static_cast<int>(vp->width()),static_cast<int>(vp->height()),GL_RGB,GL_UNSIGNED_BYTE);
        if(!osgDB::writeImageFile(*image,pending)) failed=true;
        pending.clear();
    }
};
}
int main(int argc,char** argv) {
    try {
        bool smoke=false; std::string captureDir="artifacts";
        for(int i=1;i<argc;++i) {
            const std::string arg=argv[i];
            if(arg=="--smoke-test") smoke=true;
            else if(arg=="--capture-dir"&&i+1<argc) captureDir=argv[++i];
            else if(arg=="--help") { std::cout<<"MergenDemo [--smoke-test] [--capture-dir DIRECTORY]\n"; return 0; }
            else throw std::invalid_argument("Unknown or incomplete argument: "+arg);
        }
        simCore::checkVersionThrow();
        simExamples::enableHighDpiSupport(); simExamples::configureSearchPaths();
        simData::MemoryDataStore store;
        PlatformManager platforms(store);
        Commands commands;
        osg::ref_ptr<simVis::Viewer> viewer=new simVis::Viewer(simVis::Viewer::WINDOWED,40,40,1440,900);
        viewer->getViewer()->setThreadingModel(osgViewer::ViewerBase::SingleThreaded);
        viewer->setMap(simExamples::createDefaultExampleMap());
        auto* view=viewer->getMainView();
        view->lookAt(22.095,-159.655,1500,0,-65,21000);
        auto* scenario=viewer->getSceneManager()->getScenario();
        scenario->bind(&store);
        createDemoScenario(platforms);
        DemoController controller(platforms);
        RouteGraphics routes(*scenario,platforms);
        RiskGraphics riskGraphics(*scenario);
        auto* panel=new RiskPanel;
        panel->setRiskZoneData(defaultRiskZone());
        auto* gui=new GUI::OsgImGuiHandler;
        gui->add(new FlightPanel(platforms,controller,*panel,commands)); gui->add(panel);
        view->getEventHandlers().emplace_front(gui);
        osg::ref_ptr<Capture> capture=new Capture;
        view->getCamera()->setFinalDrawCallback(capture.get());
        viewer->installDebugHandlers(); viewer->getViewer()->realize();
        osgViewer::ViewerBase::Windows windows;
        viewer->getViewer()->getWindows(windows);
        for(auto* window:windows) window->setWindowName("MERGEN - Dinamik Risk Bolgesi Demo");
        auto last=std::chrono::steady_clock::now();
        unsigned frame=0;
        if(smoke) std::filesystem::create_directories(captureDir);
        while(!viewer->getViewer()->done()) {
            const auto now=std::chrono::steady_clock::now();
            const double dt=smoke?.25:std::min(.1,std::chrono::duration<double>(now-last).count());
            last=now;
            if(commands.reset) {
                platforms.reset(); controller.reset(); panel->setRiskZoneData(defaultRiskZone());
                commands.reset=false; commands.paused=false;
            }
            if(smoke&&frame==100) { auto z=defaultRiskZone(); z.active=true; panel->setRiskZoneData(z); }
            // Process zone edits before advancing: unsafe candidates never move a UAV.
            controller.update(panel->getRiskZoneData());
            if(!commands.paused&&!controller.hasBlockedRoute()) platforms.update(platforms.simulationTime()+dt*(smoke?1.:commands.rate));
            controller.update(panel->getRiskZoneData());
            routes.refresh(); riskGraphics.refresh(controller.zone(),controller.safetyMargin());
            for(int id:platforms.getPlatformIds()) {
                const auto& s=controller.status(id);
                panel->updateUAVStatus(id,platforms.getName(id),s.risk,s.rerouted&&!s.blocked&&s.risk==RiskState::SAFE);
            }
            if(smoke&&(frame==80||frame==101||frame==200)) {
                const char* filename=frame==80?"before-route.png":frame==101?"violation.png":"after-route.png";
                capture->pending=(std::filesystem::path(captureDir)/filename).string();
            }
            viewer->frame();
            if(smoke&&++frame>=220) break;
            if(!smoke) std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        scenario->unbind(&store);
        if(smoke) {
            if(controller.status(2).applications!=1||controller.status(1).applications||controller.status(3).applications||controller.hasBlockedRoute()||capture->failed)
                throw std::runtime_error("Render smoke test did not complete the integrated risk scenario");
            for(int id:platforms.getPlatformIds()) {
                const auto s=platforms.getState(id);
                std::cout<<platforms.getName(id)<<" "<<s.latitude<<" "<<s.longitude<<" "<<s.altitude<<" applications="<<controller.status(id).applications<<"\n";
            }
            std::cout<<"PASS: 3 live SIMDIS platforms, cemre detection, berra planning/UI, validated IHA-2 reroute, 3 rendered frames\n";
        }
        return 0;
    } catch(const std::exception& e) { std::cerr<<"MERGEN: "<<e.what()<<"\n"; return 1; }
}
