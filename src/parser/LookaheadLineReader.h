#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <filesystem>
#include <optional>

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
        LookaheadLineReader(const fs::path& file);
        bool get_next_line();
        void put_line_back();
        std::vector<std::string_view> get_line_tokens();
        int get_row_code();

    private:
        std::ifstream m_fstream;
        std::string m_current_line;
        std::optional<std::string> m_buffered_line;
        int m_row_code;
        fs::path m_path;
        std::vector<std::string_view> m_line_tokens;

        void tokenize_line(std::string_view line);

};