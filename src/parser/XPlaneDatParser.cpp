#include "XPlaneDatParser.h"
#include "LookaheadLineReader.h"
#include <iostream>
#include <fstream>

namespace fs = std::filesystem;

XPlaneDatParser::XPlaneDatParser(bool logging) : m_loggingEnabled(logging) {}

void XPlaneDatParser::parseAirportDat(const fs::path& file, SQLite::Database* db) {
    LookaheadLineReader reader(file);
    int rowCode = -1;
    while (reader.getNextLine()) {
        // Tokenize the current line
        auto tokens = reader.getLineTokens();
        if (tokens.empty()) continue;
        try {
            // Check row code to determine what to do with the current line's data
            rowCode = reader.getRowCode();
        } catch (const std::exception& e) {
            std::cerr << "Error parsing " << file.string() << ": " << e.what() << std::endl;
            throw;
        }
    }
}
