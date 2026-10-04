#include "RouteGraphics.h"
#include <algorithm>
#include <osg/MatrixTransform>
#include <osgEarth/LineDrawable>
#include "simCore/Calc/CoordinateConverter.h"
#include "simCore/Calc/Coordinate.h"

namespace {
osg::Vec3d ecef(const Waypoint& p) {
    constexpr double rad=3.14159265358979323846/180.;
    auto c=simCore::CoordinateConverter::convertGeodeticToEcef(
        simCore::Coordinate(simCore::COORD_SYS_LLA,simCore::Vec3(p.latitude*rad,p.longitude*rad,p.altitude)));
    return {c->x(),c->y(),c->z()};
}
osg::Node* line(const std::vector<Waypoint>& route,const osg::Vec4& color,float width) {
    if(route.empty()) return new osg::Group;
    const auto origin=ecef(route.front());
    osg::ref_ptr<osg::MatrixTransform> transform=new osg::MatrixTransform(osg::Matrix::translate(origin));
    osg::ref_ptr<osgEarth::LineDrawable> path=new osgEarth::LineDrawable(GL_LINE_STRIP);
    path->setColor(color); path->setLineWidth(width);
    // Local vertices avoid loss of precision from float ECEF coordinates.
    for (const auto& p:route) path->pushVertex(ecef(p)-origin);
    path->finish();
    transform->addChild(path.get());
    return transform.release();
}
}
RouteGraphics::RouteGraphics(osg::Group& parent,const PlatformManager& platforms)
    : platforms_(platforms),root_(new osg::Group) { parent.addChild(root_.get()); refresh(); }
void RouteGraphics::refresh() {
    bool changed=revisions_.empty();
    for(int id:platforms_.getPlatformIds()) if(revisions_[id]!=platforms_.getRouteRevision(id)) changed=true;
    if(!changed) return;
    root_->removeChildren(0,root_->getNumChildren());
    static const osg::Vec4 colors[]={{.21f,.77f,.94f,1.f},{1.f,.70f,.28f,1.f},{.78f,.57f,.92f,1.f}};
    for(int id:platforms_.getPlatformIds()) {
        const auto original=platforms_.getOriginalRoute(id);
        const auto assigned=platforms_.getAssignedRoute(id);
        root_->addChild(line(original,colors[(id-1)%3],2.f));
        const bool same=original.size()==assigned.size() && std::equal(original.begin(),original.end(),assigned.begin(),
            [](const Waypoint& a,const Waypoint& b){return a.latitude==b.latitude&&a.longitude==b.longitude&&a.altitude==b.altitude;});
        if(!same) root_->addChild(line(assigned,osg::Vec4(.30f,1.f,.48f,1.f),4.f));
        revisions_[id]=platforms_.getRouteRevision(id);
    }
}
