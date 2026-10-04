#pragma once
#include "CommonTypes.h"
#include <osg/Group>
#include <osg/ref_ptr>

class RiskGraphics {
public:
    explicit RiskGraphics(osg::Group& parent);
    void refresh(const RiskZoneData& zone,double safetyMargin);
private:
    osg::ref_ptr<osg::Group> root_;
    RiskZoneData previous_{};
    double previousMargin_=-1.;
};
