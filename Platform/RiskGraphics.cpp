#include "RiskGraphics.h"
#include <cmath>
#include <osg/MatrixTransform>
#include <osgEarth/LineDrawable>
#include "simCore/Calc/Coordinate.h"
#include "simCore/Calc/CoordinateConverter.h"

namespace {
constexpr double rad=3.14159265358979323846/180.;
osg::Vec3d ecef(double lat,double lon,double alt) {
    const auto p=simCore::CoordinateConverter::convertGeodeticToEcef(
        simCore::Coordinate(simCore::COORD_SYS_LLA,simCore::Vec3(lat*rad,lon*rad,alt)));
    return {p->x(),p->y(),p->z()};
}
osg::Node* ring(const RiskZoneData& z,double radius,const osg::Vec4& color,float width) {
    const auto origin=ecef(z.latitude,z.longitude,1900.);
    osg::ref_ptr<osg::MatrixTransform> t=new osg::MatrixTransform(osg::Matrix::translate(origin));
    osg::ref_ptr<osgEarth::LineDrawable> line=new osgEarth::LineDrawable(GL_LINE_LOOP);
    line->setColor(color); line->setLineWidth(width);
    for(unsigned i=0;i<128;++i) {
        const double angle=2.*3.14159265358979323846*i/128.;
        const double lat=z.latitude+radius*std::sin(angle)/111320.;
        const double lon=z.longitude+radius*std::cos(angle)/(111320.*std::cos(z.latitude*rad));
        line->pushVertex(ecef(lat,lon,1900.)-origin);
    }
    line->finish(); t->addChild(line.get()); return t.release();
}
}
RiskGraphics::RiskGraphics(osg::Group& parent):root_(new osg::Group) { parent.addChild(root_.get()); }
void RiskGraphics::refresh(const RiskZoneData& z,double margin) {
    if(z.latitude==previous_.latitude&&z.longitude==previous_.longitude&&z.radiusMeters==previous_.radiusMeters
        &&z.active==previous_.active&&margin==previousMargin_) return;
    previous_=z; previousMargin_=margin;
    root_->removeChildren(0,root_->getNumChildren());
    if(!z.active) return;
    root_->addChild(ring(z,z.radiusMeters,{1.f,.18f,.18f,1.f},4.f));
    root_->addChild(ring(z,z.radiusMeters+margin,{1.f,.85f,.20f,1.f},2.f));
}
