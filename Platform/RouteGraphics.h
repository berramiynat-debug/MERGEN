#pragma once
#include "PlatformManager.h"
#include <osg/Group>
#include <osg/ref_ptr>
#include <map>

class RouteGraphics {
public:
    RouteGraphics(osg::Group& parent, const PlatformManager& platforms);
    void refresh();
private:
    const PlatformManager& platforms_;
    osg::ref_ptr<osg::Group> root_;
    std::map<int,std::size_t> revisions_;
};
