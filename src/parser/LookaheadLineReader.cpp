#include "LookaheadLineReader.h"
#include <iostream>
#include <cctype>

namespace fs = std::filesystem;

LookaheadLineReader::LookaheadLineReader(const fs::path& file, bool logging) : m_path(file), m_line_number(0), m_bytes_processed(0), m_logging_enabled(logging) {
    // Open in binary mode for accurate byte counting
    m_fstream.open(file, std::ios::binary);
    
    if (!m_fstream.is_open()) {
        throw std::runtime_error("Could not open file: " + m_path.string());
    }
    if (fs::exists(m_path) && fs::is_regular_file(m_path)) {
        m_file_size = fs::file_size(file);
    }
}

bool LookaheadLineReader::get_next_line() {
    bool success = false;
    if (m_buffered_line) {
        m_current_line = std::move(*m_buffered_line);
        m_buffered_line.reset();
        success = true;
    } else {
        success = static_cast<bool>(std::getline(m_fstream, m_current_line));
        m_line_number += 1; 
    }

    if (success) {
        m_bytes_processed += m_current_line.length() + 1;
    }
    
    get_progress(success);
    return success;
}

void LookaheadLineReader::put_line_back() {
    if (m_buffered_line) {
        throw std::logic_error("Cannot put back more than one line at a time.");
    }
    m_buffered_line = m_current_line;
    m_bytes_processed -= m_current_line.length() + 1;
    
    // We want to clear the tokens vector to prepare for getting it again and re-tokenizing
    m_line_tokens.clear();
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

void LookaheadLineReader::get_progress(bool in_progress) {
    if (m_file_size == 0) return;

    int bar_width = 50, update_interval = 50;
    if (in_progress && m_logging_enabled) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_progress_update).count() < update_interval) return;
        m_last_progress_update = now;

        float percent = static_cast<float>(m_bytes_processed) / m_file_size;
        if (percent > 1.0f) percent = 1.0f;

        int pos = static_cast<int>(bar_width * percent);

        m_progress_stream.str("");
        m_progress_stream.clear();
        m_progress_stream << "\r[";
        for (int i = 0; i < bar_width; ++i) {
            if (i < pos) {
                m_progress_stream << "=";
            } else if (i == pos) {
                m_progress_stream << ">";
            } else {
                m_progress_stream << " ";
            }
        }

        m_progress_stream << "] " << std::setw(3) << static_cast<int>(percent * 100.0) << "%" << std::setw(11) << m_bytes_processed << "/" << m_file_size << " bytes | " << m_path;
        std::cout << m_progress_stream.str();
        std::cout.flush();
    } else if (!in_progress && m_logging_enabled) {
        std::cout << "\r[";
        for (int i = 0; i < bar_width; ++i) {
            std::cout << "=";
        }
        std::cout << "] 100%" << std::setw(11) << m_bytes_processed << "/" << m_file_size << " bytes | " << m_path << std::endl;
    } else {
        return;
    }
}