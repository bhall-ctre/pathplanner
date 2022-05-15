#include "pathplanner/lib/PathPlanner.h"
#include <frc/geometry/Translation2d.h>
#include <frc/geometry/Rotation2d.h>
#include <frc/Filesystem.h>
#include <wpi/SmallString.h>
#include <wpi/raw_istream.h>
#include <wpi/json.h>
#include <units/length.h>
#include <units/angle.h>
#include <units/velocity.h>
#include <vector>

using namespace pathplanner;

double PathPlanner::resolution = 0.004;

PathPlannerTrajectory PathPlanner::loadPath(std::string name, units::meters_per_second_t maxVel, units::meters_per_second_squared_t maxAccel, bool reversed){
    std::string line;

    std::string filePath = frc::filesystem::GetDeployDirectory() + "/pathplanner/" + name + ".path";

    std::error_code error_code;
    wpi::raw_fd_istream input{filePath, error_code};

    if(error_code){
        throw std::runtime_error(("Cannot open file: " + filePath));
    }

    wpi::json json;
    input >> json;

    std::vector<PathPlannerTrajectory::Waypoint> waypoints;
    for (wpi::json::reference waypoint : json.at("waypoints")){
        wpi::json::reference jsonAnchor = waypoint.at("anchorPoint");
        double anchorX = jsonAnchor.at("x");
        double anchorY = jsonAnchor.at("y");
        frc::Translation2d anchorPoint = frc::Translation2d(units::meter_t{anchorX}, units::meter_t{anchorY});

        wpi::json::reference jsonPrevControl = waypoint.at("prevControl");
        frc::Translation2d prevControl;
        if(!jsonPrevControl.is_null()){
            double prevX = jsonPrevControl.at("x");
            double prevY = jsonPrevControl.at("y");
            prevControl = frc::Translation2d(units::meter_t{prevX}, units::meter_t{prevY});
        }

        wpi::json::reference jsonNextControl = waypoint.at("nextControl");
        frc::Translation2d nextControl;
        if(!jsonNextControl.is_null()){
            double nextX = jsonNextControl.at("x");
            double nextY = jsonNextControl.at("y");
            nextControl = frc::Translation2d(units::meter_t{nextX}, units::meter_t{nextY});
        }

        double holonomic = waypoint.at("holonomicAngle");
        frc::Rotation2d holonomicAngle = frc::Rotation2d(units::degree_t{holonomic});
        bool isReversal = waypoint.at("isReversal");
        units::meters_per_second_t velOverride = -1_mps;
        if(!waypoint.at("velOverride").is_null()){
            double vel = waypoint.at("velOverride");
            velOverride = units::meters_per_second_t{vel};
        }

        waypoints.push_back(PathPlannerTrajectory::Waypoint(anchorPoint, prevControl, nextControl, velOverride, holonomicAngle, isReversal));
    }

    std::vector<PathPlannerTrajectory::EventMarker> markers;

    if(json.find("markers") != json.end()){
        for(wpi::json::reference marker : json.at("markers")){
            PathPlannerTrajectory::EventMarker m(marker.at("name"), marker.at("position"));
            markers.push_back(m);
        }
    }

    return PathPlannerTrajectory(waypoints, markers, maxVel, maxAccel, reversed);
}