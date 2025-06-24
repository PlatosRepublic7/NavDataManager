#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <SQLiteCpp/Database.h>

namespace fs = std::filesystem;

class NavDataManager {
    public:
        // Constructor that initializes the database with the given directory
        NavDataManager(const std::string& db_directory);

        // Parse navigation data from files and populate the database
        void updateDatabase(const std::string& xp_root_path);

    private:
        SQLite::Database m_db;
        std::string m_dataDirectory;
        std::string m_xpDirectory;
        fs::path m_airportDataPath;

        void createTables();
};