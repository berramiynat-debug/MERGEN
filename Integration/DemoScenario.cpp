#include "DemoScenario.h"

void createDemoScenario(PlatformManager& p) {
    // Offline detailed SDK sample map. 3 different positions, heights and speeds.
    const int one=p.createPlatform("IHA-1",{22.06,-159.72,1600});
    p.setSpeed(one,70); p.setRoute(one,{{22.06,-159.72,1600},{22.06,-159.66,1600},{22.08,-159.59,1600}});
    const int two=p.createPlatform("IHA-2",{22.09,-159.72,1900});
    p.setSpeed(two,85); p.setRoute(two,{{22.09,-159.72,1900},{22.09,-159.66,1900},{22.09,-159.59,1900}});
    const int three=p.createPlatform("IHA-3",{22.12,-159.72,2200});
    p.setSpeed(three,100); p.setRoute(three,{{22.12,-159.72,2200},{22.13,-159.66,2200},{22.12,-159.59,2200}});
    p.update(0.);
}
RiskZoneData defaultRiskZone() { return {22.09,-159.66,1500,false}; }
