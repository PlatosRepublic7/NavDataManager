#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <optional>
#include <SQLiteCpp/Database.h>

namespace fs = std::filesystem;

struct AirportMeta {
    std::string icao;
    std::string iata;
    std::string faa;
    std::string airport_name;
    int elevation;
    std::string type;
    double latitude;
    double longitude;
    std::string country;
    std::string city;
    std::string region;
    int transition_level;
    int transition_alt;
};

class XPlaneDatParser {
    public:
        XPlaneDatParser(bool logging = false);

        // Parses a single .dat file and returns structured data or writes to a database
        void parseAirportDat(const fs::path& file, SQLite::Database* db);

    private:
        bool m_loggingEnabled;
        AirportMeta m_currentAirport;
        std::string m_currentAirportICAO;
};