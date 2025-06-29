#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

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