#include "XPlaneDatParser.h"
#include <iostream>
#include <fstream>

namespace fs = std::filesystem;

XPlaneDatParser::XPlaneDatParser(bool logging) : m_loggingEnabled(logging) {}

void XPlaneDatParser::parseAirportDat(const fs::path& file, SQLite::Database* db) {
    std::ifstream infile(file);
    std::string line;
    while (std::getline(infile, line)) {
        auto tokens = tokenizeLine(line);
    }
}

std::vector<std::string_view> XPlaneDatParser::tokenizeLine(std::string_view line) {
    std::vector<std::string_view> tokens;
    size_t start = 0, end;
    while ((end = line.find_first_of(" \t", start)) != std::string_view::npos) {
        if (end > start) {
            tokens.emplace_back(line.substr(start, end - start));
        }
        start = line.find_first_not_of(" \t", end);
        if (start == std::string_view::npos) break;
    }
    if (start < line.size()) {
        tokens.emplace_back(line.substr(start));
    }
    return tokens;
}