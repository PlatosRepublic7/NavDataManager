#pragma once
#include <string>
#include <optional>

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

using AirportData = AirportMeta;