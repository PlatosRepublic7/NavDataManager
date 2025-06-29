#include "XPlaneDatParser.h"
#include "LookaheadLineReader.h"
#include <iostream>
#include <fstream>

namespace fs = std::filesystem;

XPlaneDatParser::XPlaneDatParser(bool logging) : m_logging_enabled(logging) {}

void XPlaneDatParser::parse_airport_dat(const fs::path& file, SQLite::Database* db) {
    LookaheadLineReader reader(file);
    int row_code = -1;
    while (reader.get_next_line()) {
        // Tokenize the current line
        auto tokens = reader.get_line_tokens();
        if (tokens.empty()) continue;
        try {
            // Check row code to determine what to do with the current line's data
            row_code = reader.get_row_code();
        } catch (const std::exception& e) {
            std::cerr << "Error parsing " << file.string() << ": " << e.what() << std::endl;
            throw;
        }
    }
}
