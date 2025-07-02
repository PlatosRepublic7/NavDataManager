#pragma once
#include <string>
#include <optional>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct AirportMeta {
    std::optional<std::string> icao;
    std::optional<std::string> iata;
    std::optional<std::string> faa;
    std::optional<std::string> airport_name;
    std::optional<int> elevation;
    std::optional<std::string> type;
    std::optional<double> latitude;
    std::optional<double> longitude;
    std::optional<std::string> country;
    std::optional<std::string> city;
    std::optional<std::string> region;
    std::optional<std::string> transition_level;
    std::optional<std::string> transition_alt;

    // Convenience methods
    bool has_coordinates() const {
        return latitude.has_value() && longitude.has_value();
    }

    std::string display_name() const {
        if (airport_name.has_value()) return *airport_name;
        if (icao.has_value()) return *icao;
        return "Unknown Airport";
    }
};

struct RunwayData {
    std::optional<std::string> airport_icao;  // Foreign key to airport
    std::optional<double> width;
    std::optional<int> surface;
    std::optional<std::string> end1_rw_number;
    std::optional<double> end1_lat;
    std::optional<double> end1_lon;
    std::optional<double> end1_d_threshold;
    std::optional<int> end1_rw_marking_code;
    std::optional<int> end1_rw_app_light_code;
    std::optional<std::string> end2_rw_number;
    std::optional<double> end2_lat;
    std::optional<double> end2_lon;
    std::optional<double> end2_d_threshold;
    std::optional<int> end2_rw_marking_code;
    std::optional<int> end2_rw_app_light_code;

    std::string full_runway_name() const {
        return end1_rw_number.value_or("??") + "/" + end2_rw_number.value_or("??");
    }

    double length_meters() const {
        // Compute Haversine distance
        if (!end1_lat || !end1_lon || !end2_lat || !end2_lon) return 0.0;
        const double R = 6371000.0; // Earth's radius in meters
        const double lat1_rad = *end1_lat * M_PI / 180.0;
        const double lat2_rad = *end2_lat * M_PI / 180.0;
        const double delta_lat = (*end2_lat - *end1_lat) * M_PI / 180.0;
        const double delta_lon = (*end2_lon - *end1_lon) * M_PI / 180.0;

        const double a = std::sin(delta_lat/2) * std::sin(delta_lat/2) +
                        std::cos(lat1_rad) * std::cos(lat2_rad) *
                        std::sin(delta_lon/2) * std::sin(delta_lon/2);
        const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));

        return R * c;
    }

    double length_feet() const {
        return length_meters() * 3.28084;  // Convert to feet
    }
};