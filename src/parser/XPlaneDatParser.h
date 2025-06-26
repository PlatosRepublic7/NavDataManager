#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <SQLiteCpp/Database.h>

namespace fs = std::filesystem;

class XPlaneDatParser {
    public:
        XPlaneDatParser(bool logging = false);

        // Parses a single .dat file and returns structured data or writes to a database
        void parseAirportDat(const fs::path& file, SQLite::Database* db);

    private:
        bool m_loggingEnabled;

        std::vector<std::string_view> tokenizeLine(std::string_view line);
};