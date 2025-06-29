#include "LookaheadLineReader.h"
#include <iostream>
#include <cctype>

namespace fs = std::filesystem;

LookaheadLineReader::LookaheadLineReader(const fs::path& file) : m_path(file), m_fstream(file) {}

bool LookaheadLineReader::get_next_line() {
    bool success = false;
    if (m_buffered_line) {
        m_current_line = std::move(*m_buffered_line);
        m_buffered_line.reset();
        success = true;
    } else {
        success = static_cast<bool>(std::getline(m_fstream, m_current_line));
    }
    
    return success;
}

void LookaheadLineReader::put_line_back() {
    if (m_buffered_line) {
        throw std::logic_error("Cannot put back more than one line at a time.");
    }
    m_buffered_line = m_current_line;
}

std::vector<std::string_view> LookaheadLineReader::get_line_tokens() {
    tokenize_line(m_current_line);
    return m_line_tokens;
}

int LookaheadLineReader::get_row_code() {
    if (!m_line_tokens.empty()) {
        try {
            std::string rc_line_token = std::string(m_line_tokens[0]);
            if (std::isdigit(rc_line_token[0])) {
                m_row_code = std::stoi(rc_line_token);
            } else {
                m_row_code = -1;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error computing row code: " << e.what() << std::endl;
            throw;
        }
    }
    return m_row_code;
}

void LookaheadLineReader::tokenize_line(std::string_view line) {
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
    m_line_tokens = tokens;
}