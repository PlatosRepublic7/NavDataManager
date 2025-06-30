#pragma once
#include "LookaheadLineReader.h"
#include "NavDataManager/Types.h"
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <optional>
#include <SQLiteCpp/Database.h>

namespace fs = std::filesystem;

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
};

// Container for all parsed data from a single .dat file
struct ParsedAptData {
    std::vector<AirportMeta> airports;
    std::vector<RunwayData> runways;
};

class XPlaneDatParser {
    public:
        XPlaneDatParser(bool logging = false);

        std::vector<AirportMeta> airport_meta_batch;

        // Returns all parsed data structures, ready for database insertion
        ParsedAptData parse_airport_dat(const fs::path& file);

    private:
        bool m_logging_enabled;
        AirportMeta m_current_airport;
        std::string m_current_airport_icao;

        void process_airport_meta(LookaheadLineReader& reader, ParsedAptData& data);
};