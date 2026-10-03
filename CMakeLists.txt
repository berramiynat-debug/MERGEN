cmake_minimum_required(VERSION 3.15)
project(SIMDIS_RiskZone_Demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Navigation Library (Kişi 3)
add_library(Navigation
    Navigation/RoutePlanner.cpp
    Navigation/RoutePlanner.h
    Navigation/RouteValidator.cpp
    Navigation/RouteValidator.h
)

# UI Library (Kişi 3)
add_library(UI
    UI/ControlPanel.cpp
    UI/ControlPanel.h
)

# Kişi 3 Unit Tests
add_executable(TestRoutePlanner Tests/TestRoutePlanner.cpp)
target_link_libraries(TestRoutePlanner PRIVATE Navigation)

add_executable(TestRouteValidator Tests/TestRouteValidator.cpp)
target_link_libraries(TestRouteValidator PRIVATE Navigation)
