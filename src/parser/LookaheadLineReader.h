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
        bool getNextLine();
        void putLineBack();
        std::vector<std::string_view> getLineTokens();
        int getRowCode();

    private:
        std::ifstream m_fstream;
        std::string m_currentLine;
        std::optional<std::string> m_bufferedLine;
        int m_rowCode;
        fs::path m_path;
        std::vector<std::string_view> m_lineTokens;

        void tokenizeLine(std::string_view line);

};