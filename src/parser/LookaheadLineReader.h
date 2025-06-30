#pragma once
#include <string>
#include <string_view>
#include <sstream>
#include <vector>
#include <fstream>
#include <filesystem>
#include <optional>
#include <chrono>

namespace fs = std::filesystem;

/*
    The purpose of this class is to act as a wrapper for getline() that allows for "putting back" lines.
    The pattern is simple: We use a private buffer to hold lines we have "put back". So any time get_next_line() is called,
    we check m_buffered_line: If it's empty, we call getline() as per usual, but if something is in m_buffered_line, we std::move() it
    into the current_line, and clear the buffer. When we call put_line_back() we check if the buffer is full, and add the line to it or throw
    an error (only one line is allowed in the buffer at a time). We also incorporate tokenization here as a convenience, since the parser design
    revolves around it, as well as row_code generation.
*/

class LookaheadLineReader {
    public:
        LookaheadLineReader(const fs::path& file, bool logging = false);
        bool get_next_line();
        void put_line_back();
        std::vector<std::string_view> get_line_tokens();
        int get_row_code();
        int get_line_number() { return m_line_number; }

    private:
        // File-related memeber variables
        std::ifstream m_fstream;
        fs::path m_path;
        uintmax_t m_file_size;                          // File size in bytes
        uintmax_t m_bytes_processed;

        // Progress Tracking (mutable because it's internal bookkeeping)
        mutable std::chrono::steady_clock::time_point m_last_progress_update;
        mutable std::ostringstream m_progress_stream;
        bool m_logging_enabled;

        // Line-related member variables
        int m_line_number;
        std::string m_current_line;
        std::optional<std::string> m_buffered_line;
        int m_row_code;
        std::vector<std::string_view> m_line_tokens;

        void tokenize_line(std::string_view line);
        void get_progress(bool in_progress = true);
};