cmake_minimum_required(VERSION 3.15)
project(SIMDIS_RiskZone_Demo CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────────────────────────────
# Paths - SIMDIS SDK
# ─────────────────────────────────────────────────────────────
set(SIMDIS_SDK_BUILD_DIR "D:/Projects/simdissdk/build"
    CACHE PATH "SIMDIS SDK build output directory")

set(SIMDIS_SDK_SRC_DIR "D:/Projects/simdissdk"
    CACHE PATH "SIMDIS SDK source root directory")

set(IMGUI_INCLUDE_DIR "D:/Projects/simdissdk/3rd/win64_vc-14.5/ImGui/1.92.4"
    CACHE PATH "ImGui include directory")

set(IMGUI_EXAMPLES_DIR "${SIMDIS_SDK_SRC_DIR}/Examples/imgui"
    CACHE PATH "SIMDIS SDK imgui example helpers directory")

# ─────────────────────────────────────────────────────────────
# Try to locate SIMDIS SDK CMake config
# ─────────────────────────────────────────────────────────────
list(APPEND CMAKE_MODULE_PATH "${SIMDIS_SDK_BUILD_DIR}/cmake")
include("${SIMDIS_SDK_BUILD_DIR}/cmake/ImportSIMDIS_SDK.cmake" OPTIONAL
        RESULT_VARIABLE SIMDIS_IMPORT_RESULT)

if(SIMDIS_IMPORT_RESULT)
    message(STATUS "SIMDIS SDK found via ImportSIMDIS_SDK.cmake")
    set(SIMDIS_SDK_FOUND TRUE)
    add_definitions(-DSIMDIS_SDK_FOUND)
else()
    message(WARNING "SIMDIS SDK import file not found - building in MOCK/HEADLESS mode")
    set(SIMDIS_SDK_FOUND FALSE)
endif()

# ─────────────────────────────────────────────────────────────
# Global includes
# ─────────────────────────────────────────────────────────────
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}          # for CommonTypes.h
    ${IMGUI_INCLUDE_DIR}                 # for imgui.h
    ${IMGUI_EXAMPLES_DIR}               # for SimExamplesGui.h, OsgImGuiHandler.h
)

if(SIMDIS_SDK_FOUND)
    include_directories(
        ${SIMDIS_SDK_SRC_DIR}            # SIMDIS SDK source headers
        ${SIMDIS_SDK_BUILD_DIR}          # generated headers
    )
endif()

# ─────────────────────────────────────────────────────────────
# Navigation Library (Kisi 3)
# ─────────────────────────────────────────────────────────────
add_library(Navigation STATIC
    Navigation/RoutePlanner.cpp
    Navigation/RoutePlanner.h
    Navigation/RouteValidator.cpp
    Navigation/RouteValidator.h
)

# ─────────────────────────────────────────────────────────────
# UI Library (Kisi 3)
# ─────────────────────────────────────────────────────────────
set(UI_SOURCES
    UI/ControlPanel.cpp
    UI/ControlPanel.h
    ${IMGUI_EXAMPLES_DIR}/SimExamplesGui.cpp
    ${IMGUI_EXAMPLES_DIR}/SimExamplesGui.h
)

if(SIMDIS_SDK_FOUND)
    list(APPEND UI_SOURCES
        ${IMGUI_EXAMPLES_DIR}/OsgImGuiHandler.cpp
        ${IMGUI_EXAMPLES_DIR}/OsgImGuiHandler.h
    )
endif()

add_library(UI STATIC ${UI_SOURCES})

target_include_directories(UI PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_INCLUDE_DIR}
    ${IMGUI_EXAMPLES_DIR}
)

if(SIMDIS_SDK_FOUND)
    target_link_libraries(UI PUBLIC
        simVis
        simCore
        simUtil
        simData
        simNotify
        ImGuiLib
    )
endif()

# ─────────────────────────────────────────────────────────────
# Kisi 3 Unit Tests (headless, no SIMDIS needed)
# ─────────────────────────────────────────────────────────────
add_executable(TestRoutePlanner Tests/TestRoutePlanner.cpp)
target_link_libraries(TestRoutePlanner PRIVATE Navigation)

add_executable(TestRouteValidator Tests/TestRouteValidator.cpp)
target_link_libraries(TestRouteValidator PRIVATE Navigation)
