#include "LookaheadLineReader.h"
#include <iostream>
#include <cctype>

namespace fs = std::filesystem;

LookaheadLineReader::LookaheadLineReader(const fs::path& file) : m_path(file), m_fstream(file) {}

bool LookaheadLineReader::getNextLine() {
    bool success = false;
    if (m_bufferedLine) {
        m_currentLine = std::move(*m_bufferedLine);
        m_bufferedLine.reset();
        success = true;
    } else {
        success = static_cast<bool>(std::getline(m_fstream, m_currentLine));
    }
    tokenizeLine(m_currentLine);
    
    return success;
}

void LookaheadLineReader::putLineBack() {
    if (m_bufferedLine) {
        throw std::logic_error("Cannot put back more than one line at a time.");
    }
    m_bufferedLine = m_currentLine;
}

std::vector<std::string_view> LookaheadLineReader::getLineTokens() {
    // This should also compute and validate m_rowCode
    std::string sRowCode;
    if (!m_lineTokens.empty()) {
        try {
            std::string rcLineToken = std::string(m_lineTokens[0]);
            if (std::isdigit(rcLineToken[0])) {
                m_rowCode = std::stoi(rcLineToken);
            } else {
                m_rowCode = -1;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error computing row code: " << e.what() << std::endl;
            throw;
        }
    }
    return m_lineTokens;
}

int LookaheadLineReader::getRowCode() {
    return m_rowCode;
}

void LookaheadLineReader::tokenizeLine(std::string_view line) {
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
    m_lineTokens = tokens;
}