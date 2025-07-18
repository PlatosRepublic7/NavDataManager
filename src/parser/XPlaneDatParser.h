#pragma once
#include "LookaheadLineReader.h"
#include <NavDataManager/Types.h>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <SQLiteCpp/Database.h>

namespace fs = std::filesystem;

// Container for all parsed data from a single .dat file
struct ParsedAptData {
    std::vector<AirportMeta> airports;
    std::vector<RunwayData> runways;
    std::vector<TaxiwayNodeData> taxiway_nodes;
    std::vector<TaxiwayEdgeData> taxiway_edges;
    std::vector<LinearFeatureData> linear_features;
    std::vector<LinearFeatureNodeData> linear_feature_nodes;
};

class XPlaneDatParser {
    public:
        explicit XPlaneDatParser(bool logging = false);

        // Returns all parsed data structures, ready for database insertion
        ParsedAptData parse_airport_dat(const fs::path& file);

    private:
        bool m_logging_enabled;
        AirportMeta m_current_airport;
        std::string m_current_airport_icao;
        int m_current_airport_feature_sequence = 1; // Reset per airport
        
        void reset_airport_context(const std::string& new_icao) {
            m_current_airport_icao = new_icao;
            m_current_airport_feature_sequence = 1;
        }

        void process_airport_meta(LookaheadLineReader& reader, ParsedAptData& data);
        void process_runway(LookaheadLineReader& reader, ParsedAptData& data);
        void process_taxiway_node(LookaheadLineReader& reader, ParsedAptData& data);
        void process_taxiway_edge(LookaheadLineReader& reader, ParsedAptData& data);
        void process_linear_feature(LookaheadLineReader& reader, ParsedAptData& data);

        std::ostringstream write_parser_error(LookaheadLineReader& reader, std::vector<std::string_view>& tokens, const std::exception& e);
};